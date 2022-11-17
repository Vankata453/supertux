//  SkullyHop - A Hopping Skull
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

#include "badguy/skullyhop.hpp"

#include <algorithm>

#include "audio/sound_manager.hpp"
#include "math/random_generator.hpp"
#include "sprite/sprite.hpp"
#include "supertux/object_factory.hpp"
#include "util/reader_mapping.hpp"

namespace {
const float MIN_RECOVER_TIME = 0.1f; /**< minimum time to stand still before starting a (new) jump */
const float MAX_RECOVER_TIME = 1.0f; /**< maximum time to stand still before starting a (new) jump */
static const std::string SKULLYHOP_SOUND = "sounds/hop.ogg";
}

SkullyHop::SkullyHop(const ReaderMapping& reader) :
  BadGuy(reader, "images/creatures/skullyhop/skullyhop.sprite"),
  recover_timer(),
  state(),
  current_jump_time(1),
  jump_time_1(0),
  jump_time_2(0),
  jump_time_3(0),
  jump_time_4(0),
  jump_time_5(0)
{
  SoundManager::current()->preload( SKULLYHOP_SOUND );
}

SkullyHop::SkullyHop(const Vector& pos, Direction d) :
  BadGuy(pos, d, "images/creatures/skullyhop/skullyhop.sprite"),
  recover_timer(),
  state(),
  current_jump_time(6),
  jump_time_1(0),
  jump_time_2(0),
  jump_time_3(0),
  jump_time_4(0),
  jump_time_5(0)
{
  SoundManager::current()->preload( SKULLYHOP_SOUND );
}

ObjectSettings
SkullyHop::get_settings()
{
  ObjectSettings result = BadGuy::get_settings();

  result.options.push_back(ObjectOption(MN_NUMFIELD, _("Jump time 1"), &jump_time_1,
                                         "jump-time-1"));
  result.options.push_back(ObjectOption(MN_NUMFIELD, _("Jump time 2"), &jump_time_2,
                                         "jump-time-2"));
  result.options.push_back(ObjectOption(MN_NUMFIELD, _("Jump time 3"), &jump_time_3,
                                         "jump-time-3"));
  result.options.push_back(ObjectOption(MN_NUMFIELD, _("Jump time 4"), &jump_time_4,
                                         "jump-time-4"));
  result.options.push_back(ObjectOption(MN_NUMFIELD, _("Jump time 5"), &jump_time_5,
                                         "jump-time-5"));

  return result;
}

void
SkullyHop::after_editor_set()
{
  BadGuy::after_editor_set();
  sprite->set_action(dir == LEFT ? "standing-left" : "standing-right");

  std::vector<EditorChange> changes;

  // Watch for values above 0, so initial value of jump times (0) still gets accepted.
  changes.push_back({ jump_time_1 >= 0 && jump_time_1 <= MAX_RECOVER_TIME });
  changes.push_back({ jump_time_2 >= 0 && jump_time_2 <= MAX_RECOVER_TIME });
  changes.push_back({ jump_time_3 >= 0 && jump_time_3 <= MAX_RECOVER_TIME });
  changes.push_back({ jump_time_4 >= 0 && jump_time_4 <= MAX_RECOVER_TIME });
  changes.push_back({ jump_time_5 >= 0 && jump_time_5 <= MAX_RECOVER_TIME });

  if (std::find_if(changes.begin(), changes.end(),
      [](const EditorChange& change) { return !change.valid; }) != changes.end())
  {
    log_warning << "Wrong " << get_class() << " property range." << std::endl;
  }
}

void
SkullyHop::initialize()
{
  // initial state is JUMPING, because we might start airborne
  state = JUMPING;
  sprite->set_action(dir == LEFT ? "jumping-left" : "jumping-right");
}

void
SkullyHop::set_state(SkullyHopState newState)
{
  if (newState == STANDING) {
    physic.set_velocity_x(0);
    physic.set_velocity_y(0);
    sprite->set_action(dir == LEFT ? "standing-left" : "standing-right");

    float recover_time;
    if (current_jump_time <= 5)
    {
      switch (current_jump_time)
      {
        case 1:
          recover_time = jump_time_1;
          break;
        case 2:
          recover_time = jump_time_2;
          break;
        case 3:
          recover_time = jump_time_3;
          break;
        case 4:
          recover_time = jump_time_4;
          break;
        case 5:
          recover_time = jump_time_5;
          break;
      }
      if (recover_time < MIN_RECOVER_TIME)
        recover_time = gameRandom.randf(MIN_RECOVER_TIME, MAX_RECOVER_TIME);
      current_jump_time++;
    }
    else
    {
      recover_time = gameRandom.randf(MIN_RECOVER_TIME, MAX_RECOVER_TIME);
    }

    recover_timer.start(recover_time);
  } else
    if (newState == CHARGING) {
      sprite->set_action(dir == LEFT ? "charging-left" : "charging-right", 1);
    } else
      if (newState == JUMPING) {
        sprite->set_action(dir == LEFT ? "jumping-left" : "jumping-right");
const float HORIZONTAL_SPEED = 220; /**< x-speed when jumping */
        physic.set_velocity_x(dir == LEFT ? -HORIZONTAL_SPEED : HORIZONTAL_SPEED);
const float VERTICAL_SPEED = -450;   /**< y-speed when jumping */
        physic.set_velocity_y(VERTICAL_SPEED);
        SoundManager::current()->play( SKULLYHOP_SOUND, get_pos());
      }

  state = newState;
}

bool
SkullyHop::collision_squished(GameObject& object)
{
  if (frozen)
    return BadGuy::collision_squished(object);

  sprite->set_action(dir == LEFT ? "squished-left" : "squished-right");
  kill_squished(object);
  return true;
}

void
SkullyHop::collision_solid(const CollisionHit& hit)
{
  if (frozen || BadGuy::get_state() == STATE_BURNING)
  {
    BadGuy::collision_solid(hit);
    return;
  }

  // just default behaviour (i.e. stop at floor/walls) when squished
  if (BadGuy::get_state() == STATE_SQUISHED) {
    BadGuy::collision_solid(hit);
  }

  // ignore collisions while standing still
  if(state != JUMPING)
    return;

  // check if we hit the floor while falling
  if(hit.bottom && physic.get_velocity_y() > 0 ) {
    set_state(STANDING);
  }
  // check if we hit the roof while climbing
  if(hit.top) {
    physic.set_velocity_y(0);
  }

  // check if we hit left or right while moving in either direction
  if(hit.left || hit.right) {
    dir = dir == LEFT ? RIGHT : LEFT;
    sprite->set_action(dir == LEFT ? "jumping-left" : "jumping-right");
    physic.set_velocity_x(-0.25*physic.get_velocity_x());
  }
}

HitResponse
SkullyHop::collision_badguy(BadGuy& , const CollisionHit& hit)
{
  // behaviour for badguy collisions is the same as for collisions with solids
  collision_solid(hit);

  return CONTINUE;
}

void
SkullyHop::active_update(float elapsed_time)
{
  BadGuy::active_update(elapsed_time);

  // no change if frozen
  if (frozen)
    return;

  // charge when fully recovered
  if ((state == STANDING) && (recover_timer.check())) {
    set_state(CHARGING);
    return;
  }

  // jump as soon as charging animation completed
  if ((state == CHARGING) && (sprite->animation_done())) {
    set_state(JUMPING);
    return;
  }
}

void
SkullyHop::unfreeze()
{
  BadGuy::unfreeze();
  initialize();
}

bool
SkullyHop::is_freezable() const
{
  return true;
}

/* EOF */
