//  $Id$
// 
//  SuperTux
//  Copyright (C) 2004 Bill Kendrick <bill@newbreedsoftware.com>
//                     Tobias Glaesser <tobi.web@gmx.de>
//                     Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef SUPERTUX_GLOBALS_H
#define SUPERTUX_GLOBALS_H

#include <string>
#include <map>
#include <SDL2/SDL.h>
#include "text.h"
#include "menu.h"
#include "defines.h"
#include "mousecursor.h"

class Downloader;

extern std::string real_datadir;
extern std::string real_userdir;

struct JoystickKeymap
{
  int a_button;
  int b_button;
  int start_button;

  int x_axis;
  int y_axis;
  
  int dead_zone;

  JoystickKeymap();
};

extern JoystickKeymap joystick_keymap;

struct Addon final
{
  Addon(const std::string& filename_) :
    filename(filename_),
    mounted(false)
  {}
  const std::string filename;
  bool mounted;

  std::string title;
  std::string author;

  bool overrides_data = false;
  bool resource_pack = false;

  std::vector<std::string> dependencies;
};
extern std::map<std::string, Addon> addons;
extern std::map<std::string, bool> addons_enabled;

struct IndexAddon final
{
  std::string title;
  std::string author;

  bool resource_pack = false;

  std::string file;
  std::string custom_url;

  std::vector<std::string> dependencies;
};
extern bool addon_index_fetched;
extern std::map<std::string, IndexAddon> addon_index;
extern std::string addon_index_base_url;

extern SDL_Surface* screen;
extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern SDL_Texture* sdl_texture;
#ifndef NOOPENGL
extern SDL_GLContext glcontext;
extern int glviewport_x, glviewport_y, glviewport_w, glviewport_h;
#endif

extern Text* black_text;
extern Text* gold_text;
extern Text* silver_text;
extern Text* white_text;
extern Text* white_small_text;
extern Text* white_big_text;
extern Text* blue_text;
extern Text* red_text;
extern Text* green_text;
extern Text* yellow_nums;

extern MouseCursor* mouse_cursor;

extern Downloader* downloader;

extern int  display_idx;
extern bool use_gl;
extern bool use_joystick;
extern bool use_fullscreen;
extern bool debug_mode;
extern bool show_fps;
extern bool back_scrolling;

/** The number of the joystick that will be use in the game */
extern int joystick_num;
extern char* level_startup_file;
extern bool launch_leveleditor_mode;

extern float game_speed;
extern SDL_Joystick * js;

int wait_for_event(SDL_Event& event,unsigned int min_delay = 0, unsigned int max_delay = 0, bool empty_events = false);

inline int screen_w()
{
  return SCREEN_W;
}
inline int screen_h()
{
  return SCREEN_H;
}

#endif /* SUPERTUX_GLOBALS_H */
