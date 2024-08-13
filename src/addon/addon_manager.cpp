//  SuperTux - Add-on Manager
//  Copyright (C) 2007 Christoph Sommer <christoph.sommer@2007.expires.deltadevelopment.de>
//                2014 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "addon/addon_manager.hpp"

#include <fmt/format.h>
#include <sstream>

#include <physfs.h>
#include "zip_manager.hpp"

#include "addon/addon.hpp"
#include "addon/addon_index.hpp"
#include "addon/md5.hpp"
#include "gui/dialog.hpp"
#include "physfs/util.hpp"
#include "supertux/globals.hpp"
#include "supertux/menu/addon_menu.hpp"
#include "supertux/menu/menu_storage.hpp"
#include "util/file_system.hpp"
#include "util/gettext.hpp"
#include "util/log.hpp"
#include "util/reader.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include "util/string_util.hpp"

namespace {

static const std::string ADDON_REPOSITORY_URL = "http://localhost:3000/api/v1/repos/addons";
static const std::string LANGUAGE_ADDON_ID = "language-pack";

MD5 md5_from_file(const std::string& filename)
{
  // TODO: This does not work as expected for some files -- IFileStream seems to not always behave like an ifstream.
  //IFileStream ifs(installed_physfs_filename);
  //std::string md5 = MD5(ifs).hex_digest();

  MD5 md5;

  auto file = PHYSFS_openRead(filename.c_str());
  if (!file)
  {
    std::ostringstream out;
    out << "PHYSFS_openRead() failed: " << physfsutil::get_last_error();
    throw std::runtime_error(out.str());
  }
  else
  {
    while (true)
    {
      unsigned char buffer[1024];
      PHYSFS_sint64 len = PHYSFS_readBytes(file, buffer, sizeof(buffer));
      if (len <= 0) break;
      md5.update(buffer, static_cast<unsigned int>(len));
    }
    PHYSFS_close(file);

    return md5;
  }
}

MD5 md5_from_archive(const std::string& filename)
{
  if (physfsutil::is_directory(filename)) {
    return MD5();
  } else {
    return md5_from_file(filename);
  }
}

static Addon& get_addon(const AddonMap& list, const AddonId& id, bool installed)
{
  auto it = list.find(id);
  if (it == list.end())
    throw std::runtime_error("Couldn't find " + std::string(installed ? "installed" : "repository") +
      " addon with ID: " + id);

  return *(it->second);
}

static std::vector<AddonId> get_addons(const AddonMap& list)
{
  // Use a map for storing sorted add-on titles with their respective IDs.
  std::map<std::string, AddonId> sorted_titles;
  for (const auto& [id, addon] : list)
  {
    sorted_titles.insert({addon->get_title(), id});
  }
  std::vector<AddonId> results;
  results.reserve(sorted_titles.size());
  std::transform(sorted_titles.begin(), sorted_titles.end(),
                 std::back_inserter(results),
                 [](const auto& title_and_id)
                 {
                   return title_and_id.second;
                 });
  return results;
}

static PHYSFS_EnumerateCallbackResult add_to_dictionary_path(void *data, const char *origdir, const char *fname)
{
  std::string full_path = FileSystem::join(origdir, fname);
  if (physfsutil::is_directory(full_path))
  {
      log_debug << "Adding \"" << full_path << "\" to dictionary search path" << std::endl;
      // We want translations from add-ons to have precedence.
      g_dictionary_manager->add_directory(full_path, true);
  }
  return PHYSFS_ENUM_OK;
}

static PHYSFS_EnumerateCallbackResult remove_from_dictionary_path(void *data, const char *origdir, const char *fname)
{
  std::string full_path = FileSystem::join(origdir, fname);
  if (physfsutil::is_directory(full_path))
  {
      g_dictionary_manager->remove_directory(full_path);
  }
  return PHYSFS_ENUM_OK;
}

} // namespace

