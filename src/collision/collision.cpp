//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#include "collision/collision.hpp"

#include <algorithm>

#include "math/aatriangle.hpp"
#include "math/rectf.hpp"
#include "supertux/constants.hpp"
#include "util/log.hpp"

namespace collision {

void
Constraints::merge_constraints(const Constraints& other)
{
  constrain_left(other.position_left);
  constrain_right(other.position_right);
  constrain_top(other.position_top);
  constrain_bottom(other.position_bottom);

  hit.left |= other.hit.left;
  hit.right |= other.hit.right;
  hit.top |= other.hit.top;
  hit.bottom |= other.hit.bottom;
  hit.crush |= other.hit.crush;
}

bool intersects(const Rectf& r1, const Rectf& r2)
{
  return r1.contains(r2);
}

//---------------------------------------------------------------------------

namespace {

inline void makePlane(const Vector& p1, const Vector& p2, Vector& n, float& c)
{
  n = Vector(p2.y - p1.y, p1.x - p2.x);
  c = -glm::dot(p2, n);
  float nval = glm::length(n);
  n /= nval;
  c /= nval;
}

}

bool rectangle_aatriangle(Constraints* constraints, const Rectf& rect,
                          const AATriangle& triangle)
{
  bool dummy;
  return rectangle_aatriangle(constraints, rect, triangle, dummy);
}

bool rectangle_aatriangle(Constraints* constraints, const Rectf& rect,
                          const AATriangle& triangle,
                          bool& hits_rectangle_bottom)
{
  if (!intersects(rect, triangle.bbox))
    return false;

  Vector normal(0.0f, 0.0f);
  float c = 0.0;
  Vector p1(0.0f, 0.0f);
  Rectf area;
  switch (triangle.dir & AATriangle::DEFORM_MASK) {
    case 0:
      area.set_p1(triangle.bbox.p1());
      area.set_p2(triangle.bbox.p2());
      break;
    case AATriangle::DEFORM_BOTTOM:
      area.set_p1(Vector(triangle.bbox.get_left(), triangle.bbox.get_top() + triangle.bbox.get_height()/2));
      area.set_p2(triangle.bbox.p2());
      break;
    case AATriangle::DEFORM_TOP:
      area.set_p1(triangle.bbox.p1());
      area.set_p2(Vector(triangle.bbox.get_right(), triangle.bbox.get_top() + triangle.bbox.get_height()/2));
      break;
    case AATriangle::DEFORM_LEFT:
      area.set_p1(triangle.bbox.p1());
      area.set_p2(Vector(triangle.bbox.get_left() + triangle.bbox.get_width()/2, triangle.bbox.get_bottom()));
      break;
    case AATriangle::DEFORM_RIGHT:
      area.set_p1(Vector(triangle.bbox.get_left() + triangle.bbox.get_width()/2, triangle.bbox.get_top()));
      area.set_p2(triangle.bbox.p2());
      break;
    default:
      assert(false);
  }

  switch (triangle.dir & AATriangle::DIRECTION_MASK) {
    case AATriangle::SOUTHWEST:
      p1 = Vector(rect.get_left(), rect.get_bottom());
      makePlane(area.p1(), area.p2(), normal, c);
      break;
    case AATriangle::NORTHEAST:
      p1 = Vector(rect.get_right(), rect.get_top());
      makePlane(area.p2(), area.p1(), normal, c);
      break;
    case AATriangle::SOUTHEAST:
      p1 = rect.p2();
      makePlane(Vector(area.get_left(), area.get_bottom()),
                Vector(area.get_right(), area.get_top()), normal, c);
      break;
    case AATriangle::NORTHWEST:
      p1 = rect.p1();
      makePlane(Vector(area.get_right(), area.get_top()),
                Vector(area.get_left(), area.get_bottom()), normal, c);
      break;
    default:
      assert(false);
  }

  float n_p1 = -glm::dot(normal, p1);
  float depth = n_p1 - c;
  if (depth < 0)
    return false;

#if 0
  std::cout << "R: " << rect << " Tri: " << triangle << "\n";
  std::cout << "Norm: " << normal << " Depth: " << depth << "\n";
#endif

  Vector outvec = normal * (depth + 0.2f);

  const float RDELTA = 3;
  if (p1.x < area.get_left() - RDELTA || p1.x > area.get_right() + RDELTA
     || p1.y < area.get_top() - RDELTA || p1.y > area.get_bottom() + RDELTA) {
    set_rectangle_rectangle_constraints(constraints, rect, area);
  } else {
    if (outvec.x < 0) {
      constraints->constrain_right(rect.get_right() + outvec.x);
      constraints->hit.right = true;
    } else {
      constraints->constrain_left(rect.get_left() + outvec.x);
      constraints->hit.left = true;
    }

    if (outvec.y < 0) {
      constraints->constrain_bottom(rect.get_bottom() + outvec.y);
      constraints->hit.bottom = true;
      hits_rectangle_bottom = true;
    } else {
      constraints->constrain_top(rect.get_top() + outvec.y);
      constraints->hit.top = true;
    }
    constraints->hit.slope_normal = normal;
  }

  return true;
}

void set_rectangle_rectangle_constraints(Constraints* constraints, const Rectf& r1, const Rectf& r2)
{
  float itop = r1.get_bottom() - r2.get_top();
  float ibottom = r2.get_bottom() - r1.get_top();
  float ileft = r1.get_right() - r2.get_left();
  float iright = r2.get_right() - r1.get_left();

  float vert_penetration = std::min(itop, ibottom);
  float horiz_penetration = std::min(ileft, iright);
  if (vert_penetration < horiz_penetration) {
    if (itop < ibottom) {
      constraints->constrain_bottom(r2.get_top());
      constraints->hit.bottom = true;
    } else {
      constraints->constrain_top(r2.get_bottom());
      constraints->hit.top = true;
    }
  } else {
    if (ileft < iright) {
      constraints->constrain_right(r2.get_left());
      constraints->hit.right = true;
    } else {
      constraints->constrain_left(r2.get_right());
      constraints->hit.left = true;
    }
  }
}

namespace {

class NearestCornerPoint final
{
public:
  NearestCornerPoint(const Vector& c, const float& p) :
    corner(c), point(p)
  {}

