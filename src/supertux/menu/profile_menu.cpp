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
  std::vector<std::string> profile_directories;

  char **rc = PHYSFS_enumerateFiles("profiles");
  char **i;
  for (i = rc; *i != NULL; i++)
    profile_directories.push_back(*i);
  PHYSFS_freeList(rc);

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
  for (std::size_t i = 0; i < profile_directories.size(); ++i)
  {
    std::string folder_name = profile_directories[i];
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
  if (g_config->profile != "default")
  {
    add_hl();
    add_entry(4, _("Delete profile"));
    add_entry(5, _("Delete all profiles"));
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
    std::string profile_name = item.get_text();
    if (profile_name[0] == '[' && profile_name[profile_name.size() - 1] == ']')
    {
      profile_name.erase(0, 1);
      profile_name.erase(profile_name.size() - 1);
    }
    g_config->profile = profile_name;
  }
  else if (id == 2 || id == 4)
  {
    std::string conf_msg;
    if (g_config->profile == "default") 
    {
      conf_msg = "Resetting your profile will delete your game progress. Are you sure?";
    }
    else
    {
      conf_msg = "Deleting your profile will reset your game progress. Are you sure?";
    }
    Dialog::show_confirmation(_(conf_msg), [this, id]() {
      delete_savegames(g_config->profile, id == 2);
      g_config->profile = "default";
    });
  }
  else if (id == 3 || id == 5)
  {
    Dialog::show_confirmation(_("This will delete all of your profiles and game progress on them. Are you sure?"), [this, id]() {
      std::vector<std::string> profile_directories;

      char **rc = PHYSFS_enumerateFiles("profiles");
      char **i;
      for (i = rc; *i != NULL; i++)
        profile_directories.push_back(*i);
      PHYSFS_freeList(rc);

      for (std::size_t i = 0; i < profile_directories.size(); ++i)
      {
        std::string folder_name = profile_directories[i];
        delete_savegames(folder_name, id == 3);
      }
      g_config->profile = "default";
    });
  }
  else
  {
    return;
  }
  
  MenuManager::instance().clear_menu_stack();
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
