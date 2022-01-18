//  SuperTux
//  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include "supertux/menu/profile_menu.hpp"

#include <boost/format.hpp>
#include <sstream>
#include <vector>

#include "gui/dialog.hpp"
#include "gui/menu_manager.hpp"
#include "supertux/menu/menu_storage.hpp"
#include "gui/menu_item.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "util/file_system.hpp"
#include "util/gettext.hpp"
#include "physfs/util.hpp"

#include <physfs.h>

ProfileMenu::ProfileMenu()
{
  std::vector<std::string> profile_directories = get_savegames();

  add_label(_("Select Profile"));
  add_hl();
  if (g_config->profile == "default")
  {
    add_entry(1, _("[default]"));
  }
  else
  {
    add_entry(1, _("default"));
  }
  for (std::size_t y = 0; y < profile_directories.size(); ++y)
  {
    std::string folder_name = profile_directories[y];
    if (folder_name != "default")
    {
      std::ostringstream out;
      if (folder_name == g_config->profile)
      {
        out << str(boost::format(_("[%s]")) %folder_name);
      }
      else
      {
        out << str(boost::format(_("%s")) %folder_name);
      }
      add_entry(1, out.str());
    }
  }
  add_hl();
  add_submenu(_("Add profile"), MenuStorage::PROFILE_CREATE_MENU);
  add_hl();
  add_entry(2, _("Reset profile"));
  add_entry(3, _("Reset all profiles"));
  add_hl();
  if (g_config->profile != "default")
    add_entry(4, _("Delete profile"));
  add_entry(5, _("Delete all profiles"));

  add_hl();
  add_back(_("Back"));
}

void
ProfileMenu::menu_action(MenuItem& item)
{
  const auto& id = item.get_id();
    
  if (id == 1)
  {
    std::string profile_name = item.get_text();
    //If the selected item's name is equal to the one of the current profile with '[' and ']' from both sides,
    //the currently selected profile is being selected again, so don't do anything.
    if ('[' + g_config->profile + ']' != profile_name)
      g_config->profile = profile_name;
  }
  else if (id == 2 || id == 4)
  {
    const bool is_reset = id == 2;
    std::string conf_msg = is_reset?_("Resetting your profile will delete all of your game progress on it.\nAre you sure?") :
      _("This will delete your profile, wiping all game progress on it and removing it from the list.\nAre you sure?");
    Dialog::show_confirmation(conf_msg, [this, is_reset]() {
      delete_savegames(g_config->profile, is_reset);
      if (!is_reset) g_config->profile = "default";
    });
  }
  else if (id == 3 || id == 5)
  {
    const bool is_reset = id == 3;
    std::string conf_msg = is_reset?_("This will reset all of your profiles and game progress on them.\nAre you sure?") :
      _("This will delete all of your profiles, wiping all game progress on them and removing them from the list.\nAre you sure?");
    Dialog::show_confirmation(conf_msg, [this, is_reset]() {
      std::vector<std::string> profile_directories = get_savegames();

      for (std::size_t y = 0; y < profile_directories.size(); ++y)
      {
        std::string folder_name = profile_directories[y];
        delete_savegames(folder_name, is_reset);
      }
      if (!is_reset) g_config->profile = "default";
    });
  }
  else
  {
    return;
  }
  
  MenuManager::instance().clear_menu_stack();
}

std::vector<std::string>
ProfileMenu::get_savegames()
{
  std::vector<std::string> savegames;
  char **rc = PHYSFS_enumerateFiles("profiles");
  char **i;
  for (i = rc; *i != nullptr; i++)
    savegames.push_back(*i);
  PHYSFS_freeList(rc);
  return savegames;
}

void
ProfileMenu::delete_savegames(std::string profile_name, bool recreate) const
{
  const std::string profile_path = "profiles/" + profile_name;
  std::unique_ptr<char*, decltype(&PHYSFS_freeList)>
    files(PHYSFS_enumerateFiles(profile_path.c_str()),
          PHYSFS_freeList);
  for (const char* const* filename = files.get(); *filename != nullptr; ++filename)
  {
    std::string filepath = FileSystem::join(profile_path.c_str(), *filename);
    PHYSFS_delete(filepath.c_str());
  }
  PHYSFS_delete(profile_path.c_str());

  if (recreate) PHYSFS_mkdir(profile_path.c_str());
}

/* EOF */
