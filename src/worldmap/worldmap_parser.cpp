//  SuperTux
//  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
//                2023 Vankata453
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

#include "worldmap/worldmap_parser.hpp"

#include "physfs/util.hpp"
#include "supertux/d_scope.hpp"
#include "supertux/game_object_factory.hpp"
#include "util/file_system.hpp"
#include "worldmap/spawn_point.hpp"
#include "worldmap/worldmap.hpp"

namespace worldmap {

WorldMapParser::WorldMapParser(WorldMap& worldmap) :
  LevelParser(worldmap, true, false)
{
}

WorldMap&
WorldMapParser::get_worldmap() const
{
  return static_cast<WorldMap&>(m_level);
}

void
WorldMapParser::load(const std::string& filepath)
{
  get_worldmap().m_map_filename = physfsutil::realpath(filepath);
  get_worldmap().m_levels_path = FileSystem::dirname(get_worldmap().m_map_filename);

  LevelParser::load(filepath);
}

void
WorldMapParser::add_sector(const ReaderMapping& reader)
{
  get_worldmap().add_sector(WorldMapSectorParser::from_reader(get_worldmap(), reader));
}



std::unique_ptr<WorldMapSector>
WorldMapSectorParser::from_reader(WorldMap& worldmap, const ReaderMapping& reader)
{
  auto sector = std::make_unique<WorldMapSector>(worldmap);
  BIND_WORLDMAP_SECTOR(*sector);
  WorldMapSectorParser parser(*sector);
  parser.parse(reader);
  return sector;
}


WorldMapSectorParser::WorldMapSectorParser(WorldMapSector& sector) :
  SectorParser(sector, false)
{
}

WorldMapSector&
WorldMapSectorParser::get_sector() const
{
  return static_cast<WorldMapSector&>(m_sector);
}

bool
WorldMapSectorParser::parse_object_additional(const std::string& name, const ReaderMapping& reader)
{
  if (name == "worldmap-spawnpoint") // Custom rule for adding spawnpoints
  {
    get_sector().m_spawnpoints.push_back(std::make_unique<SpawnPoint>(reader));
    return true;
  }

  // Proceed adding the object only if it's flagged as allowed for worldmaps
  return !GameObjectFactory::instance().has_params(name, ObjectFactory::RegisteredObjectParam::OBJ_PARAM_WORLDMAP);
}

} // namespace worldmap

/* EOF */