AddonManager::AddonManager(const std::string& addon_directory,
                           std::vector<Config::Addon>& addon_config) :
  m_downloader(),
  m_addon_directory(addon_directory),
  m_cache_directory(FileSystem::join(m_addon_directory, "cache")),
  m_screenshots_cache_directory(FileSystem::join(m_cache_directory, "screenshots")),
  m_repository_url(ADDON_REPOSITORY_URL),
  m_addon_config(addon_config),
  m_installed_addons(),
  m_initialized(false),
  m_transfer_statuses(new TransferStatusList),
  m_index_page_cache()
{
  if (!PHYSFS_mkdir(m_addon_directory.c_str()))
  {
    std::ostringstream msg;
    msg << "Couldn't create directory for addons '"
        << m_addon_directory << "': " << physfsutil::get_last_error();
    throw std::runtime_error(msg.str());
  }

  add_installed_addons();

  // FIXME: We should also restore the order here.
  for (auto& addon : m_addon_config)
  {
    if (addon.enabled)
    {
      try
      {
        enable_addon(addon.id);
      }
      catch(const std::exception& err)
      {
        log_warning << "Failed to enable addon '" << addon.id << "' from config: " << err.what() << std::endl;
      }
    }
  }

  if (!g_config->repository_url.empty() &&
      g_config->repository_url != m_repository_url)
  {
    m_repository_url = g_config->repository_url;
  }

  // Create the add-on cache directory, if it doesn't exist.
  if (!PHYSFS_exists(m_cache_directory.c_str()))
  {
    PHYSFS_mkdir(m_cache_directory.c_str());
  }
  else
  {
    empty_cache_directory();
  }

  m_initialized = true;
}

AddonManager::~AddonManager()
{
  // Sync enabled/disabled add-ons into the config for saving.
  m_addon_config.clear();
  for (const auto& [id, addon] : m_installed_addons)
  {
    m_addon_config.push_back({ id, addon->m_enabled });
  }

  // Delete the add-on cache directory, if it exists.
  physfsutil::remove_with_content(m_cache_directory);
}

void
AddonManager::empty_cache_directory()
{
  physfsutil::remove_content(m_cache_directory);
}

Addon&
AddonManager::get_installed_addon(const AddonId& id) const
{
  return get_addon(m_installed_addons, id, true);
}

std::vector<AddonId>
AddonManager::get_installed_addons() const
{
  return get_addons(m_installed_addons);
}

TransferStatusPtr
AddonManager::request_index(std::unique_ptr<AddonIndex>& index, int limit)
{
  std::stringstream url_stream;
  url_stream << m_repository_url << "?limit=" << limit;

  return request_index(index, url_stream.str());
}

TransferStatusPtr
AddonManager::request_index_page(std::unique_ptr<AddonIndex>& index, bool next_page)
{
  return request_index(index, next_page ? index->get_next_page_url() : index->get_previous_page_url());
}

TransferStatusPtr
AddonManager::request_index(std::unique_ptr<AddonIndex>& index, const std::string& url)
{
  TransferStatusPtr status = m_downloader.request_string_download(url, m_index_page_cache);
  status->then([this, url, &index](bool success)
    {
      if (!success) return;

      try
      {
        index = AddonIndex::parse(m_index_page_cache);
      }
      catch (const std::exception& err)
      {
        log_warning << "Couldn't parse requested add-on index '" << url << "': " << err.what() << std::endl;
      }
      m_index_page_cache.clear();
    });
  return status;
}

TransferStatusListPtr
AddonManager::request_upstream_addons()
{
  for (const auto& [_, addon] : m_installed_addons)
  {
    m_transfer_statuses->push(request_upstream_addon(*addon));
  }
  return m_transfer_statuses;
}

