//  SuperTux - Add-on
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

#include "addon/addon.hpp"

#include <optional>
#include <fmt/format.h>
#include <sstream>

#include "addon/addon_manager.hpp"
#include "util/gettext.hpp"
#include "util/log.hpp"
#include "util/reader.hpp"
#include "util/reader_document.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"

static const char* s_allowed_characters = "-_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

namespace addon_string_util {

Addon::Type addon_type_from_string(const std::string& type)
{
  if (type == "world")
  {
    return Addon::WORLD;
  }
  else if (type == "worldmap")
  {
    return Addon::WORLDMAP;
  }
  else if (type == "levelset")
  {
    return Addon::LEVELSET;
  }
  else if (type == "languagepack")
  {
    return Addon::LANGUAGEPACK;
  }
  else if (type == "resourcepack")
  {
    return Addon::RESOURCEPACK;
  }
  else if (type == "addon")
  {
    return Addon::ADDON;
  }
  else
  {
    throw std::runtime_error("Not a valid Addon::Type: " + type);
  }
}

std::string addon_type_to_string(const Addon::Type& type)
{
  switch (type)
  {
    case Addon::LEVELSET:
      return "levelset";

    case Addon::WORLDMAP:
      return "worldmap";

    case Addon::WORLD:
      return "world";

    case Addon::LANGUAGEPACK:
      return "languagepack";

    case Addon::RESOURCEPACK:
      return "resourcepack";

    default:
      return "addon";
  }
}

std::string addon_type_to_translated_string(Addon::Type type)
{
  switch (type)
  {
    case Addon::LEVELSET:
      return _("Levelset");

    case Addon::WORLDMAP:
      return _("Worldmap");

    case Addon::WORLD:
      return _("World");

    case Addon::ADDON:
      return _("Add-on");

    case Addon::LANGUAGEPACK:
      return _("Language Pack");

    case Addon::RESOURCEPACK:
      return _("Resource Pack");

    default:
      return _("Unknown");
  }
}

std::string generate_menu_item_text(const Addon& addon, bool installed)
{
  std::string text;
  std::string type = addon_type_to_translated_string(addon.get_type());

  if (!addon.get_author().empty())
  {
    text = fmt::format(fmt::runtime(_("{} \"{}\" by \"{}\"")),
                       type, addon.get_title(), addon.get_author());
  }
  else
  {
    // Only add-on type and name, no need for translation.
    text = fmt::format("{} \"{}\"", type, addon.get_title());
  }

  if (installed)
    text += " " + _("[INSTALLED]");

  return text;
}

std::string get_addon_plural_form(size_t count)
{
  return (count == 1 ? _("add-on") : _("add-ons"));
}

} // namespace addon_string_util

std::unique_ptr<Addon>
Addon::parse(std::istream& stream)
{
  try
  {
    auto doc = ReaderDocument::from_stream(stream);
    auto root = doc.get_root();
    if (root.get_name() != "supertux-addoninfo")
    {
      throw std::runtime_error("Invalid add-on entry: Not a 'supertux-addoninfo' entry.");
    }

    return std::make_unique<Addon>(root.get_mapping());
  }
  catch (const std::exception& err)
  {
    std::stringstream msg;
    msg << "Problem when reading add-on info: " << err.what();
    throw std::runtime_error(msg.str());
  }
}

std::unique_ptr<Addon>
Addon::parse_string(const std::string& str)
{
  try
  {
    auto doc = ReaderDocument::from_string(str);
    auto root = doc.get_root();
    if (root.get_name() != "supertux-addoninfo")
    {
      throw std::runtime_error("Invalid add-on entry: Not a 'supertux-addoninfo' entry.");
    }

    return std::make_unique<Addon>(root.get_mapping());
  }
  catch (const std::exception& err)
  {
    std::stringstream msg;
    msg << "Problem when reading add-on info: " << err.what();
    throw std::runtime_error(msg.str());
  }
}

