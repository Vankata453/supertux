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

#ifndef HEADER_SUPERTUX_SCRIPTING_CAMERA_HPP
#define HEADER_SUPERTUX_SCRIPTING_CAMERA_HPP

#ifndef SCRIPTING_API
#include "scripting/game_object.hpp"

class Camera;
#endif

namespace scripting {

class Camera final
#ifndef SCRIPTING_API
  : public GameObject<::Camera>
#endif
{
public:
#ifndef SCRIPTING_API
public:
  using GameObject::GameObject;

private:
  Camera(const Camera&) = delete;
  Camera& operator=(const Camera&) = delete;
#endif

public:
  void reload_config();

  /** Shake the camera */
  void shake(float speed, float x, float y);
  /** Set camera to a specific coordinate */
  void set_pos(float x, float y);
  /** Set camera to a specific mode, can be "normal", "manual" */
  void set_mode(const std::string& mode);
  /** Scroll camera to position x,y in scrolltime seconds */
  void scroll_to(float x, float y, float scrolltime);
};

} // namespace scripting

#endif

/* EOF */
