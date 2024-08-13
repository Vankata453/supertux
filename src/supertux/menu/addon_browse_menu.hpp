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

#ifndef HEADER_SUPERTUX_SUPERTUX_MENU_ADDON_BROWSE_MENU_HPP
#define HEADER_SUPERTUX_SUPERTUX_MENU_ADDON_BROWSE_MENU_HPP

#include "gui/menu.hpp"

#include "addon/addon_index.hpp"
#include "addon/downloader_defines.hpp"

class AddonManager;

class AddonBrowseMenu final : public Menu
{
public:
  AddonBrowseMenu();

  void refresh() override;

  void menu_action(MenuItem& item) override;

private:
  void rebuild_menu();

private:
  enum {
    MNID_PREV_PAGE = 1,
    MNID_NEXT_PAGE,
    MNID_ADDON_LIST_START
  };

private:
  AddonManager& m_addon_manager;
  std::unique_ptr<AddonIndex> m_addon_index;

  int m_browse_page;
  TransferStatusPtr m_index_download_status;

private:
  AddonBrowseMenu(const AddonBrowseMenu&) = delete;
  AddonBrowseMenu& operator=(const AddonBrowseMenu&) = delete;
};

#endif

/* EOF */
