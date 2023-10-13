//  SuperTux
//  Copyright (C) 2016 Ingo Ruhnke <grumbel@gmail.com>
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

#include "math/rectf.hpp"

#include <algorithm>
#include <ostream>

#include "math/rect.hpp"
#include "supertux/constants.hpp"

Rectf::Rectf(const Rect& rect, float angle) :
  m_p1(static_cast<float>(rect.left),
       static_cast<float>(rect.top)),
  m_size(static_cast<float>(rect.get_width()),
         static_cast<float>(rect.get_height())),
  m_angle(angle)
{
}


Rectf::Corners
Rectf::get_corners(const std::function<bool (const Vector&, const Vector&)>& sorter) const
{
  std::vector<Vector> corner_positions = {
    m_p1,
    m_p1 + Vector(m_size.width, 0.f),
    m_p1 + Vector(0.f, m_size.height),
    m_p1 + Vector(m_size.width, m_size.height)
  };

  if (!is_rotated())
    return corner_positions; // No need to determine rotated corner positions.

  const Vector middle = get_middle();

  for (Vector& pos : corner_positions)
  {
    pos -= middle; // Translate the position to origin.
    math::rotate(pos, m_angle);
    pos += middle; // Translate back.
  }

  if (sorter)
    std::sort(corner_positions.begin(), corner_positions.end(), sorter);

  return corner_positions;
}

Rectf::Axis
Rectf::get_axis() const
{
  Vector ox(1.f, 0.f);
  Vector oy(0.f, 1.f);

  math::rotate(ox, m_angle);
  math::rotate(oy, m_angle);

  const Vector middle = get_middle();
  return { Line(middle, ox),
           Line(middle, oy) };
}

bool
Rectf::is_rotated() const
{
  // Check 1: Make sure angle is bigger than 0.
  // Check 2: Make sure angle isn't divisible by 180.
  // Check 3: If the rectangle is a square, make sure angle isn't divisble by 90.
  return m_angle > 0.f && fmodf(m_angle, 180.f) >= EPSILON &&
         !(is_square() && fmodf(m_angle, 90.f) < EPSILON);
}

bool
Rectf::is_square() const
{
  // Loose comparison, check is inconsistent otherwise.
  return fabsf(m_size.width - m_size.height) < EPSILON;
}


static bool projections_overlap(const Rectf::Corners& corners1, const Rectf& r2)
{
  /** Check adapted from: https://stackoverflow.com/a/62028170 */

  /** NOTE: To perform a valid collision check for rectangles, make sure this function is ran twice,
            the corners of the first/second rectangle, and the second/first rectangle being provided as arguments.
            The 2 projections of each rectangle should overlap the respective other rectangle
            for collision to occur. */

  const auto axis2 = r2.get_axis();

  for (int i = 0; i < 2; i++)
  {
    const Line& line = (i == 0 ? axis2.first : axis2.second);

    float min_distance = 0.f;
    float max_distance = 0.f;

    for (const Vector& corner : corners1)
    {
      const Vector corner_projection = line.project(corner) - r2.get_middle();
      const bool sign = (corner_projection.x * line.get_direction().x) + (corner_projection.y * line.get_direction().y) > 0;
      const float distance = math::magnitude(corner_projection) * (sign ? 1.f : -1.f);

      if (min_distance == 0.f || min_distance > distance)
        min_distance = distance;
      if (max_distance == 0.f || max_distance < distance)
        max_distance = distance;
    }

    const float half_size = (i == 0 ? r2.get_width() : r2.get_height()) / 2;

    /**
      MAIN OVERLAP CHECK

      1D check to ensure the projection overlaps the rectangle.
      If it doesn't, the rectangles do not overlap.
    */
    if (!(min_distance < 0 && max_distance > 0 ||
          fabsf(min_distance) < half_size ||
          fabsf(max_distance) < half_size))
      return false;
  }

  return true;
}

bool
Rectf::contains(const Vector& v) const
{
  if (is_rotated()) // Rotated rectangle overlap
    return projections_overlap({ v }, *this);

  // Regular rectangle overlap
  return v.x >= m_p1.x && v.y >= m_p1.y && v.x < get_right() && v.y < get_bottom();
}

bool
Rectf::contains(const Rectf& other) const
{
  if (is_rotated() || other.is_rotated()) // Rotated rectangle overlap
    return projections_overlap(get_corners(),       other) &&
           projections_overlap(other.get_corners(), *this);

  // Regular rectangle overlap
  if (m_p1.x >= other.get_right() || other.get_left() >= get_right())
    return false;
  if (m_p1.y >= other.get_bottom() || other.get_top() >= get_bottom())
    return false;

  return true;
}


Rect
Rectf::to_rect() const
{
  return { static_cast<int>(m_p1.x), static_cast<int>(m_p1.y),
           static_cast<int>(m_size.width), static_cast<int>(m_size.height) };
}

std::ostream& operator<<(std::ostream& out, const Rectf& rect)
{
  out << "Rectf("
      << rect.get_left() << ", " << rect.get_top() << ", "
      << rect.get_right() << ", " << rect.get_bottom()
      << ")";
  return out;
}

/* EOF */
