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

#include "math/vector.hpp"

#include "math/util.hpp"

namespace math {

bool x_y_sorter(const Vector& lhs, const Vector& rhs)
{
   if (lhs.x != rhs.x)
    return lhs.x < rhs.x;

  return lhs.y < rhs.y;
}

bool y_x_sorter(const Vector& lhs, const Vector& rhs)
{
  if (lhs.y != rhs.y)
    return lhs.y < rhs.y;

  return lhs.x < rhs.x;
}


void rotate(Vector& v, float angle)
{
  const float theta = angle * PI / 180.f;
  const Vector origin = v;

  v.x = origin.x * std::cos(theta) - origin.y * std::sin(theta);
  v.y = origin.x * std::sin(theta) + origin.y * std::cos(theta);
}

} // namespace math

/* EOF */
