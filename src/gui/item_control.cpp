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

#include "gui/item_control.hpp"

ItemControl::ItemControl(std::unique_ptr<InterfaceControl> control) :
  MenuItem(""),
  m_control(std::move(control))
{
}

void
ItemControl::draw(DrawingContext& context, const Vector& pos, int, bool)
{
  m_control->set_pos(Vector(pos.x + 12.f,
                            pos.y - m_control->get_rect().get_height() / 2));
  m_control->draw(context);
}

void
ItemControl::process_action(const MenuAction& action)
{
  switch (action)
  {
    case MenuAction::SELECT:
      m_control->set_focus(true);
      break;
    case MenuAction::UNSELECT:
      m_control->set_focus(false);
      break;
  }
}

void
ItemControl::event(const SDL_Event& ev)
{
  m_control->event(ev);
}

int
ItemControl::get_width() const
{
  return static_cast<int>(m_control->get_rect().get_width());
}

int
ItemControl::get_height() const
{
  return static_cast<int>(m_control->get_rect().get_height());
}

/* EOF */
