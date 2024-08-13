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

#include "supertux/menu/addon_browse_menu.hpp"

#include <fmt/format.h>

#include "addon/addon.hpp"
#include "addon/addon_index.hpp"
#include "addon/addon_manager.hpp"
#include "gui/dialog.hpp"
#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "supertux/menu/addon_preview_menu.hpp"
#include "supertux/menu/download_dialog.hpp"
#include "util/log.hpp"

static const int MAX_ADDONS_ON_PAGE = 10;

AddonBrowseMenu::AddonBrowseMenu() :
  m_addon_manager(*AddonManager::current()),
  m_addon_index(),
  m_browse_page(1),
  m_index_download_status()
{
  refresh();
}

void
AddonBrowseMenu::refresh()
{
  m_addon_index.reset();

  rebuild_menu();

  m_index_download_status = m_addon_manager.request_index(m_addon_index, MAX_ADDONS_ON_PAGE);
  m_index_download_status->then([this](bool success)
    {
      if (success)
        m_index_download_status.reset();

      rebuild_menu();
    });
}

void
AddonBrowseMenu::rebuild_menu()
{
  clear();

  add_label(_("Browse Add-ons"));
  add_hl();

  if (m_addon_index)
  {
    const auto& addons = m_addon_index->get_addons();

    add_inactive(fmt::format(fmt::runtime(_("Page {}/{}")), m_browse_page, m_addon_index->get_total_pages()));
    add_hl();

    int index = 0;
    for (const auto& addon : addons)
    {
      if (index > MAX_ADDONS_ON_PAGE) break;

      add_entry(MNID_ADDON_LIST_START + index,
          addon_string_util::generate_menu_item_text(*addon, addon->is_installed()));
      index++;
    }
    add_hl();

    if (addons.size() > 0)
    {
      if (m_browse_page <= 1)
        add_inactive(_("Previous page"));
      else
        add_entry(MNID_PREV_PAGE, _("Previous page"));

      if (m_browse_page >= m_addon_index->get_total_pages())
        add_inactive(_("Next page"));
      else
        add_entry(MNID_NEXT_PAGE, _("Next page"));
    }
    else
    {
      add_inactive(_("No Add-ons available!"));
    }
  }
  else if (m_index_download_status)
  {
    add_inactive(_("Error loading Add-ons!"));
    add_inactive(m_index_download_status->error_msg);
  }
  else
  {
    add_inactive(_("Loading Add-ons..."));
  }

  add_hl();
  add_back(_("Back"));
}

void
AddonBrowseMenu::menu_action(MenuItem& item)
{
  const int index = item.get_id();
  if (index == MNID_PREV_PAGE)
  {
    m_browse_page--;

    TransferStatusPtr status = m_addon_manager.request_index_page(m_addon_index, false);
    status->then([this](bool success)
      {
        if (success)
          rebuild_menu();
      });

    m_addon_index.reset();
    rebuild_menu();

    //set_active_item(index);
    //if (get_active_item_id() != index) // Check if the item wasn't set as active, because it's disabled.
      //set_active_item(MNID_NEXT_PAGE);
  }
  else if (index == MNID_NEXT_PAGE)
  {
    m_browse_page++;

    TransferStatusPtr status = m_addon_manager.request_index_page(m_addon_index, true);
    status->then([this](bool success)
      {
        if (success)
          rebuild_menu();
      });

    m_addon_index.reset();
    rebuild_menu();

    //set_active_item(index);
    //if (get_active_item_id() != index) // Check if the item wasn't set as active, because it's disabled.
      //set_active_item(MNID_PREV_PAGE);
  }
  else if (index >= MNID_ADDON_LIST_START)
  {
    const auto& addons = m_addon_index->get_addons();

    const int idx = index - MNID_ADDON_LIST_START;
    if (idx >= 0 && idx < static_cast<int>(addons.size()))
    {
      MenuManager::instance().push_menu(std::make_unique<AddonPreviewMenu>(*addons.at(idx)));
    }
  }
  else
  {
    log_warning << "Unknown menu item clicked: " << index << std::endl;
  }
}

/* EOF */
