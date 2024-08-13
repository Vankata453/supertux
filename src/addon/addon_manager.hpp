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

#ifndef HEADER_SUPERTUX_ADDON_ADDON_MANAGER_HPP
#define HEADER_SUPERTUX_ADDON_ADDON_MANAGER_HPP

#include <vector>

#include "addon/addon_map.hpp"
#include "addon/downloader.hpp"
#include "supertux/gameconfig.hpp"
#include "util/currenton.hpp"

class AddonIndex;

/** Checks for, installs and removes Add-ons */
class AddonManager final : public Currenton<AddonManager>
{
public:
  AddonManager(const std::string& addon_directory,
               std::vector<Config::Addon>& addon_config);
  ~AddonManager() override;

  void empty_cache_directory();

  void update();

  TransferStatusPtr request_index(std::unique_ptr<AddonIndex>& index, int limit);
  TransferStatusPtr request_index_page(std::unique_ptr<AddonIndex>& index, bool next_page);
  TransferStatusListPtr request_upstream_addons();

  std::vector<AddonId> get_installed_addons() const;
  Addon& get_installed_addon(const AddonId& addon) const;

  TransferStatusListPtr request_install_addon(const Addon& repository_addon);
  TransferStatusListPtr request_install_addon_dependencies(const Addon& repository_addon);
  void uninstall_addon(const AddonId& addon_id);
  void install_addon_from_local_file(const std::string& filename);

  TransferStatusListPtr request_download_addon_screenshots(const Addon& addon);
  std::vector<std::string> get_local_addon_screenshots(const AddonId& addon_id) const;

  void enable_addon(const AddonId& addon_id);
  void disable_addon(const AddonId& addon_id);

  bool is_addon_installed(const AddonId& addon_id) const;

  std::vector<AddonId> get_depending_addons(const AddonId& id) const;

private:
  TransferStatusPtr request_index(std::unique_ptr<AddonIndex>& index, const std::string& url);
  TransferStatusPtr request_upstream_addon(Addon& addon);

  std::vector<std::string> scan_for_archives() const;
  void add_installed_addons();

  /** add \a archive, given as physfs path, to the list of installed
      archives */
  void add_installed_archive(const std::string& archive, bool user_install = false);

  /** search for an .nfo file in the top level directory that
      originates from \a archive, \a archive is a OS path */
  std::string scan_for_info(const std::string& archive) const;

private:
  Downloader m_downloader;
  const std::string m_addon_directory;
  const std::string m_cache_directory;
  const std::string m_screenshots_cache_directory;
  std::string m_repository_url;
  std::vector<Config::Addon>& m_addon_config;

  AddonMap m_installed_addons;

  bool m_initialized;

  TransferStatusListPtr m_transfer_statuses;

  std::string m_index_page_cache;

private:
  AddonManager(const AddonManager&) = delete;
  AddonManager& operator=(const AddonManager&) = delete;
};

#endif

/* EOF */
