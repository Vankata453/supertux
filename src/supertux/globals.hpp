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

#ifndef HEADER_SUPERTUX_SUPERTUX_GLOBALS_HPP
#define HEADER_SUPERTUX_SUPERTUX_GLOBALS_HPP

#include <memory>

class Config;

/** The width of the display (this is a logical value, not the
    physical value, since aspect_ration and projection_area might
    shrink or scale things) */
extern int SCREEN_WIDTH;

/** The width of the display (this is a logical value, not the
    physical value, since aspect_ration and projection_area might
    shrink or scale things) */
extern int SCREEN_HEIGHT;

extern std::unique_ptr<Config> g_config;

extern float game_time;
extern float real_time;

/** Real time (real_time) counter for an RTA speedrun.
    Started when the first level of a worldmap is entered and the black screen has been exited.
    Captured/uncaptured manually by the user, using the respective control. */
extern float g_run_timer;
extern float g_run_timer_captured_time;

extern float g_game_speed;

#endif

/* EOF */