  float get_distance_x() const { return fabsf(point - corner.x); }
  float get_distance_y() const { return fabsf(point - corner.y); }

public:
  const Vector& corner;
  float point;
};

enum class NearestCornerHorizontalCompareType
{
  LEFT,
  RIGHT
};
NearestCornerPoint get_nearest_corner_point_x(const Vector& a, const Vector& b,
                                              const std::vector<Vector>& points,
                                              NearestCornerHorizontalCompareType compare_type)
{
  const bool reversed = (compare_type == NearestCornerHorizontalCompareType::RIGHT);

  for (int i = (reversed ? static_cast<int>(points.size()) - 1 : 0);
       (reversed ? i >= 0 : i < static_cast<int>(points.size())); i += (reversed ? -1 : 1))
  {
    const float result = math::get_nearest_point_x(a, b, points[i], true);
    if ((compare_type == NearestCornerHorizontalCompareType::LEFT && points[i].x <= result) ||
        (compare_type == NearestCornerHorizontalCompareType::RIGHT && points[i].x >= result))
      return { points[i], result };
  }

  return { points[0], math::get_nearest_point_x(a, b, points[0]) };
}

enum class NearestCornerVerticalCompareType
{
  TOP,
  BOTTOM
};
NearestCornerPoint get_nearest_corner_point_y(const Vector& a, const Vector& b,
                                              const std::vector<Vector>& points,
                                              NearestCornerVerticalCompareType compare_type)
{
  const bool reversed = (compare_type == NearestCornerVerticalCompareType::BOTTOM);

  for (int i = (reversed ? static_cast<int>(points.size()) - 1 : 0);
       (reversed ? i >= 0 : i < static_cast<int>(points.size())); i += (reversed ? -1 : 1))
  {
    float result = math::get_nearest_point_y(a, b, points[i], true);

    if (compare_type == NearestCornerVerticalCompareType::TOP) // TOP
    {
      if (points[i].y <= result)
        return { points[i], std::min(result, b.y) }; // Limit the bottommost point.
    }
    else if (points[i].y >= result) // BOTTOM
    {
      return { points[i], std::max(result, a.y) }; // Limit the topmost point.
    }
  }

  return { points[0], math::get_nearest_point_y(a, b, points[0]) };
}

enum class RotatedRectangleType
{
  NONE,
  RHOMBUS,
  FACING_LEFT,
  FACING_RIGHT
};

} // namespace

void set_rotated_rectangle_constraints(Constraints* constraints, const Rectf& r1, const Rectf& r2,
                                       const Vector& movement, bool horizontal)
{
  /** This function assumes rectangles 1 and 2 overlap. */

  const bool has_rotation1 = r1.is_rotated();
  const bool has_rotation2 = r2.is_rotated();
  assert(has_rotation1 || has_rotation2);

  /** Prepare sorted corner arrays. */
  auto corners1_x = r1.get_corners();
  auto corners1_y = corners1_x;
  auto corners2_x = r2.get_corners();
  auto corners2_y = corners2_x;

  // Sort the corners of both rectangles in ascending order, based on their X positions.
  std::sort(corners1_x.begin(), corners1_x.end(), math::x_y_sorter);
  std::sort(corners2_x.begin(), corners2_x.end(), math::x_y_sorter);

  // Sort the corners of both rectangles in ascending order, based on their Y positions.
  std::sort(corners1_y.begin(), corners1_y.end(), math::y_x_sorter);
  std::sort(corners2_y.begin(), corners2_y.end(), math::y_x_sorter);

  /**
    Determine the rotated rectangle type of rectangle 2.

    NONE: Not rotated, or rotated at 90 degrees.

    RHOMBUS:      / \          FACING_LEFT:     ___________         FACING_RIGHT:      __________
                /     \                         \          \                          /         /
              /         \                        \          \                        /         /
            /             \                       \          \                      /         /
            \             /                        \          \                    /         /
              \         /                           \          \                  /         /
                \     /                              \          \                /         /
                  \ /                                 \__________\              /_________/
  */
  RotatedRectangleType type2 = RotatedRectangleType::NONE;

  if (has_rotation2)
  {
    const auto size = r2.get_size();
    const float rotation = fmodf(r2.get_rotation(), 180.f); // In case the rectangle is rotated multiple times over.
    const bool square = r2.is_square();

    if (rotation == 90.f)
      type2 = RotatedRectangleType::NONE;
    else if (square && (rotation == 45.f || rotation == 135.f))
      type2 = RotatedRectangleType::RHOMBUS;
    else if ((square && (rotation < 45.f ||
                         (rotation > 90.f && rotation < 135.f))) ||
        (size.width < size.height && rotation <= 90.f) ||
        (size.width > size.height && rotation > 90.f))
      type2 = RotatedRectangleType::FACING_RIGHT;
    else
      type2 = RotatedRectangleType::FACING_LEFT;
  }

  /**
    Determine whether rectangle 1 is truly rotated (rotated and not at 90 degrees).
  */
  const bool rotated1 = (has_rotation1 && fmodf(r1.get_rotation(), 90.f) >= EPSILON);

  /**
    DETERMINE CONSTRAINTS

    NOTE: Topmost/bottommost corner variables represent the topmost and bottommost corner
          from the 2nd and 3rd corner on the Y axis.
          These corners are (excluding rectangles from type NONE) the 1st and 4th on the X axis,
          so they are determined from there.
  */

  const Vector& topmost_corner = (type2 == RotatedRectangleType::NONE ? corners2_x[2] :
                                  (type2 == RotatedRectangleType::FACING_LEFT ? corners2_x[0] : corners2_x[3]));
  const Vector& bottommost_corner = (type2 == RotatedRectangleType::NONE ? corners2_x[1] :
                                     (type2 == RotatedRectangleType::FACING_LEFT ? corners2_x[3] : corners2_x[0]));

  /**
    DETERMINE HORIZONTAL CONSTRAINTS
  */

  if (type2 == RotatedRectangleType::NONE) /** NONE: Second rectangle is not rotated, or rotated at exactly 90 degrees. */
  {
    // SHIFT_DELTA or a smaller value may have to be subtracted for left/right constraints
    // to avoid a bug, letting the first rectangle pass through.
    const float offset = (rotated1 ? SHIFT_DELTA : 0.f);

    if (corners1_x[3].x - movement.x <= corners2_x[0].x) /** Left */
    {
      const auto point_x = get_nearest_corner_point_x(corners2_y[0], bottommost_corner, corners1_x, NearestCornerHorizontalCompareType::RIGHT);
      constraints->constrain_right(corners1_x[3].x - point_x.get_distance_x() - offset);

      constraints->movement.x += point_x.get_distance_x() + offset;
      constraints->hit.right = true;
      return;
    }

    if (corners1_x[0].x - movement.x >= corners2_x[3].x) /** Right */
    {
      const auto point_x = get_nearest_corner_point_x(topmost_corner, corners2_y[3], corners1_x, NearestCornerHorizontalCompareType::LEFT);
      constraints->constrain_left(corners1_x[0].x + point_x.get_distance_x() + offset);

      constraints->movement.x -= point_x.get_distance_x() - offset;
      constraints->hit.left = true;
      return;
    }
  }
  else if (type2 == RotatedRectangleType::FACING_LEFT) /** FACING_LEFT */
  {
    if (corners1_x[3].x - movement.x <= corners2_y[3].x && corners1_x[3].y - movement.y >= topmost_corner.y) /** Left */
    {
      const auto point_y = get_nearest_corner_point_y(topmost_corner, corners2_y[3], corners1_y, NearestCornerVerticalCompareType::TOP);
      constraints->constrain_top(corners1_y[0].y + point_y.get_distance_y());

      constraints->movement.y -= point_y.get_distance_y();
      constraints->hit.top = true;

      if (!has_rotation1)
      {
        const auto point_x = get_nearest_corner_point_x(topmost_corner, corners2_y[3], corners1_x, NearestCornerHorizontalCompareType::RIGHT);
        constraints->constrain_right(corners1_x[3].x - point_x.get_distance_x());
        constraints->hit.right = true;
      }
      return;
    }

    if (corners1_x[0].x - movement.x >= corners2_y[0].x && corners1_x[0].y - movement.y <= bottommost_corner.y) /** Right */
    {
      const auto point_y = get_nearest_corner_point_y(corners2_y[0], bottommost_corner, corners1_y, NearestCornerVerticalCompareType::BOTTOM);
      constraints->constrain_bottom(corners1_y[3].y - point_y.get_distance_y());

      constraints->movement.y += point_y.get_distance_y();
      constraints->hit.bottom = true;

      if (!has_rotation1)
      {
        const auto point_x = get_nearest_corner_point_x(bottommost_corner, corners2_y[0], corners1_x, NearestCornerHorizontalCompareType::LEFT);
        constraints->constrain_left(corners1_x[0].x + point_x.get_distance_x());
        constraints->hit.left = true;
      }
      return;
    }
  }
  else /** RHOMBUS, FACING_RIGHT */
  {
    if (corners1_x[3].x - movement.x <= corners2_y[0].x && corners1_x[3].y - movement.y <= bottommost_corner.y) /** Left */
    {
      const auto point_y = get_nearest_corner_point_y(corners2_y[0], bottommost_corner, corners1_y, NearestCornerVerticalCompareType::BOTTOM);
      constraints->constrain_bottom(corners1_y[3].y - point_y.get_distance_y());

      constraints->movement.y += point_y.get_distance_y();
      constraints->hit.bottom = true;

      if (!has_rotation1)
      {
        const auto point_x = get_nearest_corner_point_x(bottommost_corner, corners2_y[0], corners1_x, NearestCornerHorizontalCompareType::RIGHT);
        constraints->constrain_right(corners1_x[3].x - point_x.get_distance_x());
        constraints->hit.right = true;
      }
      return;
    }

    if (corners1_x[0].x - movement.x >= corners2_y[3].x && corners1_x[0].y - movement.y >= topmost_corner.y) /** Right */
    {
      const auto point_y = get_nearest_corner_point_y(topmost_corner, corners2_y[3], corners1_y, NearestCornerVerticalCompareType::TOP);
      constraints->constrain_top(corners1_y[0].y + point_y.get_distance_y());

      constraints->movement.y -= point_y.get_distance_y();
      constraints->hit.top = true;

      if (!has_rotation1)
      {
        const auto point_x = get_nearest_corner_point_x(topmost_corner, corners2_y[3], corners1_x, NearestCornerHorizontalCompareType::LEFT);
        constraints->constrain_left(corners1_x[0].x + point_x.get_distance_x());
        constraints->hit.left = true;
      }
      return;
    }
  }

  /**
    DETERMINE VERTICAL CONSTRAINTS
  */

  if (type2 == RotatedRectangleType::FACING_LEFT) /** FACING_LEFT */
  {
    if (corners1_x[3].y - movement.y <= topmost_corner.y) /** Top */
    {
      // Detect whether the first rectangle is standing on its side on top of an edge of the second rectangle.
      const bool on_left_edge = (rotated1 && corners1_y[3].x - movement.x < topmost_corner.x);
      const bool on_edge = (rotated1 && (on_left_edge || corners1_y[3].x - movement.x > corners2_y[0].x));

      const Vector& corner_a = (on_edge ? (on_left_edge ? corners1_x[3] : corners1_x[0]) : corners2_y[0]);
      const Vector& corner_b = (on_edge ? corners1_y[3] : topmost_corner);

      const auto point_y = get_nearest_corner_point_y(corner_a, corner_b, on_edge ? corners2_y : corners1_y,
                                                      on_edge ? NearestCornerVerticalCompareType::TOP : NearestCornerVerticalCompareType::BOTTOM);
      constraints->constrain_bottom(corners1_y[3].y - point_y.get_distance_y());

      constraints->movement.y += point_y.get_distance_y();
      constraints->hit.bottom = true;
      return;
    }

    if (corners1_x[3].y - movement.y >= bottommost_corner.y) /** Bottom */
    {
      // Detect whether the first rectangle is touching an edge of the second rectangle from its side.
      const bool on_left_edge = (rotated1 && corners1_y[0].x - movement.x < corners2_y[3].x);
      const bool on_edge = (rotated1 && (on_left_edge || corners1_y[0].x - movement.x > bottommost_corner.x));

      const Vector& corner_a = (on_edge ? (on_left_edge ? corners1_x[3] : corners1_x[0]) : bottommost_corner);
      const Vector& corner_b = (on_edge ? corners1_y[0] : corners2_y[3]);

      const auto point_y = get_nearest_corner_point_y(corner_a, corner_b, on_edge ? corners2_y : corners1_y,
                                                      on_edge ? NearestCornerVerticalCompareType::BOTTOM : NearestCornerVerticalCompareType::TOP);
      constraints->constrain_top(corners1_y[0].y + point_y.get_distance_y());

      constraints->movement.y -= point_y.get_distance_y();
      constraints->hit.top = true;
      return;
    }
  }
  else /** NONE, RHOMBUS, FACING_RIGHT */
  {
    if (corners1_x[0].y - movement.y >= bottommost_corner.y) /** Bottom */
    {
      // Detect whether the first rectangle is touching an edge of the second rectangle from its side.
      const bool on_left_edge = (rotated1 && corners1_y[0].x - movement.x < bottommost_corner.x);
      const bool on_edge = (rotated1 && (on_left_edge || corners1_y[0].x - movement.x > corners2_y[3].x));

      const Vector& corner_a = (on_edge ? (on_left_edge ? corners1_x[3] : corners1_x[0]) : bottommost_corner);
      const Vector& corner_b = (on_edge ? corners1_y[0] : corners2_y[3]);

      const auto point_y = get_nearest_corner_point_y(corner_a, corner_b, on_edge ? corners2_y : corners1_y,
                                                      on_edge ? NearestCornerVerticalCompareType::BOTTOM : NearestCornerVerticalCompareType::TOP);
      constraints->constrain_top(corners1_y[0].y + point_y.get_distance_y());

      constraints->movement.y -= point_y.get_distance_y();
      constraints->hit.top = true;
      return;
    }

    if (corners1_x[0].y - movement.y <= topmost_corner.y) /** Top */
    {
      // Detect whether the first rectangle is standing on its side on top of an edge of the second rectangle.
      const bool on_left_edge = (rotated1 && corners1_y[3].x - movement.x < corners2_y[0].x);
      const bool on_edge = (rotated1 && (on_left_edge || corners1_y[3].x - movement.x > topmost_corner.x));

      const Vector& corner_a = (on_edge ? (on_left_edge ? corners1_x[3] : corners1_x[0]) : corners2_y[0]);
      const Vector& corner_b = (on_edge ? corners1_y[3] : topmost_corner);

      const auto point_y = get_nearest_corner_point_y(corner_a, corner_b, on_edge ? corners2_y : corners1_y,
                                                      on_edge ? NearestCornerVerticalCompareType::TOP : NearestCornerVerticalCompareType::BOTTOM);
      constraints->constrain_bottom(corners1_y[3].y - point_y.get_distance_y());

      constraints->movement.y += point_y.get_distance_y();
      constraints->hit.bottom = true;
      return;
    }
  }

  /** None of the checks have passed, but we know the rectangles overlap.
      Determine shiftout to left/right or top/bottom. */

  const Vector middle1 = r1.get_middle();

  if (fabsf(movement.y) > (horizontal ? 0.f : fabsf(movement.x)))
  {
    if (middle1.x < r2.get_middle().x) /** LEFT */
      constraints->constrain_right(has_rotation2 ? math::get_nearest_point_x(corners2_x[0], corners2_x[1], middle1) : r2.get_left());
    else /** RIGHT */
      constraints->constrain_left(has_rotation2 ? math::get_nearest_point_x(corners2_x[2], corners2_x[3], middle1) : r2.get_right());
  }
  else
  {
    if (middle1.y < r2.get_middle().y) /** TOP */
      constraints->constrain_bottom(has_rotation2 ? math::get_nearest_point_y(corners2_y[0], corners2_y[1], middle1) : r2.get_top());
    else /** BOTTOM */
      constraints->constrain_top(has_rotation2 ? math::get_nearest_point_y(corners2_y[2], corners2_y[3], middle1) : r2.get_bottom());
  }
}

bool line_intersects_line(const Vector& line1_start, const Vector& line1_end, const Vector& line2_start, const Vector& line2_end)
{
  // Adapted from Striker, (C) 1999 Joris van der Hoeven, GPL

  float a1 = line1_start.x, b1 = line1_start.y, a2 = line1_end.x, b2 = line1_end.y;
  float c1 = line2_start.x, d1 = line2_start.y, c2 = line2_end.x, d2 = line2_end.y;

  float num = (b2-b1)*(c2-c1) - (a2-a1)*(d2-d1);
  float den1 = (d2-b2)*(c1-c2) + (a2-c2)*(d1-d2);
  float den2 = (d2-b2)*(a1-a2) + (a2-c2)*(b1-b2);

  // Normalize to positive numerator.
  if (num < 0) {
    num = -num;
    den1 = -den1;
    den2 = -den2;
  }

  // Numerator is zero -> Check for parallel or coinciding lines.
  if (num == 0) {
    if ((b1-b2)*(c1-a2) != (a1-a2)*(d1-b2)) return false;
    if (a1 == a2) {
      std::swap(a1, b1);
      std::swap(a2, b2);
      std::swap(c1, d1);
      std::swap(c2, d2);
    }
    if (a1 > a2) std::swap(a1, a2);
    if (c1 > c2) std::swap(c1, c2);
    return ((a1 <= c2) && (a2 >= c1));
  }

  // Standard check.
  return (den1>=0) && (den1<=num) && (den2>=0) && (den2<=num);

}

bool intersects_line(const Rectf& r, const Vector& line_start, const Vector& line_end)
{
  Vector p1 = r.p1();
  Vector p2 = Vector(r.get_right(), r.get_top());
  Vector p3 = r.p2();
  Vector p4 = Vector(r.get_left(), r.get_bottom());
  if (line_intersects_line(p1, p2, line_start, line_end)) return true;
  if (line_intersects_line(p2, p3, line_start, line_end)) return true;
  if (line_intersects_line(p3, p4, line_start, line_end)) return true;
  if (line_intersects_line(p4, p1, line_start, line_end)) return true;
  return false;
}

}

/* EOF */
