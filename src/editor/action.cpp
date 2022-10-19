//  SuperTux
//  Copyright (C) 2022 Vankata453
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

#include "editor/action.hpp"

#include "editor/editor.hpp"
#include "editor/worldmap_objects.hpp"
#include "object/path.hpp"
#include "object/tilemap.hpp"
#include "supertux/game_object_factory.hpp"
#include "supertux/level.hpp"
#include "supertux/moving_object.hpp"
#include "supertux/sector_parser.hpp"
#include "util/reader_mapping.hpp"
#include "util/reader_object.hpp"
#include "util/writer.hpp"

EditorAction::EditorAction()
{
}

// Operations

void
EditorAction::undo()
{
}

void
EditorAction::redo()
{
}

// Static utilities

// Allows going to the sector where an action originated from.
void
EditorAction::go_to_sector(const std::string& sector_name)
{
  if (Editor::current()->get_sector()->get_name() != sector_name)
  {
    Editor::current()->set_sector(sector_name);
  }
}

// Simple way to get a certain sector.
Sector*
EditorAction::get_sector(const std::string& sector_name)
{
  return Editor::current()->get_level()->get_sector(sector_name);
}



SectorCreateAction::SectorCreateAction(int index, std::string name) :
  EditorAction(),
  m_index(index),
  m_name(name)
{
}

void
SectorCreateAction::undo()
{
  const auto editor = Editor::current();
  if (editor->get_sector()->get_name() == m_name)
  {
    editor->set_sector(m_index == 0 ? 1 : 0);
  }
  const auto level = editor->get_level();
  level->m_sectors.erase(level->m_sectors.begin() + m_index);
}

void
SectorCreateAction::redo()
{
  const auto editor = Editor::current();
  const auto level = editor->get_level();

  auto new_sector = SectorParser::from_nothing(*level);
  if (!new_sector)
  {
    throw std::runtime_error("Failed to re-create new sector.");
  }
  new_sector->set_name(m_name);

  level->add_sector(std::move(new_sector), m_index);
  editor->load_sector(m_name);
}


SectorDeleteAction::SectorDeleteAction(int index, std::unique_ptr<Sector> sector) :
  EditorAction(),
  m_index(index),
  m_sector(std::move(sector))
{
}

void
SectorDeleteAction::undo()
{
  const auto editor = Editor::current();
  const auto level = editor->get_level();

  const std::string sector_name = m_sector->get_name();
  level->add_sector(std::move(m_sector), m_index);
  editor->load_sector(sector_name);
}

void
SectorDeleteAction::redo()
{
  const auto editor = Editor::current();
  const auto level = editor->get_level();

  m_sector = std::move(level->m_sectors[m_index]); // Re-save the sector.

  if (editor->get_sector()->get_name() == m_sector->get_name())
  {
    editor->set_sector(m_index == 0 ? 1 : 0);
  }
  level->m_sectors.erase(level->m_sectors.begin() + m_index);
}


SectorPropertyChangeAction::SectorPropertyChangeAction(Sector::Properties properties, Sector::Properties og_properties) :
  EditorAction(),
  m_properties(properties),
  m_og_properties(og_properties)
{
}

void
SectorPropertyChangeAction::undo()
{
  get_sector(m_properties.name)->set_properties(m_og_properties);
}

void
SectorPropertyChangeAction::redo()
{
  get_sector(m_og_properties.name)->set_properties(m_properties);
}


TilePlaceAction::TilePlaceAction(std::string sector, UID tilemap_uid, std::vector<uint32_t> old_tiles) :
  EditorAction(),
  m_sector(sector),
  m_uid(tilemap_uid),
  m_tilemap_tiles(old_tiles)
{
}

void
TilePlaceAction::undo()
{
  auto* tilemap = dynamic_cast<TileMap*>(get_sector(m_sector)->get_object_by_uid(m_uid));

  const auto current_tiles = tilemap->get_tiles();
  tilemap->set_tiles(m_tilemap_tiles);
  m_tilemap_tiles = current_tiles; // Swap values to allow for redo.

  go_to_sector(m_sector);
}

void
TilePlaceAction::redo()
{
  undo(); // Mirror the undo action.
}


ObjectCreateAction::ObjectCreateAction(std::string sector, std::string object_class, UID uid, Vector target_pos, Direction direction, bool layer) :
  EditorAction(),
  m_sector(sector),
  m_uid(uid),
  m_layer(layer),
  m_create_mode(CREATE_ATTRIBUTES),
  m_object_class(object_class),
  m_target_pos(target_pos),
  m_direction(direction)
{
}

ObjectCreateAction::ObjectCreateAction(std::string sector, std::unique_ptr<ReaderDocument> object_reader, UID uid, bool layer) :
  EditorAction(),
  m_sector(sector),
  m_uid(uid),
  m_layer(layer),
  m_create_mode(CREATE_READER),
  m_object_reader(std::move(object_reader))
{
}

void
ObjectCreateAction::undo()
{
  get_sector(m_sector)->get_object_by_uid(m_uid)->editor_delete();
  go_to_sector(m_sector);
  if (m_layer) Editor::current()->refresh_layers();
  Editor::current()->delete_markers();
}

void
ObjectCreateAction::redo()
{
  std::unique_ptr<GameObject> object = nullptr;

  if (m_create_mode == CREATE_ATTRIBUTES)
  {
    object = GameObjectFactory::instance().create(m_object_class, m_target_pos, m_direction);
  }
  else if (m_create_mode == CREATE_READER)
  {
    auto root = m_object_reader->get_root();
    object = GameObjectFactory::instance().create(root.get_name(), root.get_mapping());
  }
  else
  {
    throw std::runtime_error("Unknown create mode for object.");
  }

  object->after_editor_set();
  auto* worldmap_object = dynamic_cast<worldmap_editor::WorldmapObject*>(object.get());
  if (worldmap_object)
    worldmap_object->move_to(worldmap_object->get_pos() / 32.0f);

  Editor::current()->delete_markers();
  auto& new_object = get_sector(m_sector)->add_object(std::move(object), &m_uid);
  go_to_sector(m_sector);
  if (m_layer) Editor::current()->add_layer(&new_object);
}


