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

#include "badguy/flyingsnowball.hpp"

#include "math/random_generator.hpp"
#include "object/sprite_particle.hpp"
#include "object/player.hpp"
#include "supertux/object_factory.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"

namespace {
const float PUFF_INTERVAL_MIN = 4.0f; /**< spawn new puff of smoke at most that often */
const float PUFF_INTERVAL_MAX = 8.0f; /**< spawn new puff of smoke at least that often */
}

FlyingSnowBall::FlyingSnowBall(const ReaderMapping& reader) :
  BadGuy(reader, "images/creatures/flying_snowball/flying_snowball.sprite"),
  normal_propeller_speed(-100),
  puff_timer_time(-100),
  puff_timer()
{
  if (!reader.get("normal_propeller_speed", normal_propeller_speed))
    normal_propeller_speed = -100;
  if (!reader.get("puff_timer", puff_timer_time))
    puff_timer_time = -100;
  physic.enable_gravity(true);
}

FlyingSnowBall::FlyingSnowBall(const Vector& pos) :
  BadGuy(pos, "images/creatures/flying_snowball/flying_snowball.sprite"),
  normal_propeller_speed(-100),
  puff_timer_time(-100),
  puff_timer()
{
  physic.enable_gravity(true);
}

ObjectSettings
FlyingSnowBall::get_settings()
{
  ObjectSettings result = BadGuy::get_settings();

  result.options.push_back(ObjectOption(MN_NUMFIELD, _("Normal propeller speed (Initial)"), &normal_propeller_speed,
                                         "normal_propeller_speed"));
  result.options.push_back(ObjectOption(MN_NUMFIELD, _("Puff timer (Initial)"), &puff_timer_time,
                                         "puff_timer"));

  return result;
}

void
FlyingSnowBall::after_editor_set()
{
  BadGuy::after_editor_set();

  std::vector<bool> are_valid;

  are_valid.push_back((normal_propeller_speed >= 0.95 && normal_propeller_speed <= 1.05) || normal_propeller_speed == -100);
  are_valid.push_back((puff_timer_time >= PUFF_INTERVAL_MIN && puff_timer_time <= PUFF_INTERVAL_MAX) || puff_timer_time == -100);

  if (std::find(are_valid.begin(), are_valid.end(), false) != are_valid.end())
    log_warning << "Wrong flying snowball property range." << std::endl;
}

void
FlyingSnowBall::initialize()
{
  sprite->set_action(dir == LEFT ? "left" : "right");
}

void
FlyingSnowBall::activate()
{
  if (puff_timer_time == -100) puff_timer_time = gameRandom.randf(PUFF_INTERVAL_MIN, PUFF_INTERVAL_MAX);
  puff_timer.start(puff_timer_time);
  if (normal_propeller_speed == -100) normal_propeller_speed = gameRandom.randf(0.95, 1.05);
}

bool
FlyingSnowBall::collision_squished(GameObject& object)
{
  sprite->set_action(dir == LEFT ? "squished-left" : "squished-right");
  physic.set_acceleration_y(0);
  physic.set_velocity_y(0);
  kill_squished(object);
  return true;
}

void
FlyingSnowBall::collision_solid(const CollisionHit& hit)
{
  if(hit.top || hit.bottom) {
    physic.set_velocity_y(0);
  }
}

void
FlyingSnowBall::active_update(float elapsed_time)
{

  const float grav = Sector::current()->get_gravity() * 100.0f;
  if (get_pos().y > start_position.y + 2*32) {

    // Flying too low - increased propeller speed
    physic.set_acceleration_y(-grav*1.2);

    physic.set_velocity_y(physic.get_velocity_y() * 0.99);

  } else if (get_pos().y < start_position.y - 2*32) {

    // Flying too high - decreased propeller speed
    physic.set_acceleration_y(-grav*0.8);

    physic.set_velocity_y(physic.get_velocity_y() * 0.99f);

  } else {

    // Flying at acceptable altitude - normal propeller speed
    physic.set_acceleration_y(-grav*normal_propeller_speed);

  }

  movement=physic.get_movement(elapsed_time);

  auto player = get_nearest_player();
  if (player) {
    dir = (player->get_pos().x > get_pos().x) ? RIGHT : LEFT;
    sprite->set_action(dir == LEFT ? "left" : "right");
  }

  // spawn smoke puffs
  if (puff_timer.check()) {
    Vector ppos = bbox.get_middle();
    Vector pspeed = Vector(gameRandom.randf(-10, 10), 150);
    Vector paccel = Vector(0,0);
    Sector::current()->add_object(std::make_shared<SpriteParticle>("images/objects/particles/smoke.sprite",
                                                                   "default",
                                                                   ppos, ANCHOR_MIDDLE, pspeed, paccel,
                                                                   LAYER_OBJECTS-1));
    puff_timer.start(gameRandom.randf(PUFF_INTERVAL_MIN, PUFF_INTERVAL_MAX));

    normal_propeller_speed = gameRandom.randf(0.95, 1.05);
    physic.set_velocity_y(physic.get_velocity_y() - 50);
  }
}

/* EOF */
