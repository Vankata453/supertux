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

#include "supertux/menu/editor_custom_asset_menu.hpp"

#include <physfs.h>

#include "util/file_system.hpp"

namespace custom_asset_util
{
  bool is_categorizable(int asset_type)
  {
    return asset_type < 100; // >= 100 signifies uncategorizable asset types
  }

  std::string create_asset_dir(int asset_type, std::string category_name)
  {
    // TODO
  }
} // namespace custom_asset_util


EditorCustomAssetMenu::EditorCustomAssetMenu() :
  m_import_asset_type(),
  m_import_category(),
  m_import_directory()
{
  initialize();
}

void
EditorCustomAssetMenu::initialize()
{
  add_label(_("Import custom assets"));
  add_hl();

  add_inactive(_("Choose asset category:"));
  add_hl();

  add_inactive(_("Images:"));
  add_entry(Asset::BACKGROUND, _("Backgrounds"));
  add_entry(Asset::DECAL, _("Decals"));
  add_entry(Asset::WORLDMAP, _("Worldmaps"));

  add_inactive(_("Sprites:"));
  add_entry(Asset::CREATURE, _("Creatures"));
  add_entry(Asset::OBJECT, _("Objects"));
  add_entry(Asset::PARTICLE, _("Particles"));
  add_entry(Asset::POWERUP, _("Powerups"));

  add_inactive(_("Tilesets:"));
  add_entry(Asset::TILESET_IMAGE, _("Image Resources"));
  add_entry(Asset::TILESET_AUTOTILE_INFO, _("Autotiling Info (.stac)"));
  add_entry(Asset::TILESET_INFO, _("General Info (.strf)"));

  add_inactive(_("Sounds:"));
  add_entry(Asset::MUSIC, _("Music"));
  add_entry(Asset::SOUND, _("Sounds"));
  add_entry(Asset::SPEECH, _("Speech"));

  add_hl();
  add_back();
}

void
EditorCustomAssetMenu::menu_action(MenuItem& item)
{
  const int id = item.get_id();
  if (id == MNID_SET_CATEGORY) // Category name for import is set
  {
    import_assets(); // Refresh import screen
  }
  else if (id == MNID_OPEN_ASSET_DIR) // An asset directory is being opened
  {
    FileSystem::open_path(FileSystem::join(PHYSFS_getRealDir(m_import_directory.c_str()), m_import_directory));
  }
  else if (id == MNID_IMPORT_OTHER) // Importing more custom assets requested; go back to asset type selection
  {
    m_import_active = false;
    initialize();
  }
  else
  {
    // Switch to asset importing view
    m_import_asset_type = id;
    m_import_category.clear();
    import_assets();
  }
}

void
EditorCustomAssetMenu::import_assets()
{
  clear();

  add_label(_("Import custom assets"));
  add_hl();

  const bool is_categorizable = custom_asset_util::is_categorizable(m_import_asset_type);
  if (is_categorizable)
  {
    add_textfield(_("Category name"), &m_import_category);
    add_entry(MNID_SET_CATEGORY, _("Set"));
  }

  add_hl();
  if (is_categorizable && m_import_category.empty())
  {
    add_inactive(_("Specify a category name for your assets."));

    add_hl();
  }
  else
  {
    m_import_directory = custom_asset_util::create_asset_dir(m_import_asset_type, m_import_category);

    add_inactive(_("Insert your desired assets into the given directory."));
    add_entry(MNID_OPEN_ASSET_DIR, _("Open Directory"));

    add_hl();
    add_entry(MNID_IMPORT_OTHER, _("Import other custom assets"));
  }

  add_back();
}

/* EOF */
