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
#include "object/tilemap.hpp"
#include "supertux/game_object_factory.hpp"
#include "supertux/level.hpp"
#include "supertux/moving_object.hpp"
#include "supertux/sector_parser.hpp"
#include "util/reader_mapping.hpp"
#include "util/reader_object.hpp"
#include "util/writer.hpp"

// Base class for all editor actions.
EditorAction::EditorAction()
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


// Base class for all object actions.
ObjectAction::ObjectAction(std::string sector, UID uid) :
  EditorAction(),
  m_sector(sector),
  m_uid(uid)
{
}

void
ObjectAction::undo()
{
}

void
ObjectAction::redo()
{
  undo(); // Mirror the undo action.
}

GameObject*
ObjectAction::get_object_by_uid()
{
  return get_sector(m_sector)->get_object_by_uid(m_uid);
}


TilePlaceAction::TilePlaceAction(std::string sector, UID tilemap_uid, std::vector<uint32_t> old_tiles) :
  ObjectAction(sector, tilemap_uid),
  m_tilemap_tiles(old_tiles)
{
}

void
TilePlaceAction::undo()
{
  auto* tilemap = dynamic_cast<TileMap*>(get_object_by_uid());

  const auto current_tiles = tilemap->get_tiles();
  tilemap->set_tiles(m_tilemap_tiles);
  m_tilemap_tiles = current_tiles; // Swap values to allow for redo.

  go_to_sector(m_sector);
}


ObjectCreateAction::ObjectCreateAction(std::string sector, std::string object_class, UID uid, Vector target_pos, Direction direction, bool layer) :
  ObjectAction(sector, uid),
  m_layer(layer),
  m_create_mode(CREATE_ATTRIBUTES),
  m_object_class(object_class),
  m_target_pos(target_pos),
  m_direction(direction)
{
}

ObjectCreateAction::ObjectCreateAction(std::string sector, std::unique_ptr<ReaderDocument> object_reader, UID uid, bool layer) :
  ObjectAction(sector, uid),
  m_layer(layer),
  m_create_mode(CREATE_READER),
  m_object_reader(std::move(object_reader))
{
}

void
ObjectCreateAction::undo()
{
  get_object_by_uid()->editor_delete();
  go_to_sector(m_sector);
  if (m_layer) Editor::current()->refresh_layers();
  Editor::current()->delete_markers();
}

void
ObjectCreateAction::redo()
{
  BIND_SECTOR(*get_sector(m_sector));

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


ObjectDeleteAction::ObjectDeleteAction(std::string sector, GameObject& object, bool layer, bool auto_delete, bool auto_save) :
  ObjectAction(sector, object.get_uid()),
  m_layer(layer),
  m_object_reader(auto_save ? std::make_unique<ReaderDocument>(save_object_to_reader(&object)) : nullptr)
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
  get_object_by_uid()->editor_delete();

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
  ObjectAction(sector, uid),
  m_old_values(old_values)
{
}

void
ObjectOptionChangeAction::undo()
{
  BIND_SECTOR(*get_sector(m_sector));

  auto* object = get_object_by_uid();

  swap_option_values(object, m_old_values); // Swap values to allow for redo.

  go_to_sector(m_sector);
  object->after_editor_set();
  Editor::current()->update_layer_tip();
}

// Static utilities

// Perform a swap between old and new option values.
void
ObjectOptionChangeAction::swap_option_values(GameObject* object, std::map<std::string, std::string>& values)
{
  const auto settings = object->get_settings();

  for (auto& value : values)
  {
    auto* option = settings.get_option_by_key(value.first);

    const std::string current_value = option->to_string();
    option->from_string(value.second);
    value.second = current_value;
  }
}


ObjectMoveAction::ObjectMoveAction(std::string sector, UID uid, Rectf old_rect):
  ObjectAction(sector, uid),
  m_rect(old_rect)
{
}

void
ObjectMoveAction::undo()
{
  BIND_SECTOR(*get_sector(m_sector));

  auto* object = dynamic_cast<MovingObject*>(get_object_by_uid());

  if (!object)
    throw std::runtime_error("Cannot find moving object to undo/redo move action.");

  auto current_rect = object->get_bbox();
  object->move_to(m_rect.p1());
  object->set_size(m_rect.get_width(), m_rect.get_height());
  m_rect = current_rect; // Swap values to allow for redo.

  go_to_sector(m_sector);
}


TileMapResizeAction::TileMapResizeAction(std::string sector, UID tilemap_uid) :
  ObjectDeleteAction(sector, *get_sector(sector)->get_object_by_uid(tilemap_uid), true, false)
{
}

void
TileMapResizeAction::undo()
{
  auto* tilemap = get_object_by_uid();
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
  ObjectAction::redo();
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
      log_warning << "Couldn't undo resize action for a tilemap: " << err.what() << std::endl;
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
      log_warning << "Couldn't redo resize action for a tilemap: " << err.what() << std::endl;
    }
  }
}


PathObjectDeleteAction::PathObjectDeleteAction(std::string sector, GameObject& object, bool layer, bool auto_delete) :
  ObjectDeleteAction(sector, object, layer, false),
  m_path_delete_action()
{
  auto* path_game_object = dynamic_cast<PathObject*>(&object)->get_path_gameobject();

  // Delete objects within the delete action constructor to make sure writer has saved the object beforehand.
  if (auto_delete) object.editor_delete();

  m_path_delete_action = std::make_unique<ObjectDeleteAction>(m_sector, *path_game_object, false, false);
}

void
PathObjectDeleteAction::undo()
{
  try
  {
    m_path_delete_action->undo();
  }
  catch (std::exception& err)
  {
    log_warning << "Couldn't undo path delete action." << std::endl;
  }

  ObjectDeleteAction::undo();

  auto* path_object = dynamic_cast<PathObject*>(get_object_by_uid());
  path_object->editor_set_path_by_uid(m_path_delete_action->get_uid());
}


PathNodeModifyAction::PathNodeModifyAction(std::string sector, NodeMarker* node_marker, bool auto_save) :
  ObjectAction(sector, node_marker->get_path()->get_path_gameobject()->get_uid()),
  m_path_nodes()
{
  if (auto_save) m_path_nodes = node_marker->get_path()->get_nodes();
}

void
PathNodeModifyAction::undo()
{
  auto& path = dynamic_cast<PathGameObject*>(get_object_by_uid())->get_path();

  auto current_nodes = path.get_nodes();
  path.m_nodes = m_path_nodes;
  m_path_nodes = current_nodes; // Swap values to allow for redo.

  Editor::current()->update_node_iterators();
}


PathNodeOptionChangeAction::PathNodeOptionChangeAction(std::string sector, NodeMarker* node_marker, std::map<std::string, std::string> old_values) :
  PathNodeModifyAction(sector, node_marker, false)
{
  ObjectOptionChangeAction::swap_option_values(node_marker, old_values); // Temporarily apply old values to save object data.
  m_path_nodes = node_marker->get_path()->get_nodes();
  ObjectOptionChangeAction::swap_option_values(node_marker, old_values); // Revert to current object values.
}


PathNodeMoveAction::PathNodeMoveAction(std::string sector, NodeMarker* node_marker, std::vector<Path::Node> old_nodes) :
  PathNodeModifyAction(sector, node_marker, false)
{
  m_path_nodes = old_nodes;
}

/* EOF */
