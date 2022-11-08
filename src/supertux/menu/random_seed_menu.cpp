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

#include "supertux/menu/random_seed_menu.hpp"

#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "math/random_generator.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "util/gettext.hpp"

RandomSeedMenu::RandomSeedMenu() :
  m_seed(g_config->random_seed)
{
  add_label(_("Enter new seed"));
  add_hl();

  add_intfield(_("Seed"), &m_seed);

  add_entry(1, _("OK"));

  add_hl();
  add_back(_("Back"));
}

void
RandomSeedMenu::menu_action(MenuItem* item)
{
  if (item->id != 1) return;

  g_config->random_seed = gameRandom.srand(m_seed);
  g_config->random_seed_history.push_back(g_config->random_seed);
  while (g_config->random_seed_history.size() > 100)
    g_config->random_seed_history.erase(g_config->random_seed_history.begin());

  MenuManager::instance().pop_menu();
}

/* EOF */