TransferStatusPtr
AddonManager::request_upstream_addon(Addon& addon)
{
  TransferStatusPtr status = m_downloader.request_string_download(
    addon.get_upstream_url().empty() ? m_repository_url : addon.get_upstream_url(),
    addon.m_upstream_index_cache
  );
  status->then([&addon](bool success)
    {
      if (!success) return;

      // If no update URL is available for the add-on,
      // search for it in the received index, which includes other add-ons.
      if (addon.get_upstream_url().empty())
      {
        try
        {
          addon.m_upstream_addon = AddonIndex::parse_addon(addon.m_upstream_index_cache, addon.get_id());
          if (!addon.m_upstream_addon)
            throw std::runtime_error("Add-on doesn't exist in the index.");

          addon.m_upstream_index_cache.clear();
        }
        catch (const std::exception& err)
        {
          log_warning << "Unable to find add-on '" << addon.get_id() << "' in upstream add-on index: "
                      << err.what() << std::endl;
          addon.m_upstream_index_cache.clear();
          return;
        }
      }
      else
      {
        try
        {
          addon.m_upstream_addon = Addon::parse_string(addon.m_upstream_index_cache);
          addon.m_upstream_index_cache.clear();
        }
        catch (const std::exception& err)
        {
          log_warning << "Unable to parse upstream add-on '" << addon.get_id()
                      << "' from provided 'upstream-url' '" << addon.get_upstream_url()
                      << "': " << err.what() << std::endl;
          addon.m_upstream_index_cache.clear();
          return;
        }
      }
    });
  return status;
}

TransferStatusListPtr
AddonManager::request_install_addon(const Addon& repository_addon)
{
  // Remove add-on, if it already exists.
  auto it = m_installed_addons.find(repository_addon.get_id());
  if (it != m_installed_addons.end())
  {
    log_debug << "Reinstalling add-on " << repository_addon.get_id() << std::endl;
    if (it->second->m_enabled)
    {
      disable_addon(it->first);
    }
    m_installed_addons.erase(it);
  }
  else
  {
    log_debug << "Installing add-on " << repository_addon.get_id() << std::endl;
  }

  const std::string cache_install_filename = FileSystem::join(m_cache_directory, repository_addon.get_filename());

  // Install add-on dependencies, if any.
  request_install_addon_dependencies(repository_addon);

  // Install the add-on.
  TransferStatusPtr status = m_downloader.request_download(repository_addon.get_url(), cache_install_filename);
  status->then([this, cache_install_filename, &repository_addon](bool success)
    {
      if (!success) return;

      // Complete the add-on installation.
      MD5 md5 = md5_from_file(cache_install_filename);
      if (repository_addon.get_md5() != md5.hex_digest())
      {
        if (PHYSFS_delete(cache_install_filename.c_str()) == 0)
        {
          log_warning << "PHYSFS_delete failed: " << physfsutil::get_last_error() << std::endl;
        }

        throw std::runtime_error("Downloading add-on failed: MD5 checksums differ");
      }

      const std::string install_filename = FileSystem::join(m_addon_directory, repository_addon.get_filename());
      try
      {
        Partio::ZipFileReader zip_reader(FileSystem::join(PHYSFS_getWriteDir(), cache_install_filename));
        Partio::ZipFileWriter zip_writer(FileSystem::join(PHYSFS_getWriteDir(), install_filename));

        zip_writer.Add_Zip_Files(zip_reader);
        *zip_writer.Add_File(repository_addon.get_id() + ".nfo") << repository_addon.write_info();
      }
      catch (const std::exception& err)
      {
        std::stringstream msg;
        msg << "Failed to write info to installed add-on: " << err.what();
        throw std::runtime_error(msg.str());
      }

      if (PHYSFS_delete(cache_install_filename.c_str()) == 0)
      {
        log_warning << "PHYSFS_delete failed: " << physfsutil::get_last_error() << std::endl;
      }

      if (!PHYSFS_getRealDir(install_filename.c_str()))
      {
        throw std::runtime_error("PHYSFS_getRealDir failed: " + install_filename);
      }

      add_installed_archive(install_filename);

      // Attempt to enable the add-on.
      try
      {
        enable_addon(repository_addon.get_id());
      }
      catch (const std::exception& err)
      {
        log_warning << "Enabling add-on failed: " << err.what() << std::endl;
      }
    });
  m_transfer_statuses->push(status);

  return m_transfer_statuses;
}

