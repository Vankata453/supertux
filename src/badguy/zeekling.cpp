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

#include "badguy/zeekling.hpp"

#include <math.h>

#include "math/random_generator.hpp"
#include "object/player.hpp"
#include "sprite/sprite.hpp"
#include "supertux/object_factory.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"

Zeekling::Zeekling(const ReaderMapping& reader) :
  BadGuy(reader, "images/creatures/zeekling/zeekling.sprite"),
  speed(),
  diveRecoverTimer(),
  state(),
  last_player(0),
  last_player_pos(),
  last_self_pos(),
  show_estimates(false),
  estPx(),
  estBx()
{
  state = FLYING;
  speed = gameRandom.rand(130, 171);
  reader.get("show-estimates", show_estimates);
  physic.enable_gravity(false);
}

Zeekling::Zeekling(const Vector& pos, Direction d) :
  BadGuy(pos, d, "images/creatures/zeekling/zeekling.sprite"),
  speed(),
  diveRecoverTimer(),
  state(),
  last_player(0),
  last_player_pos(),
  last_self_pos(),
  show_estimates(false),
  estPx(),
  estBx()
{
  state = FLYING;
  speed = gameRandom.rand(130, 171);
  physic.enable_gravity(false);
}

ObjectSettings
Zeekling::get_settings()
{
  ObjectSettings result = BadGuy::get_settings();

  result.options.push_back(ObjectOption(MN_TOGGLE, _("Show Position Estimates"), &show_estimates,
                                         "show-estimates"));

  return result;
}

void
Zeekling::initialize()
{
  physic.set_velocity_x(dir == LEFT ? -speed : speed);
  sprite->set_action(dir == LEFT ? "left" : "right");
}

void
Zeekling::draw(DrawingContext& context)
{
  BadGuy::draw(context);

  auto player = get_nearest_player();
  if (!show_estimates || !player) return;

  Vector est_player_pos(estPx, player->get_pos().y);
  Vector est_self_pos(estBx, get_pos().y);

  context.draw_filled_rect(est_player_pos, player->get_bbox().get_size().as_vector(), Color(0, 0, 1, 1), LAYER_OBJECTS + 1);
  context.draw_filled_rect(est_self_pos, get_bbox().get_size().as_vector(), Color(1, 0, 0, 1), LAYER_OBJECTS + 1);

  context.draw_line(est_player_pos, est_self_pos, Color(0, 1, 0, 1), LAYER_OBJECTS + 1);
}

bool
Zeekling::collision_squished(GameObject& object)
{
  sprite->set_action(dir == LEFT ? "squished-left" : "squished-right");
  kill_squished(object);
  return true;
}

void
Zeekling::onBumpHorizontal() {
  if (frozen)
  {
    physic.set_velocity_x(0);
    return;
  }
  if (state == FLYING) {
    dir = (dir == LEFT ? RIGHT : LEFT);
    sprite->set_action(dir == LEFT ? "left" : "right");
    physic.set_velocity_x(dir == LEFT ? -speed : speed);
  } else
    if (state == DIVING) {
      dir = (dir == LEFT ? RIGHT : LEFT);
      state = FLYING;
      sprite->set_action(dir == LEFT ? "left" : "right");
      physic.set_velocity_x(dir == LEFT ? -speed : speed);
      physic.set_velocity_y(0);
    } else
      if (state == CLIMBING) {
        dir = (dir == LEFT ? RIGHT : LEFT);
        sprite->set_action(dir == LEFT ? "left" : "right");
        physic.set_velocity_x(dir == LEFT ? -speed : speed);
      } else {
        assert(false);
      }
}

void
Zeekling::onBumpVertical() {
  if (frozen || BadGuy::get_state() == STATE_BURNING)
  {
    physic.set_velocity_y(0);
    physic.set_velocity_x(0);
    return;
  }
  if (state == FLYING) {
    physic.set_velocity_y(0);
  } else
    if (state == DIVING) {
      state = CLIMBING;
      physic.set_velocity_y(-speed);
      sprite->set_action(dir == LEFT ? "left" : "right");
    } else
      if (state == CLIMBING) {
        state = FLYING;
        physic.set_velocity_y(0);
      }
}

void
Zeekling::collision_solid(const CollisionHit& hit)
{
  if(sprite->get_action() == "squished-left" ||
     sprite->get_action() == "squished-right")
  {
    return;
  }

  if(hit.top || hit.bottom) {
    onBumpVertical();
  } else if(hit.left || hit.right) {
    onBumpHorizontal();
  }
}

/**
 * linear prediction of player and badguy positions to decide if we should enter the DIVING state
 */
bool
Zeekling::should_we_dive() {
  if (frozen)
    return false;

  const auto player = get_nearest_player();
  if (player && last_player && (player == last_player)) {

    // get positions, calculate movement
    const Vector& player_pos = player->get_pos();
    const Vector player_mov = (player_pos - last_player_pos);
    const Vector self_pos = bbox.p1;
    const Vector self_mov = (self_pos - last_self_pos);

    // new vertical speed to test with
    float vy = 2*fabsf(self_mov.x);

    // do not dive if we are not above the player
    float height = player_pos.y - self_pos.y;
    if (height <= 0) return false;

    // do not dive if we are too far above the player
    if (height > 512) return false;

    // do not dive if we would not descend faster than the player
    float relSpeed = vy - player_mov.y;
    if (relSpeed <= 0) return false;

    // guess number of frames to descend to same height as player
    float estFrames = height / relSpeed;

    // guess where the player would be at this time
    estPx = (player_pos.x + (estFrames * player_mov.x));

    // guess where we would be at this time
    estBx = (self_pos.x + (estFrames * self_mov.x));

    // near misses are OK, too
    if (fabsf(estPx - estBx) < 8)
    {
      if (show_estimates)
        log_warning << "Diving on " << self_pos << std::endl;

      return true;
    }
  }

  // update last player tracked, as well as our positions
  last_player = player;
  if (player) {
    last_player_pos = player->get_pos();
    last_self_pos = bbox.p1;
  }

  return false;
}

void
Zeekling::active_update(float elapsed_time) {
  if (state == FLYING) {
    if (should_we_dive()) {
      state = DIVING;
      physic.set_velocity_y(2*fabsf(physic.get_velocity_x()));
      sprite->set_action(dir == LEFT ? "diving-left" : "diving-right");
    }
    BadGuy::active_update(elapsed_time);
    return;
  } else if (state == DIVING) {
    BadGuy::active_update(elapsed_time);
    return;
  } else if (state == CLIMBING) {
    // stop climbing when we're back at initial height
    if (get_pos().y <= start_position.y) {
      state = FLYING;
      physic.set_velocity_y(0);
    }
    BadGuy::active_update(elapsed_time);
    return;
  } else {
    assert(false);
  }
}

void
Zeekling::freeze()
{
  BadGuy::freeze();
  physic.enable_gravity(true);
}

void
Zeekling::unfreeze()
{
  BadGuy::unfreeze();
  physic.enable_gravity(false);
  state = FLYING;
  initialize();
}

bool
Zeekling::is_freezable() const
{
  return true;
}

/* EOF */
