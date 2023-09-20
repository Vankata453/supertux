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

#ifndef HEADER_SUPERTUX_MATH_LINE_HPP
#define HEADER_SUPERTUX_MATH_LINE_HPP

#include "math/vector.hpp"

class Line final
{
public:
  Line() :
    m_origin(),
    m_direction()
  {}
  Line(const float& x, const float& y,
       const float& dx, const float& dy) :
    m_origin(x, y),
    m_direction(dx, dy)
  {}
  Line(const Vector& origin, const Vector& direction) :
    m_origin(origin),
    m_direction(direction)
  {}

  const Vector& get_origin() const { return m_origin; }
  const Vector& get_direction() const { return m_direction; }

  void set_origin(const Vector& origin) { m_origin = origin; }
  void set_direction(const Vector& direction) { m_direction = direction; }

  Vector project(const Vector& dest) const;

private:
  Vector m_origin;
  Vector m_direction;
};


namespace math {

float get_nearest_point_x(const Vector& a, const Vector& b,
                          const Vector& p, bool allow_outside_line = false);
float get_nearest_point_y(const Vector& a, const Vector& b,
                          const Vector& p, bool allow_outside_line = false);

} // namespace math

#endif

/* EOF */
