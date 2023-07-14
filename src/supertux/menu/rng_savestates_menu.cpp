//  SuperTux
//  Copyright (C) 2023 Vankata453
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

#include "supertux/menu/rng_savestates_menu.hpp"

#include <sstream>

#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "math/random_generator.hpp"
#include "util/gettext.hpp"
#include "util/log.hpp"

RNGSavestatesMenu::RNGSavestatesMenu()
{
  refresh();
}

void
RNGSavestatesMenu::refresh()
{
  clear();

  add_label(_("Randomization Savestates"));
  add_hl();

  if (RandomGenerator::s_rng_savestates.empty())
  {
    add_inactive(_("Empty"));
  }
  else
  {
    int index = 0;
    for (auto& rand : RandomGenerator::s_rng_savestates)
    {
      std::stringstream text;
      text << rand->get_seed() << "; " << rand->get_rand_count() << " randomizations performed";

      add_entry(index, text.str());
      index++;
    }
  }
  add_hl();

  add_entry(MNID_SAVESTATE, _("Save current state"));
  add_hl();

  add_back(_("Back"));

  set_active_item(MNID_SAVESTATE);
}

void
RNGSavestatesMenu::menu_action(MenuItem* item)
{
  if (item->id == MNID_SAVESTATE)
  {
    RandomGenerator::s_rng_savestates.push_back(std::unique_ptr<RandomGenerator>(new RandomGenerator(gameRandom)));
    refresh();
  }
  else if (item->id >= 0)
  {
    gameRandom = *RandomGenerator::s_rng_savestates.at(item->id);
    MenuManager::instance().pop_menu();
  }
  else
  {
    log_warning << "Invalid menu item with ID " << item->id << " pressed." << std::endl;
  }
}

/* EOF */
