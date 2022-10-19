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

#ifndef HEADER_SUPERTUX_EDITOR_ACTION_HPP
#define HEADER_SUPERTUX_EDITOR_ACTION_HPP

#include <memory>

#include "supertux/sector.hpp"
#include "util/gettext.hpp"
#include "util/reader_document.hpp"

class EditorAction
{
public:
  EditorAction();
  virtual ~EditorAction() = default;

  // Operations
  virtual void undo();
  virtual void redo();

  // Property functions
  virtual std::string get_name() const { return _("Unknown action"); }

protected:
  // Static utilities
  static void go_to_sector(const std::string& sector_name);
  static Sector* get_sector(const std::string& sector_name);

private:
  EditorAction(const EditorAction&) = delete;
  EditorAction& operator=(const EditorAction&) = delete;
};



class SectorCreateAction : public EditorAction
{
private:
  const int m_index;
  const std::string m_name;

public:
  SectorCreateAction(int index, std::string name);
  virtual ~SectorCreateAction() override = default;

  virtual void undo() override;
  virtual void redo() override;

  virtual std::string get_name() const override { return _("Create sector"); }

private:
  SectorCreateAction(const SectorCreateAction&) = delete;
  SectorCreateAction& operator=(const SectorCreateAction&) = delete;
};


class SectorDeleteAction : public EditorAction
{
private:
  const int m_index;
  std::unique_ptr<Sector> m_sector;

public:
  SectorDeleteAction(int index, std::unique_ptr<Sector> sector);
  virtual ~SectorDeleteAction() override = default;

  virtual void undo() override;
  virtual void redo() override;

  virtual std::string get_name() const override { return _("Delete sector"); }

private:
  SectorDeleteAction(const SectorDeleteAction&) = delete;
  SectorDeleteAction& operator=(const SectorDeleteAction&) = delete;
};


class SectorPropertyChangeAction : public EditorAction
{
private:
  Sector::Properties m_properties;
  Sector::Properties m_og_properties;

public:
  SectorPropertyChangeAction(Sector::Properties properties, Sector::Properties og_properties);
  virtual ~SectorPropertyChangeAction() override = default;

  virtual void undo() override;
  virtual void redo() override;

  virtual std::string get_name() const override { return _("Sector property change"); }

private:
  SectorPropertyChangeAction(const SectorPropertyChangeAction&) = delete;
  SectorPropertyChangeAction& operator=(const SectorPropertyChangeAction&) = delete;
};


class TilePlaceAction : public EditorAction
{
private:
  const std::string m_sector;
  UID m_uid;
  std::vector<uint32_t> m_tilemap_tiles;

public:
  TilePlaceAction(std::string sector, UID tilemap_uid, std::vector<uint32_t> old_tiles);
  virtual ~TilePlaceAction() override = default;

  virtual void undo() override;
  virtual void redo() override;

  virtual std::string get_name() const override { return _("Place tile/s"); }

private:
  TilePlaceAction(const TilePlaceAction&) = delete;
  TilePlaceAction& operator=(const TilePlaceAction&) = delete;
};


class ObjectCreateAction : public EditorAction
{
private:
  enum // Ways in which the object can be created, based on arguments.
  {
    CREATE_ATTRIBUTES,
    CREATE_READER
  };

  const std::string m_sector;
  UID m_uid;
  const bool m_layer;

  const int m_create_mode;

  // For attributes creation.
  const std::string m_object_class;
  Vector m_target_pos;
  Direction m_direction;

  // For reader creation.
  std::unique_ptr<ReaderDocument> m_object_reader;

public:
  ObjectCreateAction(std::string sector, std::string object_class, UID uid, Vector target_pos, Direction direction, bool layer = false);
  ObjectCreateAction(std::string sector, std::unique_ptr<ReaderDocument> object_reader, UID uid, bool layer = false);
  virtual ~ObjectCreateAction() override = default;

  virtual void undo() override;
  virtual void redo() override;

  virtual std::string get_name() const override { return _("Create object"); }

private:
  ObjectCreateAction(const ObjectCreateAction&) = delete;
  ObjectCreateAction& operator=(const ObjectCreateAction&) = delete;
};


class ObjectDeleteAction : public EditorAction
{
public:
  ObjectDeleteAction(std::string sector, GameObject& object, bool layer = false, bool auto_delete = true);
  virtual ~ObjectDeleteAction() override = default;

  virtual void undo() override;
  virtual void redo() override;

protected:
  virtual ReaderDocument save_object_to_reader(GameObject* object);

  const std::string m_sector;
  UID m_uid;
  const bool m_layer;
  std::unique_ptr<ReaderDocument> m_object_reader;

  virtual std::string get_name() const override { return _("Delete object"); }

private:
  ObjectDeleteAction(const ObjectDeleteAction&) = delete;
  ObjectDeleteAction& operator=(const ObjectDeleteAction&) = delete;
};


class ObjectOptionChangeAction : public EditorAction
{
private:
  const std::string m_sector;
  UID m_uid;
  std::map<std::string, std::string> m_old_values;

public:
  ObjectOptionChangeAction(std::string sector, UID uid, std::map<std::string, std::string> old_values);
  virtual ~ObjectOptionChangeAction() override = default;

  virtual void undo() override;
  virtual void redo() override;

  virtual std::string get_name() const override { return _("Change object option/s"); }

private:
  ObjectOptionChangeAction(const ObjectOptionChangeAction&) = delete;
  ObjectOptionChangeAction& operator=(const ObjectOptionChangeAction&) = delete;
};


class ObjectMoveAction : public EditorAction
{
private:
  const std::string m_sector;
  UID m_uid;
  Rectf m_rect;

public:
  ObjectMoveAction(std::string sector, UID uid, Rectf old_rect);
  virtual ~ObjectMoveAction() override = default;

  virtual void undo() override;
  virtual void redo() override;

  virtual std::string get_name() const override { return _("Move/Resize object"); }

private:
  ObjectMoveAction(const ObjectMoveAction&) = delete;
  ObjectMoveAction& operator=(const ObjectMoveAction&) = delete;
};


class TileMapResizeAction : public ObjectDeleteAction
{
public:
  TileMapResizeAction(std::string sector, UID tilemap_uid);

  virtual void undo() override;
  virtual void redo() override;

  virtual std::string get_name() const override { return _("Resize tilemap"); }

private:
  TileMapResizeAction(const TileMapResizeAction&) = delete;
  TileMapResizeAction& operator=(const TileMapResizeAction&) = delete;
};


class SectorResizeAction : public EditorAction
{
private:
  std::vector<std::unique_ptr<TileMapResizeAction>> m_actions;

public:
  SectorResizeAction(std::string name);
  virtual ~SectorResizeAction() override = default;

  virtual void undo() override;
  virtual void redo() override;

  virtual std::string get_name() const override { return _("Resize sector"); }

private:
  SectorResizeAction(const SectorResizeAction&) = delete;
  SectorResizeAction& operator=(const SectorResizeAction&) = delete;
};

#endif

/* EOF */
