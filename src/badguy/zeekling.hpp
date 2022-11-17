//  Zeekling - flyer that swoops down when she spots the player
//  Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//  Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_ZEEKLING_HPP
#define HEADER_SUPERTUX_BADGUY_ZEEKLING_HPP

#include "badguy/badguy.hpp"

class Zeekling : public BadGuy
{
private:
  static const float s_status_shift;

public:
  Zeekling(const ReaderMapping& reader);
  Zeekling(const Vector& pos, Direction d);

  ObjectSettings get_settings() override;
  void after_editor_set() override;

  float draw_status(DrawingContext& context, const float& pos_x) override;

  void initialize();
  void collision_solid(const CollisionHit& hit);
  void active_update(float elapsed_time);

  void freeze();
  void unfreeze();
  bool is_freezable() const;
  std::string get_class() const {
    return "zeekling";
  }
  std::string get_display_name() const {
    return _("Zeekling");
  }

private:
  bool collision_squished(GameObject& object);
  bool should_we_dive();
  void onBumpHorizontal();
  void onBumpVertical();

private:
  enum ZeeklingState {
    FLYING,
    DIVING,
    CLIMBING
  };

  struct DiveVariables {
    DiveVariables() :
      last_player_pos(),
      last_self_pos(),
      player_pos(),
      player_mov(),
      self_pos(),
      self_mov(),
      vy(),
      height(),
      relSpeed(),
      estFrames(),
      estPx(),
      estBx()
    {}

    Vector last_player_pos; /**< position we last spotted the player at */
    Vector last_self_pos; /**< position we last were at */
    Vector player_pos;
    Vector player_mov;
    Vector self_pos;
    Vector self_mov;
    float vy;
    float height;
    float relSpeed;
    float estFrames;
    float estPx;
    float estBx;
  };

private:
  float speed;
  Timer diveRecoverTimer;
  ZeeklingState state;
  const MovingObject* last_player; /**< last player we tracked */
  DiveVariables dive_variables;
  bool display_dive_variables;

private:
  Zeekling(const Zeekling&);
  Zeekling& operator=(const Zeekling&);
};

#endif

/* EOF */
