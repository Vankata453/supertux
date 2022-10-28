//  SuperTux
//  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_PATH_GAMEOBJECT_HPP
#define HEADER_SUPERTUX_OBJECT_PATH_GAMEOBJECT_HPP

#include "sprite/sprite_ptr.hpp"
#include "supertux/game_object.hpp"
#include "math/fwd.hpp"

class Path;

enum class PathStyle
{
  NONE,
  SOLID
};

class PathGameObject : public GameObject
{
public:
  PathGameObject();
  PathGameObject(const Vector& pos);
  PathGameObject(const ReaderMapping& mapping, bool backward_compatibility_hack=false);
  ~PathGameObject() override;

  virtual void update(float dt_sec) override;
  virtual void draw(DrawingContext& context) override;

  static std::string class_name() { return "path"; }
  virtual std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Path"); }
  virtual std::string get_display_name() const override { return display_name(); }

  virtual const std::string get_icon_path() const override {
    return "images/engine/editor/path.png";
  }

  virtual void editor_update() override;
  virtual void editor_select() override;
  virtual void editor_deselect() override;

  virtual void remove_me() override;
  virtual void force_remove();

  virtual void on_flip(float height) override;

  virtual ObjectSettings get_settings() override;

  Path& get_path() const { return *m_path; }
  Path* get_path_ptr() const { return m_path.get(); }

  void copy_into(PathGameObject& other);

  PathObject* get_path_object() const;

private:
  /** Removes the object if the path is not referenced anywhere */
  void check_references();

private:
  std::unique_ptr<Path> m_path;
  PathStyle m_style;
  SpritePtr m_edge_sprite;
  SpritePtr m_node_sprite;

private:
  PathGameObject(const PathGameObject&) = delete;
  PathGameObject& operator=(const PathGameObject&) = delete;
};

#endif

/* EOF */