TransferStatusListPtr
AddonManager::request_install_addon_dependencies(const Addon& addon)
{
  for (const auto& dependency : addon.get_dependencies())
  {
    if (is_addon_installed(dependency->get_id()))
      continue; // Don't attempt to install add-ons that are already installed.

    request_install_addon(*dependency);
  }

  return m_transfer_statuses;
}

void
AddonManager::install_addon_from_local_file(const std::string& filename)
{
  const std::string& source_filename = FileSystem::basename(filename);
  if(!StringUtil::has_suffix(source_filename, ".zip"))
    return;

  const std::string& target_directory = FileSystem::join(PHYSFS_getRealDir(m_addon_directory.c_str()), m_addon_directory);
  const std::string& target_filename = FileSystem::join(target_directory, source_filename);
  const std::string& physfs_target_filename = FileSystem::join(m_addon_directory, source_filename);

  FileSystem::copy(filename, target_filename);
  add_installed_archive(physfs_target_filename, true);
}

void
AddonManager::uninstall_addon(const AddonId& addon_id)
{
  log_debug << "Uninstalling add-on " << addon_id << std::endl;
  auto& addon = get_installed_addon(addon_id);
  if (addon.m_enabled)
  {
    disable_addon(addon_id);
  }
  log_debug << "Deleting file \"" << addon.m_install_filename << "\"" << std::endl;
  const auto it = m_installed_addons.find(addon.get_id());
  if (it != m_installed_addons.end())
  {
    if (PHYSFS_delete(FileSystem::join(m_addon_directory, addon.get_filename()).c_str()) == 0)
    {
      throw std::runtime_error("Error deleting addon .zip file: \"PHYSFS_delete\" failed: " + std::string(physfsutil::get_last_error()));
    }
    m_installed_addons.erase(it);
  }
  else
  {
    throw std::runtime_error("Error uninstalling add-on: Addon with id " + addon_id + " not found.");
  }
}

TransferStatusListPtr
AddonManager::request_download_addon_screenshots(const Addon& addon)
{
  // Create the add-on screenshots cache directory, if it doesn't exist.
  if (!PHYSFS_exists(m_screenshots_cache_directory.c_str()))
  {
    PHYSFS_mkdir(m_screenshots_cache_directory.c_str());
  }

  const std::string& base_url = addon.get_screenshots().base_url;
  const auto& files = addon.get_screenshots().files;
  for (size_t i = 0; i < files.size(); i++)
  {
    const std::string filename = addon.get_id() + "_" + std::to_string(i + 1) + FileSystem::extension(files[i]);
    const std::string filepath = FileSystem::join(m_screenshots_cache_directory, filename);
    if (PHYSFS_exists(filepath.c_str())) continue; // Do not re-download existing screenshots.

    const std::string url = FileSystem::join(base_url, files[i]);

    TransferStatusPtr status;
    try
    {
      status = m_downloader.request_download(url, filepath);
    }
    catch (std::exception& err)
    {
      log_warning << "Error downloading add-on screenshot from URL '" << url
                  << "' to file '" << filename << "': " << err.what() << std::endl;
      continue;
    }

    m_transfer_statuses->push(status);
  }
  return m_transfer_statuses;
}

std::vector<std::string>
AddonManager::get_local_addon_screenshots(const AddonId& addon_id) const
{
  std::vector<std::string> screenshots;
  physfsutil::enumerate_files(m_screenshots_cache_directory, [&screenshots, &addon_id, this](const std::string& filename) {
    // Push any files from the cache directory, starting with the ID of the add-on.
    if (StringUtil::starts_with(filename, addon_id))
    {
      screenshots.push_back(FileSystem::join(m_screenshots_cache_directory, filename));
    }
  });
  return screenshots;
}

