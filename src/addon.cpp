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

#include "addon.hpp"

#include <physfs.h>

#include <algorithm>
#include <iostream>
#include <sstream>

#include "file_system.hpp"
#include "globals.h"

std::vector<std::unique_ptr<Addon>> g_addons = {};

void
Addon::load_addons()
{
  for (const auto& addon : g_addons)
    addon->unmount();
  g_addons.clear();

  const std::vector<std::string> addon_files = FileSystem::get_files("addons");
  for (const std::string& addon_file : addon_files)
  {
    if (FileSystem::extension(addon_file) != ".zip")
      continue; // Only accept .zip archives

    g_addons.push_back(std::unique_ptr<Addon>(new Addon(addon_file,
        std::find(g_init_enabled_addons.begin(), g_init_enabled_addons.end(), addon_file)
          != g_init_enabled_addons.end())));
  }
}


Addon::Addon(const std::string& filename, bool enabled) :
  m_filename(filename),
  m_enabled(enabled)
{
  if (m_enabled)
    mount();
}

std::string
Addon::get_filepath() const
{
  std::stringstream path;
  path << FileSystem::g_userdir << "addons/" << m_filename;
  return path.str();
}

void
Addon::toggle()
{
  if (m_enabled)
    unmount();
  else
    mount();
}

void
Addon::mount()
{
  std::cout << "Mounting add-on '" << m_filename << "'" << std::endl;

  if (!PHYSFS_mount(get_filepath().c_str(), NULL, 0))
    throw FileSystem::PhysfsError("Failed to mount add-on '" + m_filename + "'", "mount");

  m_enabled = true;
}

void
Addon::unmount()
{
  std::cout << "Unmounting add-on '" << m_filename << "'" << std::endl;

  if (!PHYSFS_unmount(get_filepath().c_str()))
    throw FileSystem::PhysfsError("Failed to unmount add-on '" + m_filename + "'", "unmount");

  m_enabled = false;
}
