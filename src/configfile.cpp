//  $Id$
//
//  SuperTux -  A Jump'n Run
//  Copyright (C) 2004 Michael George <mike@georgetech.com>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <stdlib.h>
#include <string>
#include "configfile.h"
#include "setup.h"
#include "globals.h"
#include "lispreader.h"
#include "player.h"
#include "physfs_util.h"

#ifdef WIN32
const char * config_filename = "/st_config.dat";
#else
const char * config_filename = "/config";
#endif

static void defaults ()
{
  /* Set defaults: */
  debug_mode = false;
  audio_device = true;

  use_fullscreen = true;
  show_fps = false;
  use_gl = false;

  use_sound = true;
  use_music = true;
}

void loadconfig(void)
{
  defaults();

  /* override defaults from config file */

  PHYSFS_File* file = PHYSFS_openRead(config_filename);
  PHYSFS_FileCharReader file_reader(file);

  if (file == NULL)
    return;

  /* read config file */
  lisp_stream_t   stream;
  lisp_object_t * root_obj = NULL;

  lisp_stream_init_file (&stream, &file_reader);
  root_obj = lisp_read (&stream);

  if (root_obj->type == LISP_TYPE_EOF || root_obj->type == LISP_TYPE_PARSE_ERROR)
    return;

  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-config") != 0)
    return;

  LispReader reader(lisp_cdr(root_obj));

  reader.read_int ("display",    &display_idx);
  reader.read_bool("fullscreen", &use_fullscreen);
  reader.read_bool("sound",      &use_sound);
  reader.read_bool("music",      &use_music);
  reader.read_bool("show_fps",   &show_fps);
  reader.read_bool("player-run-by-default", &run_by_default);
  reader.read_bool("back-scrolling", &back_scrolling);

  std::string video;
  reader.read_string ("video", &video);
  if (video == "opengl")
    use_gl = true;
  else
    use_gl = false;

  reader.read_int ("joystick", &joystick_num);
  if (!(joystick_num >= 0))
    use_joystick = false;
  else
    use_joystick = true;

  reader.read_int ("joystick-x", &joystick_keymap.x_axis);
  reader.read_int ("joystick-y", &joystick_keymap.y_axis);
  reader.read_int ("joystick-a", &joystick_keymap.a_button);
  reader.read_int ("joystick-b", &joystick_keymap.b_button);
  reader.read_int ("joystick-start", &joystick_keymap.start_button);
  reader.read_int ("joystick-deadzone", &joystick_keymap.dead_zone);

  reader.read_int ("keyboard-jump", &keymap.jump);
  reader.read_int ("keyboard-duck", &keymap.duck);
  reader.read_int ("keyboard-left", &keymap.left);
  reader.read_int ("keyboard-right", &keymap.right);
  reader.read_int ("keyboard-fire", &keymap.fire);

  lisp_object_t* addon_cur;
  if (reader.read_lisp("addons", &addon_cur))
  {
    while (!lisp_nil_p(addon_cur))
    {
      lisp_object_t* addon_lisp_el = lisp_car(addon_cur);

      const char* addon_id = lisp_symbol(lisp_car(addon_lisp_el));
      lisp_object_t* addon_val = lisp_car(lisp_cdr(addon_lisp_el));

      if (!lisp_boolean_p(addon_val))
        st_abort("LispReader expected type bool at token: ", addon_id);

      addons_enabled.insert({ addon_id, lisp_boolean(addon_val) });

      addon_cur = lisp_cdr(addon_cur);
    }
  }

  lisp_free(root_obj);
  PHYSFS_close(file);
}

void saveconfig (void)
{
  /* write settings to config file */
  PHYSFS_File* config = PHYSFS_openWrite(config_filename);
  if(config)
    {
      PHYSFS_writeFormatted(config, "(supertux-config\n");
      PHYSFS_writeFormatted(config, "\t;; the following options can be set to #t or #f:\n");
      PHYSFS_writeFormatted(config, "\t(display    %d)\n", display_idx);
      PHYSFS_writeFormatted(config, "\t(fullscreen %s)\n", use_fullscreen ? "#t" : "#f");
      PHYSFS_writeFormatted(config, "\t(sound      %s)\n", use_sound      ? "#t" : "#f");
      PHYSFS_writeFormatted(config, "\t(music      %s)\n", use_music      ? "#t" : "#f");
      PHYSFS_writeFormatted(config, "\t(show_fps   %s)\n", show_fps       ? "#t" : "#f");
      PHYSFS_writeFormatted(config, "\t(player-run-by-default %s)\n", run_by_default ? "#t" : "#f");
      PHYSFS_writeFormatted(config, "\t(back-scrolling %s)\n", back_scrolling ? "#t" : "#f");

      PHYSFS_writeFormatted(config, "\n\t;; either \"opengl\" or \"sdl\"\n");
      PHYSFS_writeFormatted(config, "\t(video      \"%s\")\n", use_gl ? "opengl" : "sdl");

      PHYSFS_writeFormatted(config, "\n\t;; joystick number (-1 means no joystick):\n");
      PHYSFS_writeFormatted(config, "\t(joystick   %d)\n", use_joystick ? joystick_num : -1);

      PHYSFS_writeFormatted(config, "\t(joystick-x   %d)\n", joystick_keymap.x_axis);
      PHYSFS_writeFormatted(config, "\t(joystick-y   %d)\n", joystick_keymap.y_axis);
      PHYSFS_writeFormatted(config, "\t(joystick-a   %d)\n", joystick_keymap.a_button);
      PHYSFS_writeFormatted(config, "\t(joystick-b   %d)\n", joystick_keymap.b_button);
      PHYSFS_writeFormatted(config, "\t(joystick-start  %d)\n", joystick_keymap.start_button);
      PHYSFS_writeFormatted(config, "\t(joystick-deadzone  %d)\n", joystick_keymap.dead_zone);

      PHYSFS_writeFormatted(config, "\t(keyboard-jump  %d)\n", keymap.jump);
      PHYSFS_writeFormatted(config, "\t(keyboard-duck  %d)\n", keymap.duck);
      PHYSFS_writeFormatted(config, "\t(keyboard-left  %d)\n", keymap.left);
      PHYSFS_writeFormatted(config, "\t(keyboard-right %d)\n", keymap.right);
      PHYSFS_writeFormatted(config, "\t(keyboard-fire  %d)\n", keymap.fire);

      PHYSFS_writeFormatted(config, "\t(addons\n");
      for (const auto& addon_info : addons_enabled)
        PHYSFS_writeFormatted(config, "\t\t(%s #%c)\n", addon_info.first.c_str(), addon_info.second ? 't' : 'f');
      PHYSFS_writeFormatted(config, "\t)\n");

      PHYSFS_writeFormatted(config, ")\n");

      PHYSFS_close(config);
    }
}

/* EOF */