void
AddonManager::enable_addon(const AddonId& addon_id)
{
  log_debug << "enabling addon " << addon_id << std::endl;
  auto& addon = get_installed_addon(addon_id);
  if (addon.m_enabled)
  {
    throw std::runtime_error("Tried enabling already enabled add-on.");
  }
  else
  {
    if (addon.get_type() == Addon::RESOURCEPACK)
    {
      for (const auto& [id, installed_addon] : m_installed_addons)
      {
        if (installed_addon->get_type() == Addon::RESOURCEPACK &&
            installed_addon->m_enabled)
        {
          throw std::runtime_error(_("Only one resource pack is allowed to be enabled at a time."));
        }
      }
    }

    // Only mount resource packs on startup (AddonManager initialization).
    if (addon.get_type() == Addon::RESOURCEPACK && m_initialized)
    {
      addon.m_enabled = true;
      return;
    }

    log_debug << "Adding archive \"" << addon.m_install_filename << "\" to search path" << std::endl;
    if (PHYSFS_mount(addon.m_install_filename.c_str(), "/", !addon.overrides_data()) == 0)
    {
      std::stringstream err;
      err << "Could not add " << addon.m_install_filename << " to search path: "
          << physfsutil::get_last_error() << std::endl;
      throw std::runtime_error(err.str());
    }
    else
    {
      if (addon.get_type() == Addon::LANGUAGEPACK)
      {
        PHYSFS_enumerate(addon.get_id().c_str(), add_to_dictionary_path, nullptr);
      }
      addon.m_enabled = true;
    }
  }
}

void
AddonManager::disable_addon(const AddonId& addon_id)
{
  log_debug << "disabling addon " << addon_id << std::endl;
  auto& addon = get_installed_addon(addon_id);
  if (!addon.m_enabled)
  {
    throw std::runtime_error("Tried disabling already disabled add-on.");
  }
  else
  {
    // Don't unmount resource packs. Disabled resource packs will not be mounted on next startup.
    if (addon.get_type() == Addon::RESOURCEPACK)
    {
      addon.m_enabled = false;
      return;
    }

    log_debug << "Removing archive \"" << addon.m_install_filename << "\" from search path" << std::endl;
    if (PHYSFS_unmount(addon.m_install_filename.c_str()) == 0)
    {
      std::stringstream err;
      err << "Could not remove " << addon.m_install_filename << " from search path: "
          << physfsutil::get_last_error() << std::endl;
      throw std::runtime_error(err.str());
    }
    else
    {
      if (addon.get_type() == Addon::LANGUAGEPACK)
      {
        PHYSFS_enumerate(addon.get_id().c_str(), remove_from_dictionary_path, nullptr);
      }
      addon.m_enabled = false;
    }
  }
}

bool
AddonManager::is_addon_installed(const AddonId& addon_id) const
{
  return std::any_of(m_installed_addons.begin(), m_installed_addons.end(),
    [addon_id](const auto& installed_addon_entry) {
      return installed_addon_entry.first == addon_id;
    });
}

std::vector<AddonId>
AddonManager::get_depending_addons(const AddonId& addon_id) const
{
  std::vector<AddonId> addons;
  for (const auto& [installed_addon_id, installed_addon] : m_installed_addons)
  {
    const auto& dependency_ids = installed_addon->get_dependency_ids();
    if (std::find(dependency_ids.begin(), dependency_ids.end(), addon_id) != dependency_ids.end())
      addons.push_back(installed_addon_id);
  }
  return addons;
}

