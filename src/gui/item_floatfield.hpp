//  SuperTux
//  Copyright (C) 2015 Hume2 <teratux.mail@gmail.com>
//                2022 Vankata453
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

#ifndef HEADER_SUPERTUX_GUI_ITEM_FLOATFIELD_HPP
#define HEADER_SUPERTUX_GUI_ITEM_FLOATFIELD_HPP

#include "gui/item_textfield.hpp"

class ItemFloatField final : public ItemTextField
{
public:
  ItemFloatField(const std::string& text_, float* input_, int id_ = -1);
  ~ItemFloatField() override;

  float* number;

  /** Processes the given custom event cases. */
  virtual bool custom_event(const SDL_Event& ev) override;

  /** Processes the menu action. */
  virtual void process_action(const MenuAction& action) override;

private:
  std::string m_input;
  bool m_has_comma;

  void add_char(char c);

private:
  ItemFloatField(const ItemFloatField&) = delete;
  ItemFloatField& operator=(const ItemFloatField&) = delete;
};

#endif

/* EOF */
