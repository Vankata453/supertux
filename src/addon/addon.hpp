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

#ifndef HEADER_SUPERTUX_ADDON_ADDON_HPP
#define HEADER_SUPERTUX_ADDON_ADDON_HPP

#include <memory>
#include <vector>
#include <string>

class ReaderMapping;

class Addon final
{
  friend class AddonManager;

public:
  static std::unique_ptr<Addon> parse(std::istream& stream);
  static std::unique_ptr<Addon> parse_string(const std::string& str);

  enum Type { WORLD, WORLDMAP, LEVELSET, LANGUAGEPACK, RESOURCEPACK, ADDON };

  struct Version final
  {
    std::string commit;
    std::string title;
    std::string description;
    int64_t created_at;
  };
  struct Screenshots final
  {
    std::string base_url;
    std::vector<std::string> files;
  };

private:
  std::string m_id;
  Version m_version;
  Type m_type;
  std::string m_title;
  std::string m_description;
  std::string m_author;
  std::string m_license;
  std::string m_origin_url;
  std::string m_url;
  std::string m_upstream_url;
  std::string m_md5;
  Screenshots m_screenshots;
  std::vector<std::unique_ptr<Addon>> m_dependencies;
  std::vector<std::string> m_dependency_ids;

  // Fields filled by the AddonManager for installed add-ons
  std::string m_upstream_index_cache;
  std::unique_ptr<Addon> m_upstream_addon;
  std::string m_install_filename;
  bool m_enabled;

public:
  Addon(const ReaderMapping& mapping);

public:
  const std::string& get_id() const { return m_id; }
  const Version& get_version() const { return m_version; }
  Type get_type() const { return m_type; }
  const std::string& get_title() const { return m_title; }
  const std::string& get_description() const { return m_description; }
  const std::string& get_author() const { return m_author; }
  const std::string& get_license() const { return m_license; }
  const std::string& get_origin_url() const { return m_origin_url; }
  const std::string& get_url() const { return m_url; }
  const std::string& get_upstream_url() const { return m_upstream_url; }
  const std::string& get_md5() const { return m_md5; }
  const Screenshots& get_screenshots() const { return m_screenshots; }
  const std::vector<std::unique_ptr<Addon>>& get_dependencies() const { return m_dependencies; }
  const std::vector<std::string>& get_dependency_ids() const { return m_dependency_ids; }

  const Addon* get_upstream_addon() const;
  std::string get_filename() const;
  std::string get_install_filename() const;

  bool is_installed() const;
  bool is_enabled() const;

  bool is_levelset() const;
  bool overrides_data() const;
  bool requires_restart() const;

  bool has_available_update() const;

  std::string write_info() const;

private:
  Addon(const Addon&) = delete;
  Addon& operator=(const Addon&) = delete;
};

namespace addon_string_util
{
  Addon::Type addon_type_from_string(const std::string& type);
  std::string addon_type_to_string(const Addon::Type& type);
  std::string addon_type_to_translated_string(Addon::Type type);

  std::string generate_menu_item_text(const Addon& addon, bool installed);
  std::string get_addon_plural_form(size_t count);
}

#endif

/* EOF */