std::vector<std::string>
AddonManager::scan_for_archives() const
{
  std::vector<std::string> archives;

  // Search for archives and add them to the search path.
  physfsutil::enumerate_files(m_addon_directory, [this, &archives](const std::string& filename) {
    const std::string fullpath = FileSystem::join(m_addon_directory, filename);
    if (physfsutil::is_directory(fullpath))
    {
      // ignore dot files (e.g. '.git/'), as well as the addon cache directory.
      if (filename[0] != '.' && fullpath != m_cache_directory) {
        archives.push_back(fullpath);
      }
    }
    else
    {
      if (StringUtil::has_suffix(StringUtil::tolower(filename), ".zip")) {
        if (PHYSFS_exists(fullpath.c_str())) {
          archives.push_back(fullpath);
        }
      }
    }
  });

  return archives;
}

std::string
AddonManager::scan_for_info(const std::string& archive_os_path) const
{
  std::string nfoFilename = "";
  physfsutil::enumerate_files("/", [archive_os_path, &nfoFilename](const std::string& file) {
    if (StringUtil::has_suffix(file, ".nfo"))
    {
      std::string nfo_filename = FileSystem::join("/", file);

      // Make sure it's in the current archive_os_path.
      const char* realdir = PHYSFS_getRealDir(nfo_filename.c_str());
      if (!realdir)
      {
        log_warning << "PHYSFS_getRealDir() failed for " << nfo_filename << ": " << physfsutil::get_last_error() << std::endl;
      }
      else
      {
        if (realdir == archive_os_path)
        {
          nfoFilename = nfo_filename;
        }
      }
    }
  });

  return nfoFilename;
}

void
AddonManager::add_installed_archive(const std::string& archive, bool user_install)
{
  const char* realdir = PHYSFS_getRealDir(archive.c_str());
  if (!realdir)
  {
    log_warning << "PHYSFS_getRealDir() failed for " << archive << ": "
                << physfsutil::get_last_error() << std::endl;
  }
  else
  {
    bool has_error = false;
    std::string os_path = FileSystem::join(realdir, archive);

    PHYSFS_mount(os_path.c_str(), nullptr, 1);

    std::string nfo_filename = scan_for_info(os_path);

    if (nfo_filename.empty())
    {
      log_warning << "Couldn't find .nfo file for " << os_path << std::endl;
      has_error = true;
    }
    else
    {
      try
      {
        std::unique_ptr<Addon> addon = Addon::parse(nfo_filename);
        addon->m_install_filename = os_path;
        const auto& addon_id = addon->get_id();

        try
        {
          get_installed_addon(addon_id);
          if(user_install)
          {
            Dialog::show_message(fmt::format(_("Add-on {} by {} is already installed."),
                                             addon->get_title(), addon->get_author()));
          }
        }
        catch(...)
        {
          // Save add-on title and author on stack before std::move.
          const std::string addon_title = addon->get_title();
          const std::string addon_author = addon->get_author();
          m_installed_addons[addon_id] = std::move(addon);
          if(user_install)
          {
            try
            {
              enable_addon(addon_id);
            }
            catch(const std::exception& err)
            {
              log_warning << "Failed to enable add-on archive '" << addon_id << "': " << err.what() << std::endl;
            }
            Dialog::show_message(fmt::format(_("Add-on {} by {} successfully installed."),
                                             addon_title, addon_author));
            // If currently opened menu is add-ons menu refresh it.
            AddonMenu* addon_menu = dynamic_cast<AddonMenu*>(MenuManager::instance().current_menu());
            if (addon_menu)
              addon_menu->refresh();
          }
        }
      }
      catch (const std::runtime_error& e)
      {
        log_warning << "Could not load add-on info for " << archive << ": " << e.what() << std::endl;
        has_error = true;
      }
    }

    if(!user_install || has_error)
    {
      PHYSFS_unmount(os_path.c_str());
    }
  }
}

void
AddonManager::add_installed_addons()
{
  for (const auto& archive : scan_for_archives())
    add_installed_archive(archive);
}

void
AddonManager::update()
{
  m_downloader.update();
}

/* EOF */
