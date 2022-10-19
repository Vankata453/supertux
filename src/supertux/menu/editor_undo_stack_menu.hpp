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

#ifndef HEADER_SUPERTUX_SUPERTUX_MENU_EDITOR_UNDO_STACK_MENU_HPP
#define HEADER_SUPERTUX_SUPERTUX_MENU_EDITOR_UNDO_STACK_MENU_HPP

#include "gui/menu.hpp"

#include "gui/menu_item.hpp"

class EditorUndoStackMenu final : public Menu
{
public:
  enum
  {
    UNDO_STACK,
    REDO_STACK
  };

private:
  static const int s_actions_on_page;

private:
  const int m_stack_type;
  int m_current_page;
  int m_stack_size;

public:
  EditorUndoStackMenu(int stack_type = UNDO_STACK);
  ~EditorUndoStackMenu() override;

  void rebuild_menu();
  void menu_action(MenuItem& item) override;

private:
  EditorUndoStackMenu(const EditorUndoStackMenu&) = delete;
  EditorUndoStackMenu& operator=(const EditorUndoStackMenu&) = delete;
};

#endif

/* EOF */
