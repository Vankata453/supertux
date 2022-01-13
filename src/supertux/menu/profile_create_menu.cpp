//  SuperTux
//  Copyright (C) 2022 Vankata453
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

#include "supertux/menu/profile_create_menu.hpp"

#include "editor/editor.hpp"
#include "gui/dialog.hpp"
#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "supertux/menu/menu_storage.hpp"
#include "supertux/world.hpp"
#include "util/file_system.hpp"
#include "util/gettext.hpp"

#include <physfs.h>

ProfileCreateMenu::ProfileCreateMenu() :
  profile_name()
{
  add_label(_("Add profile"));
  add_hl();

  add_textfield(_("Name"), &profile_name);

  add_entry(1, _("Create"));

  add_hl();
  add_back(_("Back"));
}

void
ProfileCreateMenu::menu_action(MenuItem& item)
{
  if (item.get_id() <= 0)
    return;

  if (profile_name.empty())
  {
    Dialog::show_message(_("Please enter a name for the profile."));
    return;
  }

  if (profile_name.size() > 20)
  {
    Dialog::show_message(_("Profile names must have a maximum of 20 characters.\nPlease choose a different name."));
    return;
  }

  if (PHYSFS_exists(profile_name.c_str()))
  {
    Dialog::show_message(_("A profile with this name currently exists, or there was an error creating it.\nPlease choose a different name."));
    return;
  }

  std::string profile_path = "profiles/" + profile_name;
  PHYSFS_mkdir(profile_path.c_str());
  g_config->profile = profile_name;

  MenuManager::instance().pop_menu();
}

/* EOF */
