//  SuperTux - v0.5.1 RNG Mod
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

#ifndef HEADER_SUPERTUX_SUPERTUX_MENU_RNG_SAVESTATES_MENU_HPP
#define HEADER_SUPERTUX_SUPERTUX_MENU_RNG_SAVESTATES_MENU_HPP

#include "gui/menu.hpp"

class RNGSavestatesMenu : public Menu
{
private:
  enum RNGSavestatesMenuIDs
  {
    MNID_SAVESTATE = -2,
    MNID_IMPORTFILE = -3,
    MNID_SAVEFILE = -4
  };

public:
  RNGSavestatesMenu();

  void refresh() override;
  void menu_action(MenuItem* item) override;

private:
  std::string m_states_file;

private:
  RNGSavestatesMenu(const RNGSavestatesMenu&);
  RNGSavestatesMenu& operator=(const RNGSavestatesMenu&);
};

#endif

/* EOF */
