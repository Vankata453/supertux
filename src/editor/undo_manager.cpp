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

#include "editor/undo_manager.hpp"

#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "util/log.hpp"

UndoManager::UndoManager() :
  m_index_pos(0),
  m_undo_stack(),
  m_redo_stack()
{
}

void
UndoManager::push_action(std::unique_ptr<EditorAction> action)
{
  if (m_undo_stack.empty())
  {
    push_undo_stack(std::move(action));
  }
  else if (action == m_undo_stack.back())
  {
    log_debug << "Skipping action save, because nothing has changed." << std::endl;
  }
  else // Level_snapshot changed
  {
    push_undo_stack(std::move(action));
  }
}

void
UndoManager::push_undo_stack(std::unique_ptr<EditorAction> action)
{
  m_redo_stack.clear();
  m_undo_stack.push_back(std::move(action));
  m_index_pos += 1;

  cleanup();
}

void
UndoManager::cleanup()
{
  const int& max_snapshots = g_config->editor_undo_stack_size;
  while (m_undo_stack.size() > max_snapshots)
  {
    m_undo_stack.erase(m_undo_stack.begin());
  }
}

void
UndoManager::reset_index()
{
  m_index_pos = 0;
  m_undo_stack.clear();
  m_redo_stack.clear();
}

void
UndoManager::undo(int steps)
{
  if (m_undo_stack.empty()) return;
  if (steps > m_undo_stack.size() || steps < 1)
  {
    log_warning << "Cannot perform " << steps << " undo steps." << std::endl;
    return;
  }

  for (int i = 1; i <= steps; i++)
  {
    try
    {
      m_undo_stack.back()->undo(); // Undo the action.
    }
    catch (std::exception& err)
    {
      log_warning << "Undo failed at step " << i << ": " << err.what() << std::endl;
      return;
    }

    m_redo_stack.push_back(std::move(m_undo_stack.back()));
    m_undo_stack.pop_back();

    m_index_pos -= 1;
  }
}

void
UndoManager::redo(int steps)
{
  if (m_redo_stack.empty()) return;
  if (steps > m_redo_stack.size() || steps < 1)
  {
    log_warning << "Cannot perform " << steps << " redo steps." << std::endl;
    return;
  }

  for (int i = 1; i <= steps; i++)
  {
    try
    {
      m_redo_stack.back()->redo(); // Redo the action.
    }
    catch (std::exception& err)
    {
      log_warning << "Redo failed at step " << i << ": " << err.what() << std::endl;
      return;
    }

    m_undo_stack.push_back(std::move(m_redo_stack.back()));
    m_redo_stack.pop_back();

    m_index_pos += 1;
  }
}

bool
UndoManager::has_unsaved_changes() const
{
  return m_index_pos > 0;
}

/* EOF */
