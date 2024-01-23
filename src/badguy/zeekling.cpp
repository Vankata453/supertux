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

#include "editor/editor.hpp"
#include "math/random_generator.hpp"
#include "object/player.hpp"
#include "sprite/sprite.hpp"
#include "supertux/globals.hpp"
#include "supertux/object_factory.hpp"
#include "supertux/resources.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"

const float Zeekling::s_status_shift = 40.0f;

Zeekling::Zeekling(const ReaderMapping& reader) :
  BadGuy(reader, "images/creatures/zeekling/zeekling.sprite"),
  speed(),
  diveRecoverTimer(),
  state(),
  last_player(0),
  dive_variables(),
  display_dive_variables(false),
  show_estimates(false)
{
  state = FLYING;
  speed = gameRandom.rand(130, 171);
  if (!reader.get("display_dive_variables", display_dive_variables))
    display_dive_variables = false;
  reader.get("show-estimates", show_estimates);

  physic.enable_gravity(false);
}

Zeekling::Zeekling(const Vector& pos, Direction d) :
  BadGuy(pos, d, "images/creatures/zeekling/zeekling.sprite"),
  speed(),
  diveRecoverTimer(),
  state(),
  last_player(0),
  dive_variables(),
  display_dive_variables(false),
  show_estimates(false)
{
  state = FLYING;
  speed = gameRandom.rand(130, 171);
  physic.enable_gravity(false);
}

ObjectSettings
Zeekling::get_settings()
{
  ObjectSettings result = BadGuy::get_settings();

  result.options.push_back(ObjectOption(MN_NUMFIELD, _("Speed"), &speed,
                                         "speed"));
  result.options.push_back(ObjectOption(MN_TOGGLE, _("Display dive variables"), &display_dive_variables,
                                         "display_dive_variables"));
  result.options.push_back(ObjectOption(MN_TOGGLE, _("Show Position Estimates"), &show_estimates,
                                         "show-estimates"));

  return result;
}

void
Zeekling::after_editor_set()
{
  BadGuy::after_editor_set();

  if (!(speed >= 130 && speed <= 171))
    log_warning << "Wrong zeekling speed property range." << std::endl;
}

float
Zeekling::draw_status(DrawingContext& context, const float& pos_x)
{
  if (!display_dive_variables) return 0.0f;

  const float x_pos = pos_x + s_status_shift;

  context.draw_text(Resources::small_font, "Zeekling:",
                    Vector(x_pos, 15), ALIGN_LEFT, LAYER_GUI, Color::RED);
  context.draw_text(Resources::small_font, "name: " + name,
                    Vector(x_pos, 35), ALIGN_LEFT, LAYER_GUI, Color::YELLOW);
  context.draw_text(Resources::small_font, "speed: " + std::to_string(speed),
                    Vector(x_pos, 45), ALIGN_LEFT, LAYER_GUI);

  context.draw_text(Resources::small_font, "last_player_pos: " + dive_variables.last_player_pos.to_string(),
                    Vector(x_pos, 55), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "last_self_pos: " + dive_variables.last_self_pos.to_string(),
                    Vector(x_pos, 65), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "player_pos: " + dive_variables.player_pos.to_string(),
                    Vector(x_pos, 75), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "player_mov: " + dive_variables.player_mov.to_string(),
                    Vector(x_pos, 85), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "self_pos: " + dive_variables.self_pos.to_string(),
                    Vector(x_pos, 95), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "self_mov: " + dive_variables.self_mov.to_string(),
                    Vector(x_pos, 105), ALIGN_LEFT, LAYER_GUI);

  context.draw_text(Resources::small_font, "vy: " + std::to_string(dive_variables.vy),
                    Vector(x_pos, 115), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "height: " + std::to_string(dive_variables.height),
                    Vector(x_pos, 125), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "relSpeed: " + std::to_string(dive_variables.relSpeed),
                    Vector(x_pos, 135), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "estFrames: " + std::to_string(dive_variables.estFrames),
                    Vector(x_pos, 145), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "estPx: " + std::to_string(dive_variables.estPx),
                    Vector(x_pos, 155), ALIGN_LEFT, LAYER_GUI);
  context.draw_text(Resources::small_font, "estBx: " + std::to_string(dive_variables.estBx),
                    Vector(x_pos, 165), ALIGN_LEFT, LAYER_GUI);

  return s_status_shift + 300.0f;
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

  if (Editor::is_active() || !show_estimates) return;

  auto player = get_nearest_player();
  if (!player) return;

  Vector est_player_pos(dive_variables.estPx, player->get_pos().y);
  Vector est_self_pos(dive_variables.estBx, get_pos().y);

  context.draw_filled_rect(est_player_pos, player->get_bbox().get_size().as_vector(), Color(0, 0, 1, 1), LAYER_OBJECTS + 1);
  context.draw_filled_rect(est_self_pos, get_bbox().get_size().as_vector(), Color(1, 0, 1, 1), LAYER_OBJECTS + 1);

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
    dive_variables.player_pos = player->get_pos();
    dive_variables.player_mov = (dive_variables.player_pos - dive_variables.last_player_pos);
    dive_variables.self_pos = bbox.p1;
    dive_variables.self_mov = (dive_variables.self_pos - dive_variables.last_self_pos);

    // new vertical speed to test with
    dive_variables.vy = 2*fabsf(dive_variables.self_mov.x);

    // do not dive if we are not above the player
    dive_variables.height = dive_variables.player_pos.y - dive_variables.self_pos.y;
    if (dive_variables.height <= 0) return false;

    // do not dive if we are too far above the player
    if (dive_variables.height > 512) return false;

    // do not dive if we would not descend faster than the player
    dive_variables.relSpeed = dive_variables.vy - dive_variables.player_mov.y;
    if (dive_variables.relSpeed <= 0) return false;

    // guess number of frames to descend to same height as player
    dive_variables.estFrames = dive_variables.height / dive_variables.relSpeed;

    // guess where the player would be at this time
    dive_variables.estPx = (dive_variables.player_pos.x + (dive_variables.estFrames * dive_variables.player_mov.x));

    // guess where we would be at this time
    dive_variables.estBx = (dive_variables.self_pos.x + (dive_variables.estFrames * dive_variables.self_mov.x));

    // near misses are OK, too
    if (fabsf(dive_variables.estPx - dive_variables.estBx) < 8)
    {
      if (show_estimates)
        log_warning << "Diving on " << dive_variables.self_pos << std::endl;

      return true;
    }
  }

  // update last player tracked, as well as our positions
  last_player = player;
  if (player) {
    dive_variables.last_player_pos = player->get_pos();
    dive_variables.last_self_pos = bbox.p1;
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
