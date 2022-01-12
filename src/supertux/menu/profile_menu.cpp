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
#include "gui/menu_storage.hpp"
#include "gui/menu_item.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "util/file_system.hpp"
#include "util/gettext.hpp"

#include <physfs.h>

ProfileMenu::ProfileMenu()
{
  std::vector<std::string> userdata_directories = FileSystem::get_subfolder_names("/");
  std::string selected_profile;

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
  if (userdata_directories.size() > 4) add_hl();
  for (std::size_t i = 0; i < userdata_directories.size(); ++i)
  {
    std::string folder_name = userdata_directories[i];
    if (folder_name != "addons" && folder_name != "levels" && folder_name != "screenshots" && folder_name != "default")
    {
      std::ostringstream out;
      if (folder_name == g_config->profile)
      {
        out << str(boost::format(_("[%s]")) %folder_name);
        selected_profile = folder_name;
      }
      else
      {
        out << str(boost::format(_("%s")) %folder_name);
      }
      add_entry(1, out.str());
    }
  }
  add_hl();
  add_entry(2, _("Add profile"));
  add_hl();
  if (selected_profile == "default")
  {
    add_entry(3, _("Reset profile"));
  }
  else
  {
    add_entry(3, _("Delete profile"));
    add_entry(4, _("Delete all profiles"));
  }

  add_hl();
  add_back(_("Back"));
}

void
ProfileMenu::menu_action(MenuItem& item)
{
  const auto& id = item.get_id();
  if (id == 1)
  {
    g_config->profile = item.get_text();
  }
  else if (id == 2)
  {
    MenuManager::instance().push_menu(MenuStorage::PROFILE_CREATE_MENU);
  }
  else if (id == 3)
  {
    std::string conf_msg;
    if (item.get_text() == "default") 
    {
      conf_msg = "Resetting your profile will delete your game progress. Are you sure?";
    }
    else
    {
      conf_msg = "Deleting your profile will reset your game progress. Are you sure?";
    }
    Dialog::show_confirmation(_(conf_msg), [this]() {
      delete_savegames(g_config->profile);
      g_config->profile = "default";
    });
  }
  else if (id == 4)
  {
    Dialog::show_confirmation(_("This will delete all of your profiles and game progress on them. Are you sure?"), [this]() {
      std::vector<std::string> userdata_directories = FileSystem::get_subfolder_names("/");
      for (std::size_t i = 0; i < userdata_directories.size(); ++i)
      {
        std::string folder_name = userdata_directories[i];
        if (folder_name != "addons" && folder_name != "levels" && folder_name != "screenshots")
        {
          delete_savegames(folder_name);
        }
      }
      g_config->profile = "default";
    });
  }
  MenuManager::instance().clear_menu_stack();
}

void
ProfileMenu::delete_savegames(std::string name) const
{
  const auto& profile_path = std::to_string(name);
  std::unique_ptr<char*, decltype(&PHYSFS_freeList)>
    files(PHYSFS_enumerateFiles(profile_path.c_str()),
          PHYSFS_freeList);
  for (const char* const* filename = files.get(); *filename != nullptr; ++filename)
  {
    std::string filepath = FileSystem::join(profile_path.c_str(), *filename);
    PHYSFS_delete(filepath.c_str());
  }
  PHYSFS_delete(profile_path.c_str());

  //If the default profile was deleted, recreate it
  if (name == "default") PHYSFS_mkdir(profile_path.c_str());
}

/* EOF */
