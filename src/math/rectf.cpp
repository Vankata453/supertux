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

#include <ostream>

#include "math/rect.hpp"

Rectf::Rectf(const Rect& rect) :
  m_p1(static_cast<float>(rect.left),
     static_cast<float>(rect.top)),
  m_size(static_cast<float>(rect.get_width()),
         static_cast<float>(rect.get_height()))
{
}

void
Rectf::from_string(std::string& str)
{
  str.erase(0, 6); // Remove "Rectf(".
  str.pop_back(); // Remove ")".

  std::vector<float> values;
  std::string current_num;

  str += ", ";
  for (auto& ch : str)
  {
    if (std::isspace(ch))
    {
      current_num.pop_back(); // Remove the comma before the space (", ").
      values.push_back(std::stof(current_num));
      current_num.clear();
      continue;
    }
    current_num += ch;
  }

  assert(values.size() >= 4);

  set_left(values[0]);
  set_top(values[1]);
  set_right(values[2]);
  set_bottom(values[3]);
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
