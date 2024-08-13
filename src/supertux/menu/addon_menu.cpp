//  SuperTux
//  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
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

#include "supertux/menu/addon_menu.hpp"

#include <fmt/format.h>

#include "addon/addon.hpp"
#include "addon/addon_manager.hpp"
#include "gui/dialog.hpp"
#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "supertux/menu/addon_browse_menu.hpp"
#include "supertux/menu/addon_file_install_menu.hpp"
#include "supertux/menu/addon_preview_menu.hpp"
#include "supertux/menu/download_dialog.hpp"
#include "util/log.hpp"

AddonMenu::AddonMenu() :
  m_addon_manager(*AddonManager::current()),
  m_installed_addons(),
  m_addons_enabled()
{
  refresh();
}

void
AddonMenu::refresh()
{
  m_installed_addons = m_addon_manager.get_installed_addons();
  m_addons_enabled.reset(new bool[m_installed_addons.size()]);

  rebuild_menu();
}

void
AddonMenu::rebuild_menu()
{
  clear();

  add_label(_("Installed Add-ons"));
  add_hl();

  std::vector<int> addon_updates_to_list;
  std::vector<int> addons_to_list;
  if (m_installed_addons.empty())
  {
    add_inactive(_("No Add-ons installed!"));
  }
  else
  {
    int idx = 0;
    for (const auto& addon_id : m_installed_addons)
    {
      const Addon& addon = m_addon_manager.get_installed_addon(addon_id);
      m_addons_enabled[idx] = addon.is_enabled();

      if (addon.has_available_update())
      {
        const Addon* upstream_addon = addon.get_upstream_addon();
        log_debug << addon.get_id() << " is installed, but updated: '"
                  << addon.get_version().commit << "' vs '" << upstream_addon->get_version().commit << "'  '"
                  << addon.get_version().created_at << "' vs '" << upstream_addon->get_version().created_at << "'"
                  << std::endl;

        addon_updates_to_list.push_back(idx);
      }
      else
      {
        addons_to_list.push_back(idx);
      }

      idx++;
    }
  }

  for (const auto& index : addon_updates_to_list)
  {
    const Addon& addon = m_addon_manager.get_installed_addon(m_installed_addons[index]);
    const std::string text = addon_string_util::generate_menu_item_text(addon, false);

    if (addon.is_enabled())
      add_entry(MNID_ADDON_LIST_START + index, fmt::format(fmt::runtime(_("{} *UPDATE*")), text));
    else
      add_entry(MNID_ADDON_LIST_START + index, fmt::format(fmt::runtime(_("{} [DISABLED] *UPDATE*")), text));
  }
  for (const auto& index : addons_to_list)
  {
    const Addon& addon = m_addon_manager.get_installed_addon(m_installed_addons[index]);
    const std::string text = addon_string_util::generate_menu_item_text(addon, false);

    if (addon.is_enabled())
      add_entry(MNID_ADDON_LIST_START + index, text);
    else
      add_entry(MNID_ADDON_LIST_START + index, fmt::format(fmt::runtime(_("{} [DISABLED]")), text));
  }

  add_hl();

  if (m_installed_addons.size() > 0)
  {
    const size_t addon_updates_count = addon_updates_to_list.size();
    if (addon_updates_count == 0)
    {
      add_inactive(_("No updates available."));
    }
    else
    {
      add_inactive(fmt::format(fmt::runtime(_("{} {} available")), addon_updates_count, addon_updates_count == 1 ? _("update") : _("updates")));
    }
    add_entry(MNID_UPDATE_CHECK, _("Check for updates"));
    add_hl();
  }

  add_entry(MNID_BROWSE, _("Browse Add-ons"));
  add_entry(MNID_INSTALL_FROM_FILE, _("Install from file"));

  add_hl();
  add_back(_("Back"));
}

void
AddonMenu::menu_action(MenuItem& item)
{
  int index = item.get_id();
  if (index == MNID_UPDATE_CHECK)
  {
    check_for_updates();
  }
  else if (index == MNID_BROWSE)
  {
    MenuManager::instance().push_menu(std::make_unique<AddonBrowseMenu>());
  }
  else if (index == MNID_INSTALL_FROM_FILE)
  { 
    MenuManager::instance().push_menu(std::make_unique<AddonFileInstallMenu>(this));
  }
  else if (index >= MNID_ADDON_LIST_START)
  {
    const int idx = index - MNID_ADDON_LIST_START;
    if (idx >= 0 && idx < static_cast<int>(m_installed_addons.size()))
    {
      const Addon& addon = m_addon_manager.get_installed_addon(m_installed_addons[idx]);
      MenuManager::instance().push_menu(std::make_unique<AddonPreviewMenu>(addon));
    }
  }
  else
  {
    log_warning << "Unknown menu item clicked: " << index << std::endl;
  }
}

void
AddonMenu::check_for_updates()
{
  try
  {
    TransferStatusListPtr status = m_addon_manager.request_upstream_addons();
    status->then([this](bool success)
    {
      if (success)
        refresh();

      set_active_item(MNID_UPDATE_CHECK);
    });
    auto dialog = std::make_unique<DownloadDialog>(status, false);
    dialog->set_title(_("Checking for updates..."));
    MenuManager::instance().set_dialog(std::move(dialog));
  }
  catch (const std::exception& err)
  {
    log_warning << "Fetching upstream add-ons failed: " << err.what() << std::endl;
  }
}

/* EOF */
