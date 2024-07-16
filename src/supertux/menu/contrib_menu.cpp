//  SuperTux
//  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
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

#include "supertux/menu/contrib_menu.hpp"

#include "gui/item_horizontalmenu.hpp"
#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "physfs/util.hpp"
#include "supertux/levelset.hpp"
#include "supertux/menu/sorted_contrib_menu.hpp"
#include "supertux/world.hpp"
#include "util/file_system.hpp"
#include "util/gettext.hpp"
#include "util/log.hpp"

static const std::string ICONS_BASE_DIR = "images/engine/contrib/";

ContribMenu::ContribMenu() :
  m_contrib_worlds()
{
  // Generating contrib levels list by making use of Level Subset.
  std::vector<std::string> level_worlds;
  physfsutil::enumerate_files("levels", [&level_worlds](const std::string& filename) {
    std::string filepath = FileSystem::join("levels", filename);
    if (physfsutil::is_directory(filepath))
    {
      level_worlds.push_back(filepath);
    }
  });
  physfsutil::enumerate_files("custom", [&level_worlds](const std::string& addon_filename) {
    std::string addonpath = FileSystem::join("custom", addon_filename);
    if (physfsutil::is_directory(addonpath))
    {
      std::string addonlevelpath = FileSystem::join(addonpath, "levels");
      if (physfsutil::is_directory(addonlevelpath))
      {
        physfsutil::enumerate_files(addonlevelpath, [addonlevelpath, &level_worlds](const std::string& filename) {
          std::string filepath = FileSystem::join(addonlevelpath, filename);
          if (physfsutil::is_directory(filepath))
          {
            level_worlds.push_back(filepath);
          }
        });
      }
    }
  });

  for (std::vector<std::string>::const_iterator it = level_worlds.begin(); it != level_worlds.end(); ++it)
  {
    try
    {
      auto levelset = std::unique_ptr<Levelset>(new Levelset(*it, /* recursively = */ true));
      if (levelset->get_num_levels() == 0)
        continue;

      std::unique_ptr<World> world = World::from_directory(*it);
      if (!world->hide_from_contribs())
      {
        if (world->is_levelset() || world->is_worldmap())
        {
          m_contrib_worlds.push_back(std::move(world));
        }
        else
        {
          log_warning << "Unknown World type." << std::endl;
        }
      }
    }
    catch (const std::exception& e)
    {
      log_info << "Couldn't parse levelset info for '" << *it << "': " << e.what() << std::endl;
    }
  }

  add_label(_("Contrib Levels"));
  add_hl();

  ItemHorizontalMenu& horizontal_menu = add_horizontalmenu(MNID_CONTRIB_TYPES, 150.f, 100.f);
  horizontal_menu.add_item(_("Official"), "", ICONS_BASE_DIR + "official.png", CONTRIB_OFFICIAL);
  horizontal_menu.add_item(_("Community"), "", ICONS_BASE_DIR + "community.png", CONTRIB_COMMUNITY);
  horizontal_menu.add_item(_("User"), "", ICONS_BASE_DIR + "user.png", CONTRIB_USER);

  add_hl();
  add_back(_("Back"));
}

void
ContribMenu::menu_action(MenuItem& item)
{
  if (item.get_id() != MNID_CONTRIB_TYPES)
    return;

  auto& horizontal_menu = static_cast<ItemHorizontalMenu&>(item);

  std::unique_ptr<SortedContribMenu> contrib_menu;
  switch (horizontal_menu.get_selected_item().id)
  {
    case CONTRIB_OFFICIAL:
    {
      contrib_menu = std::make_unique<SortedContribMenu>(m_contrib_worlds, "official", _("Official Contrib Levels"),
        _("How is this possible? There are no Official Contrib Levels!"));
      break;
    }
    case CONTRIB_COMMUNITY:
    {
      contrib_menu = std::make_unique<SortedContribMenu>(m_contrib_worlds, "community", _("Community Contrib Levels"),
        _("No Community Contrib Levels yet. Download them from the Add-ons Menu."));
      break;
    }
    case CONTRIB_USER:
    {
      contrib_menu = std::make_unique<SortedContribMenu>(m_contrib_worlds, "user", _("User Contrib Levels"),
        _("No User Contrib Levels yet. Create some with the Level Editor."));
      break;
    }
  }
  MenuManager::instance().push_menu(std::move(contrib_menu));
}

/* EOF */