ObjectDeleteAction::ObjectDeleteAction(std::string sector, GameObject& object, bool layer, bool auto_delete) :
  EditorAction(),
  m_sector(sector),
  m_uid(object.get_uid()),
  m_layer(layer),
  m_object_reader(std::make_unique<ReaderDocument>(save_object_to_reader(&object)))
{
  // Delete objects within the delete action constructor to make sure writer has saved the object beforehand.
  if (auto_delete) object.editor_delete();
}

void
ObjectDeleteAction::undo()
{
  BIND_SECTOR(*get_sector(m_sector));

  auto root = m_object_reader->get_root();
  auto object = GameObjectFactory::instance().create(root.get_name(), root.get_mapping());

  object->after_editor_set();
  auto* worldmap_object = dynamic_cast<worldmap_editor::WorldmapObject*>(object.get());
  if (worldmap_object)
    worldmap_object->move_to(worldmap_object->get_pos() / 32.0f);

  Editor::current()->delete_markers();
  auto& new_object = get_sector(m_sector)->add_object(std::move(object), &m_uid);
  go_to_sector(m_sector);
  if (m_layer) Editor::current()->add_layer(&new_object);
}

void
ObjectDeleteAction::redo()
{
  get_sector(m_sector)->get_object_by_uid(m_uid)->editor_delete();

  go_to_sector(m_sector);
  if (m_layer) Editor::current()->refresh_layers();
  Editor::current()->delete_markers();
}

ReaderDocument
ObjectDeleteAction::save_object_to_reader(GameObject* object)
{
  std::stringstream stream;

  Writer writer(stream);
  writer.start_list(object->get_class_name());
  object->save(writer);
  writer.end_list(object->get_class_name());

  return ReaderDocument::from_stream(stream);
}


ObjectOptionChangeAction::ObjectOptionChangeAction(std::string sector, UID uid, std::map<std::string, std::string> old_values) :
  EditorAction(),
  m_sector(sector),
  m_uid(uid),
  m_old_values(old_values)
{
}

void
ObjectOptionChangeAction::undo()
{
  BIND_SECTOR(*get_sector(m_sector));

  auto* object = get_sector(m_sector)->get_object_by_uid(m_uid);
  const auto settings = object->get_settings();

  for (auto& value : m_old_values)
  {
    auto* option = settings.get_option_by_key(value.first);

    const std::string current_value = option->to_string();
    option->from_string(value.second);
    value.second = current_value; // Swap values to allow for redo.
  }

  go_to_sector(m_sector);
  object->after_editor_set();
  Editor::current()->update_layer_tip();
}

void
ObjectOptionChangeAction::redo()
{
  undo(); // Mirror the undo action.
}


ObjectMoveAction::ObjectMoveAction(std::string sector, UID uid, Rectf old_rect):
  EditorAction(),
  m_sector(sector),
  m_uid(uid),
  m_rect(old_rect)
{
}

void
ObjectMoveAction::undo()
{
  auto* object = dynamic_cast<MovingObject*>(get_sector(m_sector)->get_object_by_uid(m_uid));

  if (!object)
    throw std::runtime_error("Cannot find moving object to undo/redo move action.");

  auto current_rect = object->get_bbox();
  object->set_pos(m_rect.p1());
  object->set_size(m_rect.get_width(), m_rect.get_height());
  m_rect = current_rect; // Swap values to allow for redo.

  go_to_sector(m_sector);
}

void
ObjectMoveAction::redo()
{
  undo(); // Mirror the undo action.
}


TileMapResizeAction::TileMapResizeAction(std::string sector, UID tilemap_uid) :
  ObjectDeleteAction(sector, *get_sector(sector)->get_object_by_uid(tilemap_uid), true, false)
{
}

void
TileMapResizeAction::undo()
{
  auto* tilemap = get_sector(m_sector)->get_object_by_uid(m_uid);
  const auto current_tilemap = save_object_to_reader(tilemap);

  tilemap->editor_delete();

  // Mirror the object delete action undo function,
  // because it re-creates the object from reader data as well.
  ObjectDeleteAction::undo();

  // Save previous tilemap to allow for redo.
  m_object_reader = std::make_unique<ReaderDocument>(current_tilemap);

  go_to_sector(m_sector);
}

void
TileMapResizeAction::redo()
{
  undo(); // Mirror the undo action.
}


SectorResizeAction::SectorResizeAction(std::string name) :
  EditorAction(),
  m_actions()
{
  for (const auto& tilemap : get_sector(name)->get_objects_by_type<TileMap>())
  {
    m_actions.push_back(std::make_unique<TileMapResizeAction>(name, tilemap.get_uid()));
  }
}

void
SectorResizeAction::undo()
{
  for (const auto& action : m_actions)
  {
    try
    {
      action->undo();
    }
    catch (std::exception& err)
    {
      log_warning << "Couldn't undo resize action for a tilemap." << std::endl;
    }
  }
}

void
SectorResizeAction::redo()
{
  for (const auto& action : m_actions)
  {
    try
    {
      action->redo();
    }
    catch (std::exception& err)
    {
      log_warning << "Couldn't redo resize action for a tilemap." << std::endl;
    }
  }
}

/* EOF */
