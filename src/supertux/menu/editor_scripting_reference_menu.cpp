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

#include "supertux/menu/editor_scripting_reference_menu.hpp"

#include <physfs.h>

#include "editor/editor.hpp"
#include "gui/dialog.hpp"
#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "supertux/level.hpp"
#include "supertux/menu/menu_storage.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "supertux/menu/editor_save_as.hpp"
#include "supertux/menu/menu_storage.hpp"
#include "util/gettext.hpp"
#include "video/compositor.hpp"

EditorScriptingReferenceMenu::EditorScriptingReferenceMenu()
{
    add_label(_("Scripting Reference"));
    add_hl();
    add_entry(MNID_AMBIENT_SOUND, _("AmbientSound"));
    add_entry(MNID_CAMERA, _("Camera"));
    add_entry(MNID_CANDLE, _("Candle"));
    add_entry(MNID_DISPLAY_EFFECT, _("DisplayEffect"));
    add_entry(MNID_FLOATING_IMAGE, _("FloatingImage"));
    add_entry(MNID_GLOBALS, _("Globals"));
    add_entry(MNID_LEVEL, _("Level"));
    add_entry(MNID_LEVEL_TIME, _("LevelTime"));
    add_entry(MNID_PATH, _("Path"));
    add_entry(MNID_PLATFORM, _("Platform"));
    add_entry(MNID_PLAYER, _("Player"));
    add_entry(MNID_SCRIPTED_OBJECT, _("ScriptedObject"));
    add_entry(MNID_SECTOR, _("Sector"));
    add_entry(MNID_SOUND, _("Sound"));
    add_entry(MNID_TEXT, _("Text"));
    add_entry(MNID_THUNDERSTORM, _("Thunderstorm"));
    add_entry(MNID_TILEMAP, _("Tilemap"));
    add_entry(MNID_WILL_O_WISP, _("Will-o-wisp"));
    add_entry(MNID_WIND, _("Wind"));
}

