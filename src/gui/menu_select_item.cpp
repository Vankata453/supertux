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

#include "gui/menu_select_item.hpp"

#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "util/gettext.hpp"

MenuSelectItem::MenuSelectItem(std::vector<std::string>& select_items,
                               MenuSelectItem::SelectedItemCallback callback) :
  m_callback(std::move(callback))
{
  add_label(_("Choose Item"));
  add_hl();

  for (int i = 0; i < static_cast<int>(select_items.size()); i++)
    add_entry(i, select_items[i]);

  add_hl();
  add_back(_("Cancel"));
}

void
MenuSelectItem::change_label(std::string label)
{
  get_item(0).change_text(label);
}

void
MenuSelectItem::menu_action(MenuItem* item)
{
  if (item->id < 0) return;

  m_callback(item->id);
  MenuManager::instance().pop_menu();
}

/* EOF */
