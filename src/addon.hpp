//  Add-on
//
//  SuperTux
//  Copyright (C) 2023 Vankata453
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#ifndef SUPERTUX_ADDON_HPP
#define SUPERTUX_ADDON_HPP

#include <string>
#include <vector>
#include <memory>

class Addon
{
public:
  static void load_addons();

private:
  Addon(const std::string& filename, bool enabled);

  std::string get_filepath() const;

public:
  void toggle();

  const std::string& get_filename() const { return m_filename; }
  bool is_enabled() const { return m_enabled; }

private:
  void mount();
  void unmount();

private:
  const std::string m_filename;
  bool m_enabled;

private:
  Addon(const Addon&) = delete;
  Addon& operator=(const Addon&) = delete;
};

extern std::vector<std::unique_ptr<Addon>> g_addons;

#endif /*SUPERTUX_ADDON_HPP*/
