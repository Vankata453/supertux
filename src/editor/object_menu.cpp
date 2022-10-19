//  SuperTux
//  Copyright (C) 2015 Hume2 <teratux.mail@gmail.com>
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

#include "editor/object_menu.hpp"

#include "editor/editor.hpp"
#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "supertux/d_scope.hpp"
#include "supertux/sector.hpp"
#include "supertux/moving_object.hpp"

ObjectMenu::ObjectMenu(Editor& editor, GameObject* go) :
  m_editor(editor),
  m_object(go),
  m_object_settings(m_object->get_settings()),
  m_initial_values(m_object_settings.get_option_values())
{
  add_label(m_object_settings.get_name());
  add_hl();
  for (const auto& oo_ptr : m_object_settings.get_options())
  {
    const auto& oo = *oo_ptr;

    if (!(oo.get_flags() & OPTION_HIDDEN)) {
      oo.add_to_menu(*this);
    }
  }
  add_hl();
  add_back(_("OK"), -1);
}

ObjectMenu::~ObjectMenu()
{
}

void
ObjectMenu::menu_action(MenuItem& item)
{
  switch (item.get_id())
  {
    case MNID_REMOVE:
      m_editor.delete_markers();
      m_editor.m_reactivate_request = true;
      MenuManager::instance().pop_menu();
      m_editor.save_action(std::make_unique<ObjectDeleteAction>(m_editor.get_sector()->get_name(), *m_object, true));
      break;

    case MNID_TEST_FROM_HERE: {
      const MovingObject *here = dynamic_cast<const MovingObject *>(m_object);
      m_editor.m_test_pos = std::make_pair(m_editor.get_sector()->get_name(),
                                           here->get_pos());
      m_editor.m_test_request = true;
      MenuManager::instance().pop_menu();
      break;
    }

    case MNID_OPEN_PARTICLE_EDITOR:
      m_editor.m_particle_editor_request = true;
      MenuManager::instance().pop_menu();
      break;

    default:
      break;
  }
}

bool
ObjectMenu::on_back_action()
{
  // FIXME: this is a bit fishy, menus shouldn't mess with editor internals
  BIND_SECTOR(*m_editor.get_sector());

  // Get a map of modified options, containing their keys and old values.
  std::map<std::string, std::string> modified_values;
  for (auto& value : m_initial_values)
  {
    if (m_object_settings.get_option_by_key(value.first)->to_string() != value.second)
    {
      // Option has changed.
      modified_values.insert(value);
    }
  }
  if (modified_values.size() > 0)
  {
    m_editor.save_action(std::make_unique<ObjectOptionChangeAction>(m_editor.get_sector()->get_name(),
        m_object->get_uid(), modified_values));
  }

  m_object->after_editor_set();

  m_editor.m_reactivate_request = true;
  if (!dynamic_cast<MovingObject*>(m_object)) {
    m_editor.sort_layers();
  }

  return true;
}

/* EOF */
