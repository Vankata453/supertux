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

#ifndef HEADER_SUPERTUX_GUI_MENU_SELECT_ITEM_HPP
#define HEADER_SUPERTUX_GUI_MENU_SELECT_ITEM_HPP

#include <functional>

#include "gui/menu.hpp"

class MenuSelectItem : public Menu
{
private:
  typedef std::function<void(int)> SelectedItemCallback;

private:
  SelectedItemCallback m_callback;

public:
  MenuSelectItem(std::vector<std::string>& select_items,
                  SelectedItemCallback callback);

  void change_label(std::string label);

  void menu_action(MenuItem* item) override;

private:
  MenuSelectItem(const MenuSelectItem&);
  MenuSelectItem& operator=(const MenuSelectItem&);
};

#endif

/* EOF */
