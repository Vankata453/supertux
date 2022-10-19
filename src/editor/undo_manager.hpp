//  SuperTux
//  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
//                2022 Vankata453
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

#ifndef HEADER_SUPERTUX_EDITOR_UNDO_MANAGER_HPP
#define HEADER_SUPERTUX_EDITOR_UNDO_MANAGER_HPP

#include <vector>
#include <string>

#include "editor/action.hpp"

class Level;

class UndoManager
{
  friend class EditorUndoStackMenu;

public:
  UndoManager();

  void push_action(std::unique_ptr<EditorAction> action);
  void cleanup();
  void reset_index();

  void undo(int steps = 1);
  void redo(int steps = 1);

  bool has_unsaved_changes() const;

private:
  void push_undo_stack(std::unique_ptr<EditorAction> action);

private:
  int m_index_pos;
  std::vector<std::unique_ptr<EditorAction>> m_undo_stack;
  std::vector<std::unique_ptr<EditorAction>> m_redo_stack;

private:
  UndoManager(const UndoManager&) = delete;
  UndoManager& operator=(const UndoManager&) = delete;
};

#endif

/* EOF */
