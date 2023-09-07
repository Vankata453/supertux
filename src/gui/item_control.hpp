//  SuperTux
//  Copyright (C) 2015 Hume2 <teratux.mail@gmail.com>
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

#ifndef HEADER_SUPERTUX_GUI_ITEM_CONTROL_HPP
#define HEADER_SUPERTUX_GUI_ITEM_CONTROL_HPP

#include "gui/menu_item.hpp"

#include "interface/control.hpp"

/** An item, which manages an interface control. */
class ItemControl final : public MenuItem
{
public:
  ItemControl(std::unique_ptr<InterfaceControl> control);

  void draw(DrawingContext& context, const Vector& pos, int, bool) override;

  void process_action(const MenuAction& action) override;
  void event(const SDL_Event& ev) override;

  int get_width() const override;
  int get_height() const override;

  bool select_blink() const { return false; }

private:
  std::unique_ptr<InterfaceControl> m_control;

private:
  ItemControl(const ItemControl&) = delete;
  ItemControl& operator=(const ItemControl&) = delete;
};

#endif

/* EOF */
