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

#include "math/line.hpp"

Vector
Line::project(const Vector& dest) const
{
  const float distance = m_direction.x * (dest.x - m_origin.x) +
                         m_direction.y * (dest.y - m_origin.y);

  return m_origin + m_direction * distance;
}


namespace math {

float get_nearest_point_x(const Vector& a, const Vector& b,
                          const Vector& p, bool allow_outside_line)
{
  /** Get the nearest point to P on line A-B on the X axis. */
  const float result = (a.y == b.y ? p.x : ((a.y - p.y) * b.x - (b.y - p.y) * a.x) / (a.y - b.y));

  if (!allow_outside_line)
  {
    if (a.x < b.x && result < a.x ||
        a.x > b.x && result > a.x)
      return a.x;
    if (a.x > b.x && result < b.x ||
        a.x < b.x && result > b.x)
      return b.x;
  }

  return result;
}

float get_nearest_point_y(const Vector& a, const Vector& b,
                          const Vector& p, bool allow_outside_line)
{
  /** Get the nearest point to P on line A-B on the Y axis. */
  const float result = (a.x == b.x ? p.y : ((a.x - p.x) * b.y - (b.x - p.x) * a.y) / (a.x - b.x));

  if (!allow_outside_line)
  {
    if (a.y < b.y && result < a.y ||
        a.y > b.y && result > a.y)
      return a.y;
    if (a.y > b.y && result < b.y ||
        a.y < b.y && result > b.y)
      return b.y;
  }

  return result;
}

} // namespace math

/* EOF */
