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

#include <iostream>
#include <stdlib.h>
#include <string>
#include "addon.hpp"
#include "configfile.h"
#include "globals.h"
#include "reader/reader_document.hpp"
#include "reader/reader_mapping.hpp"
#include "reader/writer.hpp"
#include "player.h"

#ifdef WIN32
const char * config_filename = "st_config.dat";
#else
const char * config_filename = "config";
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

  g_init_enabled_addons.clear();
}

void loadconfig(void)
{
  defaults();

  try
  {
    ReaderDocument doc = ReaderDocument::from_file("config");
    ReaderObject root = doc.get_root();
    if (root.get_name() != "supertux-config")
    {
      throw std::runtime_error("File is not a supertux-config file");
    }
    ReaderMapping reader = root.get_mapping();

    reader.get("fullscreen", use_fullscreen);
    reader.get("sound",      use_sound);
    reader.get("music",      use_music);
    reader.get("show_fps",   show_fps);

    std::string video;
    reader.get ("video", video);
    use_gl = video == "opengl";

    reader.get ("joystick", joystick_num);
    use_joystick = joystick_num >= 0;

    reader.get ("joystick-x", joystick_keymap.x_axis);
    reader.get ("joystick-y", joystick_keymap.y_axis);
    reader.get ("joystick-a", joystick_keymap.a_button);
    reader.get ("joystick-b", joystick_keymap.b_button);
    reader.get ("joystick-start", joystick_keymap.start_button);
    reader.get ("joystick-deadzone", joystick_keymap.dead_zone);

    reader.get ("keyboard-jump", keymap.jump);
    reader.get ("keyboard-duck", keymap.duck);
    reader.get ("keyboard-left", keymap.left);
    reader.get ("keyboard-right", keymap.right);
    reader.get ("keyboard-fire", keymap.fire);

    reader.get("enabled-addons", g_init_enabled_addons);
  }
  catch (std::exception& err)
  {
    log_warning << "Error reading full config: " << err.what() << std::endl;
  }
}

void saveconfig (void)
{
  Writer writer("config");

  writer.start_list("supertux-config");
  writer.write_comment("the following options can be set to #t or #f:");

  writer.write("fullscreen", use_fullscreen);
  writer.write("sound",      use_sound);
  writer.write("music",      use_music);
  writer.write("show_fps",   show_fps);

  writer.write("video", use_gl ? "opengl" : "sdl");

  writer.write("joystick", use_joystick ? joystick_num : -1);

  writer.write("joystick-x", joystick_keymap.x_axis);
  writer.write("joystick-y", joystick_keymap.y_axis);
  writer.write("joystick-a", joystick_keymap.a_button);
  writer.write("joystick-b", joystick_keymap.b_button);
  writer.write("joystick-start", joystick_keymap.start_button);
  writer.write("joystick-deadzone", joystick_keymap.dead_zone);

  writer.write("keyboard-jump", keymap.jump);
  writer.write("keyboard-duck", keymap.duck);
  writer.write("keyboard-left", keymap.left);
  writer.write("keyboard-right", keymap.right);
  writer.write("keyboard-fire", keymap.fire);

  std::vector<std::string> enabled_addons;
  for (const auto& addon : g_addons)
    if (addon->is_enabled())
      enabled_addons.push_back(addon->get_filename());

  writer.write("enabled-addons", enabled_addons);

  writer.end_list("supertux-config");
}

/* EOF */
