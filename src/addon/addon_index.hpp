//  SuperTux
//  Copyright (C) 2024 Vankata453
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

#ifndef HEADER_SUPERTUX_ADDON_ADDON_INDEX_HPP
#define HEADER_SUPERTUX_ADDON_ADDON_INDEX_HPP

#include <memory>
#include <string>
#include <vector>

#include "addon/addon.hpp"

class ReaderMapping;

class AddonIndex final
{
public:
  static std::unique_ptr<AddonIndex> parse(const std::string& index);

  /** Finds and parses the add-on with the provided ID from a full add-on index. */
  static std::unique_ptr<Addon> parse_addon(const std::string& index, const std::string& addon_id);

public:
  AddonIndex(const ReaderMapping& mapping);

  const std::vector<std::unique_ptr<Addon>>& get_addons() const { return m_addons; }

  const std::string& get_previous_page_url() const { return m_previous_page_url; }
  const std::string& get_next_page_url() const { return m_next_page_url; }
  int get_total_pages() const { return m_total_pages; }

private:
  std::vector<std::unique_ptr<Addon>> m_addons;

  std::string m_previous_page_url;
  std::string m_next_page_url;
  int m_total_pages;

private:
  AddonIndex(const AddonIndex&) = delete;
  AddonIndex& operator=(const AddonIndex&) = delete;
};

#endif

/* EOF */
