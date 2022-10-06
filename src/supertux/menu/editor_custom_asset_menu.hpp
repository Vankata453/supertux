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

#ifndef HEADER_SUPERTUX_SUPERTUX_MENU_EDITOR_CUSTOM_ASSET_MENU_HPP
#define HEADER_SUPERTUX_SUPERTUX_MENU_EDITOR_CUSTOM_ASSET_MENU_HPP

#include "gui/menu.hpp"

enum Asset
{
  BACKGROUND,
  DECAL,
  WORLDMAP,
  CREATURE,
  OBJECT,
  PARTICLE,
  POWERUP,
  MUSIC,
  SOUND,
  SPEECH,
  TILESET_IMAGE = 100, // >= 100 signifies uncategorizable asset types
  TILESET_AUTOTILE_INFO,
  TILESET_INFO
}

namespace custom_asset_util
{
  bool is_categorizable(int asset_type);
  std::string create_asset_dir(int asset_type, std::string category_name);
}

class EditorCustomAssetMenu final : public Menu
{
private:
  enum
  {
    MNID_SET_CATEGORY,
    MNID_OPEN_ASSET_DIR,
    MNID_IMPORT_OTHER
  }
  bool m_import_active;
  int m_import_asset_type;
  std::string m_import_category;
  std::string m_import_directory;

public:
  EditorCustomAssetMenu();

  void menu_action(MenuItem& item) override;

  void import_assets(int asset_type);

private:
  EditorCustomAssetMenu(const EditorCustomAssetMenu&) = delete;
  EditorCustomAssetMenu& operator=(const EditorCustomAssetMenu&) = delete;
};

#endif

/* EOF */
