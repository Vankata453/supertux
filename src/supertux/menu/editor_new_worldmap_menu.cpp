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

#include "supertux/menu/editor_new_worldmap_menu.hpp"

#include <physfs.h>
#include <regex>

#include "editor/editor.hpp"
#include "gui/dialog.hpp"
#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "supertux/level.hpp"
#include "supertux/level_parser.hpp"
#include "supertux/menu/menu_storage.hpp"
#include "supertux/world.hpp"
#include "util/file_system.hpp"
#include "util/gettext.hpp"

EditorNewWorldmapMenu::EditorNewWorldmapMenu() :
  m_worldmap_name()
{
  add_label(_("New Worldmap"));
  add_hl();

  add_textfield(_("Name"), &m_worldmap_name)
    .set_help(_("Worldmap names should only contain\nlowercase letters, numbers, dashes and underscores."));;

  add_entry(1,_("Create"));

  add_hl();
  add_back(_("Back"));
}

void
EditorNewWorldmapMenu::menu_action(MenuItem& item)
{
  if (item.get_id() <= 0)
    return;

  if (m_worldmap_name.empty())
  {
    Dialog::show_message(_("Please enter a name for this level subset."));
    return;
  }

  if (!std::regex_match(m_worldmap_name, std::regex("^[A-Za-z0-9\\-\\_]+$")))
  {
    Dialog::show_message(_("Worldmap names should only contain\nlowercase letters, numbers, dashes and underscores.\nPlease choose a different name."));
    return;
  }

  const auto world = Editor::current()->get_world();
  const std::string basedir = world->get_basedir();
  const std::string worldmap_path = FileSystem::join(basedir, m_worldmap_name + ".stwm");

  if (PHYSFS_exists(worldmap_path.c_str()))
  {
    Dialog::show_message(_("A worldmap with this name already exists."));
    return;
  }
  else
  {
    const auto new_worldmap = LevelParser::from_nothing_worldmap(basedir, world->get_title(), m_worldmap_name);
    new_worldmap->save(basedir + "/" + new_worldmap->m_filename);
    Editor::current()->set_level(new_worldmap->m_filename);
    Editor::current()->show_level_license_info();
    MenuManager::instance().clear_menu_stack();
  }
}

/* EOF */