Addon::Addon(const ReaderMapping& mapping) :
  m_id(),
  m_version(),
  m_type(),
  m_title(),
  m_description(),
  m_author(),
  m_license(),
  m_origin_url(),
  m_url(),
  m_upstream_url(),
  m_md5(),
  m_screenshots(),
  m_dependencies(),
  m_dependency_ids(),
  m_upstream_index_cache(),
  m_upstream_addon(),
  m_install_filename(),
  m_enabled(false)
{
  try
  {
    if (!mapping.get("id", m_id))
    {
      throw std::runtime_error("(id ...) field missing from add-on description!");
    }

    if (m_id.empty())
    {
      throw std::runtime_error("Add-on ID is empty!");
    }

    if (m_id.find_first_not_of(s_allowed_characters) != std::string::npos)
    {
      throw std::runtime_error("Add-on ID contains illegal characters: " + m_id);
    }

    bool has_version_info = false;
    try
    {
      std::optional<ReaderMapping> version_mapping;
      has_version_info = mapping.get("version", version_mapping);
      if (has_version_info)
      {
        version_mapping->get("commit", m_version.commit);
        version_mapping->get("title", m_version.title);
        version_mapping->get("description", m_version.description);
        version_mapping->get("created-at", m_version.created_at);
      }
    }
    catch (const std::exception&)
    {
      int legacy_version = 1;
      has_version_info = mapping.get("version", legacy_version);
      if (has_version_info)
      {
        log_warning << "Add-on '" << m_id << "' has legacy 'version' info." << std::endl;

        m_version.commit = std::to_string(legacy_version);
        m_version.title = "v" + m_version.commit;
        m_version.created_at = 0;
      }
    }
    if (!has_version_info)
    {
      throw std::runtime_error("Add-on '" + m_id + "' has no 'version' info!");
    }

    std::string type;
    mapping.get("type", type);
    m_type = addon_string_util::addon_type_from_string(type);

    mapping.get("title", m_title);
    mapping.get("description", m_description);
    mapping.get("author", m_author);
    mapping.get("license", m_license);
    mapping.get("origin-url", m_origin_url);
    mapping.get("url", m_url);
    mapping.get("upstream-url", m_upstream_url);
    mapping.get("md5", m_md5);

    std::optional<ReaderMapping> screenshots_mapping;
    if (mapping.get("screenshots", screenshots_mapping))
    {
      screenshots_mapping->get("base-url", m_screenshots.base_url);

      std::optional<ReaderMapping> files_mapping;
      if (screenshots_mapping->get("files", files_mapping))
      {
        auto iter = files_mapping->get_iter();
        while (iter.next())
        {
          if (iter.get_key() != "file")
            throw std::runtime_error("Invalid field '" + iter.get_key() + "': " + m_id);

          std::string file;
          iter.get(file);
          m_screenshots.files.push_back(std::move(file));
        }
      }
    }

    std::optional<ReaderMapping> dependencies_mapping;
    if (mapping.get("dependencies", dependencies_mapping))
    {
      auto iter = dependencies_mapping->get_iter();
      while (iter.next())
      {
        if (iter.get_key() != "dependency")
          throw std::runtime_error("Invalid field '" + iter.get_key() + "': " + m_id);

        try
        {
          auto dependency = std::make_unique<Addon>(iter.as_mapping());
          m_dependency_ids.push_back(dependency->get_id());
          m_dependencies.push_back(std::move(dependency));
        }
        catch (const std::exception& err)
        {
          std::stringstream msg;
          msg << "Problem when reading add-on dependency info: " << err.what();
          throw std::runtime_error(msg.str());
        }
      }
    }
    else if (mapping.get("dependency-ids", dependencies_mapping))
    {
      auto iter = dependencies_mapping->get_iter();
      while (iter.next())
      {
        if (iter.get_key() != "dependency")
          throw std::runtime_error("Invalid field '" + iter.get_key() + "': " + m_id);

        std::string id;
        iter.get(id);
        m_dependency_ids.push_back(std::move(id));
      }
    }
  }
  catch (const std::exception& err)
  {
    std::stringstream msg;
    msg << "Problem when parsing add-on info: " << err.what();
    throw std::runtime_error(msg.str());
  }
}

const Addon*
Addon::get_upstream_addon() const
{
  return AddonManager::current()->get_installed_addon(m_id).m_upstream_addon.get();
}

std::string
Addon::get_filename() const
{
  return get_id() + ".zip";
}

std::string
Addon::get_install_filename() const
{
  return AddonManager::current()->get_installed_addon(m_id).m_install_filename;
}

bool
Addon::is_installed() const
{
  return AddonManager::current()->is_addon_installed(m_id);
}

bool
Addon::is_enabled() const
{
  return AddonManager::current()->get_installed_addon(m_id).m_enabled;
}

bool
Addon::is_levelset() const
{
  // Determines if the add-on is a levelset.
  return m_type == WORLD || m_type == WORLDMAP || m_type == LEVELSET;
}

bool
Addon::overrides_data() const
{
  // Determines if the add-on should override game data.
  return m_type == RESOURCEPACK;
}

bool
Addon::requires_restart() const
{
  // Determines if the add-on requires a restart.
  return m_type == LANGUAGEPACK || m_type == RESOURCEPACK;
}

bool
Addon::has_available_update() const
{
  const Addon* upstream_addon = get_upstream_addon();
  return upstream_addon &&
         upstream_addon->get_version().commit != m_version.commit &&
         upstream_addon->get_version().created_at > m_version.created_at;
}

std::string
Addon::write_info() const
{
  std::stringstream stream;
  Writer info(stream);

  info.start_list("supertux-addoninfo");
  {
    info.write("id", m_id);

    info.start_list("version");
    {
      info.write("commit", m_version.commit);
      info.write("title", m_version.title);
      info.write("description", m_version.description);
      info.write("created-at", m_version.created_at);
    }
    info.end_list("version");

    info.write("type", addon_string_util::addon_type_to_string(m_type));
    info.write("title", m_title);
    info.write("description", m_description);
    info.write("author", m_author);
    info.write("license", m_license);
    info.write("origin-url", m_origin_url);
    info.write("upstream-url", m_upstream_url);

    info.start_list("screenshots");
    {
      info.write("base-url", m_screenshots.base_url);

      info.start_list("files");
      for (const std::string& file : m_screenshots.files)
      {
        info.write("file", file);
      }
      info.end_list("files");
    }
    info.end_list("screenshots");

    info.start_list("dependency-ids");
    for (const auto& dependency : m_dependencies)
    {
      info.write("dependency", dependency->get_id());
    }
    info.end_list("dependency-ids");
  }
  info.end_list("supertux-addoninfo");

  return stream.str();
}

/* EOF */
