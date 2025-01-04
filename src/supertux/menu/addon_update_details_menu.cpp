//  SuperTux
//  Copyright (C) 2025 Vankata453
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

#include "supertux/menu/addon_update_details_menu.hpp"

#include <fmt/format.h>

#include "addon/addon.hpp"
#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "supertux/menu/addon_preview_menu.hpp"
#include "supertux/resources.hpp"
#include "util/log.hpp"
#include "util/string_util.hpp"

AddonUpdateDetailsMenu::AddonUpdateDetailsMenu(const Addon& addon) :
  m_addon(addon)
{
  assert(addon.has_available_update());

  const Addon::Version& local_version = m_addon.get_version();
  const Addon::Version& upstream_version = m_addon.get_upstream_addon()->get_version();

  add_label(fmt::format(fmt::runtime(_("Update Details for \"{}\"")), m_addon.get_title()));
  add_hl();

  if (local_version.commit.length() >= 6 && upstream_version.commit.length() >= 6)
  {
    add_inactive(local_version.commit.substr(0, 6) + " -> " + upstream_version.commit.substr(0, 6));
    add_inactive("");
  }

  add_inactive(upstream_version.title, true);
  if (upstream_version.description.empty())
  {
    add_inactive(_("No description available."));
  }
  else
  {
    // TODO: Support multiline format
    std::string overflow;
    add_inactive(Resources::normal_font->wrap_to_width(upstream_version.description, 600.f, &overflow), true);
    while (!overflow.empty())
    {
      add_inactive(Resources::normal_font->wrap_to_width(overflow, 600.f, &overflow), true);
    }
  }
  add_inactive("");

  add_inactive(StringUtil::timestamp_to_date_string(upstream_version.created_at, "%B %d, %Y; %T"), true);
  add_hl();

  add_entry(MNID_UPDATE, _("Update"));
  add_back(_("Back"));
}

void
AddonUpdateDetailsMenu::menu_action(MenuItem& item)
{
  if (item.get_id() != MNID_UPDATE)
  {
    log_warning << "Unknown menu item clicked: " << item.get_id() << std::endl;
    return;
  }

  AddonPreviewMenu::install_addon(m_addon);
  MenuManager::instance().pop_menu(true);
}

/* EOF */
