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

#include "supertux/menu/debug_menu.hpp"

#include "gui/item_toggle.hpp"
#include "supertux/debug.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "util/gettext.hpp"

AdvancedOptionsMenu::AdvancedOptionsMenu()
{
  add_toggle(-1, _("Show Controller"), &g_config->show_controller)
    .set_help(_("Toggles whether the game should show what keys are being pressed, on a controller"));
  add_toggle(-1, _("Show Framerate"), &g_config->show_fps)
    .set_help(_("Toggles whether the game shows the framerate it's running at"));
  add_toggle(-1, _("Show Player Position"), &g_config->show_player_pos)
    .set_help(_("Toggles whether the game shows the player's exact position (coordinates)"));
  add_toggle(-1, _("Use custom mouse cursor"), &g_config->custom_mouse_cursor)
    .set_help(_("Toggles whether the game renders its own cursor or uses the system's cursor"));
  add_toggle(-1, _("Use Bitmap Fonts"),
             []{ return g_debug.get_use_bitmap_fonts(); },
             [](bool value){ g_debug.set_use_bitmap_fonts(value); })
            .set_help(_("Toggles whether the game uses the old-style Bitmap fonts"));

  add_hl();
  add_back(_("Back"));
}

void
AdvancedOptionsMenu::menu_action(MenuItem& item)
{
    g_config->save();
}

/* EOF */