void
EditorScriptingReferenceMenu::menu_action(MenuItem& item)
{
  auto dialog = std::make_unique<Dialog>();
  switch (item.get_id())
  {
    case MNID_AMBIENT_SOUND:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "`set_pos(float x, float y)` -> Sets the position of the ambient sound.\n"
        "`float get_pos_x()` -> Returns the x coordinate.\n"
        "`float get_pos_y()` -> Returns the y coordinate."));
      break;

    case MNID_CAMERA:
      dialog->set_text(_("The camera can be accessed with the 'Camera' instance.\n"
        "`shake(float time, float x, float y)` -> Moves camera to the given coordinates in time seconds returning quickly to the original position after that.\n"
        "`set_pos(float x, float y)` -> Moves the camera to the specified absolute position. The origin is at the top left.\n"
        "`set_mode(string modestring)` -> This function sets the camera mode. Valid values for modestring are “normal” and “manual”.\n"
        "`scroll_to(float x, float y, float time)` -> Scrolls the camera to the given coordinates within time seconds."));
      break;

    case MNID_CANDLE:
      dialog->set_text(_("A candle can be accessed by its name from a script.\n"
        "`bool get_burning()` -> Returns true if candle is lighted.\n"
        "`set_burning(bool burning)` -> Lights candle if true; extinguishes candle if false."));
      break;

    case MNID_DISPLAY_EFFECT:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "`fade_out(float fadetime)` -> Gradually fades out the screen to black for the next fadetime seconds.\n"
        "`fade_in(float fadetime)` -> Gradually fades in the screen from black for the next fadetime seconds.\n"
        "`set_black(bool black)` -> Blackens or un-blackens the screen (depending on the value of black).\n"
        "`is_black()` -> Returns: bool; has the screen been blackened by set_black\nNote: Calling fade_in or fade_out resets the return value to false.\n"
        "`sixteen_to_nine(float fadetime)` -> Sets the display ratio to 16:9, effectively adding black bars at the top and bottom of the screen.\nShould be used before cutscenes. Gradually fades to this state for the next fadetime seconds.\n"
        "`four_to_three(float fadetime)` -> Sets the display ratio to 4:3, removing the black bars added by sixteen_to_nine().\nShould be used after cutscenes. Gradually fades to this state for the next fadetime seconds."));
      break;
      
    case MNID_FLOATING_IMAGE:
      dialog->set_text(_("Constructor for floating images in a script: `FloatingImage(string filename)`\n"
        "`set_layer(int layer)` -> Moves this image to the layer layer aka z-position.\n"
        "`int get_layer()` -> Returns: int; the layer the floating image is on.\n"
        "`set_pos(float x, float y)` -> Sets the position of this image in relation to the current anchor point.\n"
        "`int get_x()` -> Returns the x position of this image relative to the current anchor point.\n"
        "`int get_y()` -> Returns the y position of this image relative to the current anchor point.\n"
        "`int get_anchor_point()` -> Returns the current anchor point of this image.\n"
        "`set_anchor_point(int anchor)` -> Set the image's anchor point. Possible values are represented by the ANCHOR_* constants.\n"
        "`string get_action()` -> Returns the name of the current action of this image. (Only useful for sprites)\n"
        "`set_action(string action)` -> Sets the current action of this image. (Only useful for sprites)\n"
        "`bool get_visible()` -> Returns the current visibility of this image.\n"
        "`set_visible(bool visible)` -> Shows or hides the image abruptly according to visible (drastic counterpart to fade_in and fade_out).\n"
        "`fade_in(float time)` -> Fades in this image for the next time seconds.\n"
        "`fade_out(float time)` -> Just the opposite of fade_in."));
      break;
      
    case MNID_GLOBALS:
      dialog->set_text(_("This module contains global constants and methods.\n"
        "`display(*** object)` -> Displays the string value of object in the Console. Object can be of any data type.\n"
        "`print_stacktrace()` -> Displays contents of the current stack.\n"
        "`bool is_christmas()` -> Returns whether Christmas mode is enabled.\n"
        "`load_worldmap(string filename)` -> Loads and runs the worldmap specified in filename. (The path is relative to the data root.)\n"
        "`set_next_worldmap(string dirname, string spawnpoint)` -> Switches to a different worldmap after unloading current one, after exit_screen() is called.\n"
        "`load_level(string filename)` -> Loads and runs the level specified in filename. (The path is relative to the data root.)\n"
        "`get_current_thread()` -> Returns the currently running thread.\n"
        "`display_text_file(string filename)` -> Displays the SuperTux text file named filename. (The path is relative to the data root, e.g. “/usr/share/games/supertux2/”)\n"
        "`wait(float time)` -> Pauses execution of the Squirrel code for time seconds.\n"
        "`wait_for_screenswitch()` -> Pauses execution of the Squirrel code until a new screen is displayed (e.g. menu → worldmap or worldmap → level).\n"
        "`exit_screen()` -> Exits the current screen, returning to the previous one or, if the active screen is the last one, exiting SuperTux.\n"
        "`string translate(string text)` -> Translates text into the user's locale.\nNote: This construct is unfortunately not yet recognized by XGetText, so translation files have to be written manually.\n"
        "`string translate_plural(string text, string text_plural, int num)` -> Returns text or text_plural depending on num and the locale.\n"
        "`import(string filename)` -> Imports and runs the Squirrel script filename. (The path is relative to the data root.)\n"
        "`save_state()` -> Dumps the current state into the user's save game file.\n"
        "`load_state()` -> Loads world state from scripting table.\n"
        "`play_music(string musicfile)` -> Plays music, e.g. “antarctica/chipdisko.music”.\n"
        "`stop_music(float fadetime)` -> Fades out music in fadetime seconds.\n"
        "`play_sound(string soundfile)` -> Plays sound, e.g “sounds/lifeup.wav”.\n"
        "`debug_collrects(bool enable)` -> Enables or disables drawing of collision rectangles.\n"
        "`debug_show_fps(bool enable)` -> Enables or disables drawing of the FPS. (Also affects config file)\n"
        "`debug_draw_solids_only(bool enable)` -> When enabled, only draws solid tilemaps. (No background/foreground tiles)\n"
        "`debug_draw_editor_images(bool enable)` -> Enables or disables drawing of editor images.\n"
        "`debug_worldmap_ghost(bool enable)` -> Enables or disables worldmap ghost mode.\n"
        "`set_game_speed(float speed)` -> Sets speed to run the game at. (Doesn't affect menus/gui)\n"
        "`grease()` -> Speeds Tux's horizontal velocity by a factor of 3.\n"
        "`ghost()` -> Makes Tux a ghost, letting him float around and through objects.\n"
        "`invincible()` -> Makes Tux invincible for 10000 units of game time.\n"
        "`mortal()` -> Recalls Tux's invincibility or ghost status. (Even when not given with above 2 commands)\n"
        "`restart()` -> Reinitializes and respawn Tux at the beginning of the current level.\n"
        "`whereami()` -> Prints out Tux's coordinates to the console.\n"
        "`gotoend()` -> Moves Tux horizontally 2 screens away from the end.\n"
        "`warp(float x, float y)` -> Moves Tux x tiles to the right and y tiles to the bottom.\n"
        "`camera()` -> Displays the current camera's coordinates. (top-left corner)\n"
        "`set_gamma(float gamma)` -> Sets gamma (brightness).\n"
        "`quit()` -> Exits the game. (Not recommended for use in levels!)\n"
        "`int rand()` -> Returns a random evenly-distributed integer between 0 and 2147483647, inclusive.\n"
        "`record_demo(string filename)` -> Records a demo to the given file.\n"
        "`play_demo(string filename)` -> Plays back a demo from the given file."));
      break;
      
    case MNID_LEVEL:
      dialog->set_text(_("The level can be accessed with the 'Level' instance.\n"
        "`finish(bool win)` -> Ends the current level. If you set win to true, the level is marked as completed if launched from a worldmap.\n"
        "`spawn(string sector string spawnpoint)` -> Respawns Tux in sector sector at spawnpoint spawnpoint.\n"
        "`flip_vertically()` -> Flips the level vertically. Call again to revert the effect.\n"
        "`toggle_pause()` -> Toggle pause.\n"
        "`edit(bool editing)` -> Change to/from edit mode."));
      break;
      
    case MNID_LEVEL_TIME:
      dialog->set_text(_("A level time object can be accessed by its name from a script.\n"
        "`start()` -> Resumes the countdown (assuming it isn't already started, in which case it does nothing).\n"
        "`stop()` -> Pauses the countdown (assuming it isn't already stopped, in which case it does nothing).\n"
        "`float get_time()` -> Returns the number of seconds left on the clock.\n"
        "`set_time(float time_left)` -> Changes the number of seconds left on the clock."));
      break;
      
    case MNID_PATH:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "`set_pos(float x, float y)` -> Sets the position of the ambient sound.\n"
        "`float get_pos_x()` -> Returns the x coordinate.\n"
        "`float get_pos_y()` -> Returns the y coordinate."));
      break;
      
    case MNID_PLATFORM:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;
      
    case MNID_PLAYER:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;
      
    case MNID_SCRIPTED_OBJECT:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;
      
    case MNID_SECTOR:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;
      
    case MNID_SOUND:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;
      
    case MNID_TEXT:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;
      
    case MNID_THUNDERSTORM:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;
      
    case MNID_TILEMAP:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;
      
    case MNID_WILL_O_WISP:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;
      
    case MNID_WIND:
      dialog->set_text(_("An ambient sound can be accessed by its name from a script.\n"
        "set_pos(float x, float y) -> Sets the position of the ambient sound.\n"
        "float get_pos_x() -> Returns the x coordinate.\n"
        "float get_pos_y() -> Returns the y coordinate."));
      break;

    default:
      break;
  }
  dialog->add_cancel_button(_("Got it!"));
  MenuManager::instance().set_dialog(std::move(dialog));
}

/* EOF */