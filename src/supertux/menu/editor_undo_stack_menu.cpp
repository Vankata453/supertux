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

#include "supertux/menu/editor_undo_stack_menu.hpp"

#include <fmt/format.h>

#include "editor/editor.hpp"
#include "gui/menu_manager.hpp"
#include "util/gettext.hpp"
#include "util/log.hpp"

const int EditorUndoStackMenu::s_actions_on_page = 10;

EditorUndoStackMenu::EditorUndoStackMenu(int stack_type) :
  m_stack_type(stack_type),
  m_current_page(1),
  m_stack_size(0)
{
  rebuild_menu();
}

EditorUndoStackMenu::~EditorUndoStackMenu()
{
  auto editor = Editor::current();

  if (editor == nullptr)
    return;

  editor->m_reactivate_request = true;
}

void
EditorUndoStackMenu::rebuild_menu()
{
  clear();

  auto* undo_manager = Editor::current()->get_undo_manager();
  if (!undo_manager)
  {
    MenuManager::instance().pop_menu();
    return;
  }
  auto& stack = m_stack_type == UNDO_STACK ? undo_manager->m_undo_stack : undo_manager->m_redo_stack;
  m_stack_size = static_cast<int>(stack.size());
  std::vector<std::string> stack_action_names;

  // Add names of actions from stack in reversed order for displaying.
  for (auto& action : stack)
  {
    stack_action_names.insert(stack_action_names.begin(), action->get_name());
  }

  // Add menu label.
  const std::string label = m_stack_type == UNDO_STACK ? _("Undo stack") : _("Redo stack");
  add_label(fmt::format(fmt::runtime(label + " ({})"), m_stack_size));
  add_hl();

  // List actions on the current page.
  if (!stack.empty())
  {
    for (int i = (m_current_page - 1) * s_actions_on_page;
         i < m_current_page * s_actions_on_page && i < m_stack_size; i++)
    {
      add_entry(i + 1, stack_action_names[i]);
    }
  }
  else
  {
    add_inactive(_("Empty"));
  }
  add_hl();

  // Add page navigation buttons.
  if (m_current_page > 1)
    add_entry(-1, _("Previous page"));
  else
    add_inactive(_("Previous page"));

  if (m_stack_size > m_current_page * s_actions_on_page)
    add_entry(-2, _("Next page"));
  else
    add_inactive(_("Next page"));

  add_hl();

  add_back(_("Back"));
}

void
EditorUndoStackMenu::menu_action(MenuItem& item)
{
  const int id = item.get_id();
  switch (id)
  {
    case -1: // Previous page button.
    {
      m_current_page--;
      rebuild_menu();
      return;
    }
    case -2: // Next page button.
    {
      m_current_page++;
      rebuild_menu();
      return;
    }
  }

  if (id <= 0 || id > m_stack_size)
  {
    log_warning << "Invalid action or menu ID of pressed item." << std::endl;
    return;
  }

  // Undo/redo action is pressed.
  if (m_stack_type == UNDO_STACK)
    Editor::current()->undo(id);
  else if (m_stack_type == REDO_STACK)
    Editor::current()->redo(id);
  else
    log_warning << "Invalid stack type." << std::endl;

  MenuManager::instance().pop_menu();
}

/* EOF */
