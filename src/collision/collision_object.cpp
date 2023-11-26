//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//                2018 Ingo Ruhnke <grumbel@gmail.com>
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

#include "collision/collision_object.hpp"

#include "collision/collision_listener.hpp"
#include "supertux/game_object.hpp"

CollisionObject::CollisionObject(CollisionGroup group, CollisionListener& listener) :
  m_listener(listener),
  m_bbox(),
  m_movement(),
  m_group(group),
  m_dest()
{
}

void
CollisionObject::collision_solid(const CollisionHit& hit)
{
  m_listener.collision_solid(hit);
}

bool
CollisionObject::collides(CollisionObject& other, const CollisionHit& hit) const
{
  return m_listener.collides(dynamic_cast<GameObject&>(other.m_listener), hit);
}

HitResponse
CollisionObject::collision(CollisionObject& other, const CollisionHit& hit)
{
  return m_listener.collision(dynamic_cast<GameObject&>(other.m_listener), hit);
}

void
CollisionObject::collision_tile(uint32_t tile_attributes)
{
  m_listener.collision_tile(tile_attributes);
}

bool
CollisionObject::is_valid() const
{
  return m_listener.listener_is_valid();
}

/* EOF */
