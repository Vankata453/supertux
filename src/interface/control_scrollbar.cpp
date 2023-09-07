//  SuperTux
//  Copyright (C) 2020 A. Semphris <semphris@protonmail.com>
//                2023 Vankata453
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

#include "interface/control_scrollbar.hpp"

#include <math.h>

#include "editor/editor.hpp"
#include "video/drawing_context.hpp"
#include "video/renderer.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"

ControlScrollbar::ControlScrollbar(const float& total_region, const float& covered_region,
                                   float& progress, float mouse_wheel_speed) :
  m_mouse_wheel_speed(mouse_wheel_speed),
  m_scrolling(),
  m_hovering(),
  m_total_region(total_region),
  m_covered_region(covered_region),
  m_progress(progress),
  m_rect(),
  //is_horizontal(),
  last_mouse_pos()
  //zoom_factor()
{
  if (m_total_region < m_covered_region)
    m_total_region = m_covered_region;
}

void
ControlScrollbar::draw(DrawingContext& context)
{
  context.color().draw_filled_rect(get_bar_rect(),
                                   Color(1.f, 1.f, 1.f, (m_hovering || m_scrolling) ? 1.f : 0.5f),
                                   8,
                                   LAYER_GUI);
/*
  context.color().draw_filled_rect(Rectf(Vector(0, 0), Vector(SIZE, SIZE)),
                                     Color(0.9f, 0.9f, 1.0f, 0.6f),
                                     MIDDLE, LAYER_GUI-10);
  context.color().draw_filled_rect(Rectf(Vector(40, 40), Vector(56, 56)),
                                     Color(0.9f, 0.9f, 1.0f, 0.6f),
                                     8, LAYER_GUI-20);
  if (can_scroll()) {
    draw_arrow(context, m_mouse_pos);
  }

  draw_arrow(context, Vector(TOPLEFT, MIDDLE));
  draw_arrow(context, Vector(BOTTOMRIGHT, MIDDLE));
  draw_arrow(context, Vector(MIDDLE, TOPLEFT));
  draw_arrow(context, Vector(MIDDLE, BOTTOMRIGHT));
*/
}

void
ControlScrollbar::update(float dt_sec)
{
  
}

bool
ControlScrollbar::on_mouse_button_up(const SDL_MouseButtonEvent& button)
{
  m_scrolling = false;
  return false;
}

bool
ControlScrollbar::on_mouse_button_down(const SDL_MouseButtonEvent& button)
{
  if (button.button == SDL_BUTTON_LEFT) {
    Vector mouse_pos = VideoSystem::current()->get_viewport().to_logical(button.x, button.y);
    if (get_bar_rect().contains(mouse_pos)) {
      m_scrolling = true;
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool
ControlScrollbar::on_mouse_motion(const SDL_MouseMotionEvent& motion)
{
  //InterfaceControl::on_mouse_motion(motion);

  Vector mouse_pos = VideoSystem::current()->get_viewport().to_logical(motion.x, motion.y);

  /*if (mouse_pos.x < SIZE && m_mouse_pos.y < SIZE) {
    m_scrolling_vec = m_mouse_pos - Vector(MIDDLE, MIDDLE);
    if (m_scrolling_vec.x != 0 || m_scrolling_vec.y != 0) {
      float norm = m_scrolling_vec.norm();
      m_scrolling_vec *= powf(static_cast<float>(M_E), norm / 16.0f - 1.0f);
    }
  }*/

  m_hovering = get_bar_rect().contains(mouse_pos);

  float new_progress = m_progress + (mouse_pos.y - last_mouse_pos) * m_total_region / m_covered_region;
  last_mouse_pos = mouse_pos.y;

  if (!m_scrolling)
    return false;

  m_progress = std::min(m_total_region - m_covered_region, std::max(0.f, new_progress));
  return true;
}

bool
ControlScrollbar::on_mouse_wheel(const SDL_MouseWheelEvent& wheel)
{
  /** This will always be executed, regardless of the mouse position.
      The control's parent manager should check conditions, if needed,
      before calling this function. */

  m_progress -= wheel.y * m_mouse_wheel_speed;

  if (m_progress < 0.f)
    m_progress = 0.f;
  else if (m_progress > m_total_region - m_covered_region)
    m_progress = m_total_region - m_covered_region;

  return true;
}

Rectf
ControlScrollbar::get_bar_rect()
{
  return Rectf(m_rect.get_left(),
               m_rect.get_top() + m_progress
                                * m_covered_region
                                / m_total_region,
               m_rect.get_right(),
               m_rect.get_top() + m_progress
                                * m_covered_region
                                / m_total_region
                          + m_rect.get_height()
                          * m_covered_region
                          / m_total_region
             );
}

/* EOF */
