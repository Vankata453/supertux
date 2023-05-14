//  $Id$
// 
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <SDL.h>
#include <SDL_image.h>

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include "addon.hpp"
#include "defines.h"
#include "globals.h"
#include "title.h"
#include "screen.h"
#include "high_scores.h"
#include "menu.h"
#include "texture.h"
#include "timer.h"
#include "setup.h"
#include "level.h"
#include "gameloop.h"
#include "leveleditor.h"
#include "scene.h"
#include "player.h"
#include "math.h"
#include "tile.h"
#include "resources.h"
#include "worldmap.h"
#include "file_system.hpp"

static Surface* bkg_title;
static Surface* logo;
static Surface* img_choose_subset;

static bool walking;
static Timer random_timer;

static int frame;
static unsigned int last_update_time;
static unsigned int update_time;

static std::vector<LevelSubset*> contrib_subsets;
static std::string current_contrib_subset;

static std::vector<std::string> worldmaps;

void free_contrib_menu()
{
  for(std::vector<LevelSubset*>::iterator i = contrib_subsets.begin();
      i != contrib_subsets.end(); ++i)
    delete *i;

  contrib_subsets.clear();
  contrib_menu->clear();
}

void generate_contrib_menu()
{
  const std::vector<std::string> level_subsets = FileSystem::get_files("levels");

  free_contrib_menu();

  contrib_menu->additem(MN_LABEL,"Bonus Levels",0,0);
  contrib_menu->additem(MN_HL,"",0,0);

  int levelsets_added = 0;
  for (size_t i = 0; i < level_subsets.size(); i++)
  {
    if (!FileSystem::file_exists("levels/" + level_subsets.at(i) + "/info")) // No info file included in subset
      continue; // Nothing to display

    LevelSubset* subset = new LevelSubset();
    subset->load(level_subsets.at(i));
    contrib_menu->additem(MN_GOTO, subset->title.c_str(), levelsets_added,
        contrib_subset_menu, levelsets_added);
    contrib_subsets.push_back(subset);
    levelsets_added++;
  }

  for(size_t i = 0; i < worldmaps.size(); i++)
  {
    WorldMapNS::WorldMap worldmap;
    worldmap.loadmap(worldmaps.at(i));
    contrib_menu->additem(MN_ACTION, worldmap.get_world_title(),0,0, i + levelsets_added);
  }

  contrib_menu->additem(MN_HL,"",0,0);
  contrib_menu->additem(MN_BACK,"Back",0,0);
}

void check_contrib_menu()
{
  int index = contrib_menu->check();
  if (index == -1)
    return;

  if (index < static_cast<int>(contrib_subsets.size()))
    {
      // FIXME: This shouln't be busy looping
      LevelSubset& subset = * (contrib_subsets[index]);

      current_contrib_subset = subset.name;
 
      contrib_subset_menu->clear();
 
      contrib_subset_menu->additem(MN_LABEL, subset.title, 0,0);
      contrib_subset_menu->additem(MN_HL,"",0,0);

      for (int i = 0; i < subset.levels; ++i)
        {
        /** get level's title */
        Level level;
        level.load(subset.name, i+1);
        contrib_subset_menu->additem(MN_ACTION, level.name, 0,0,i+1);
        }

      contrib_subset_menu->additem(MN_HL,"",0,0);      
      contrib_subset_menu->additem(MN_BACK, "Back", 0, 0);
      }
    else if(index < static_cast<int>(worldmaps.size() + contrib_subsets.size()))
      {
      // Loading fade
      fadeout();

      WorldMapNS::WorldMap worldmap;
      worldmap.loadmap(worldmaps[index - contrib_subsets.size()]);
//      worldmap.set_levels_as_solved();
      std::string savegame = worldmaps[index - contrib_subsets.size()];
      // remove .stwm...
      savegame = savegame.substr(0, savegame.size()-5);
      savegame = std::string(st_save_dir) + "/" + savegame + ".stsg";
      std::cout << "SaveGameName: " << savegame << "\n";
      worldmap.loadgame(savegame.c_str());

      worldmap.display();

      Menu::set_current(main_menu);
      }
}

void generate_addons_menu()
{
  addons_menu->clear();

  addons_menu->additem(MN_LABEL,"Add-ons",0,0);
  addons_menu->additem(MN_HL,"",0,0);

  int i = 0;
  for (const auto& addon : g_addons)
  {
    addons_menu->additem(MN_TOGGLE, addon->get_filename().c_str(), addon->is_enabled(),
        nullptr, i); // Add addon to menu
    i++;
  }

  addons_menu->additem(MN_HL,"",0,0);
  addons_menu->additem(MN_BACK,"Back",0,0);
}

void check_addons_menu()
{
  int index = addons_menu->check();
  if (index != -1 &&
      addons_menu->get_item_by_id(index).kind == MN_TOGGLE)
  {
    g_addons[index]->toggle(); // Toggle addon
  }
}

void check_contrib_subset_menu()
{
  int index = contrib_subset_menu->check();
  if (index != -1)
    {
      if (contrib_subset_menu->get_item_by_id(index).kind == MN_ACTION)
        {
          std::cout << "Starting level: " << index << std::endl;
          GameSession session(current_contrib_subset, index, ST_GL_PLAY);
          session.run();
          player_status.reset();
          Menu::set_current(main_menu);
        }
    }
}

void draw_background()
{
  /* Draw the title background: */

  bkg_title->draw_bg();
}

void draw_demo(GameSession* session, double frame_ratio)
{
  World* world  = session->get_world();
  World::set_current(world);
  Level* plevel = session->get_level();
  Player* tux = world->get_tux();

  world->play_music(LEVEL_MUSIC);
  
  global_frame_counter++;
  tux->key_event((SDLKey) keymap.right,DOWN);
  
  if(random_timer.check())
    {
      if(walking)
        tux->key_event((SDLKey) keymap.jump,UP);
      else
        tux->key_event((SDLKey) keymap.jump,DOWN);
    }
  else
    {
      random_timer.start(rand() % 3000 + 3000);
      walking = !walking;
    }

  // Wrap around at the end of the level back to the beginnig
  if(plevel->width * 32 - 320 < tux->base.x)
    {
      tux->level_begin();
      scroll_x = 0;
    }

  tux->can_jump = true;
  float last_tux_x_pos = tux->base.x;
  world->action(frame_ratio);
  

  // disabled for now, since with the new jump code we easily get deadlocks
  // Jump if tux stays in the same position for one loop, ie. if he is
  // stuck behind a wall
  if (last_tux_x_pos == tux->base.x)
    {
      walking = false;
    }

  world->draw();
}

/* --- TITLE SCREEN --- */
void title(void)
{
  random_timer.init(true);

  walking = true;

  st_pause_ticks_init();

  GameSession session("levels/misc/menu.stl", 0, ST_GL_DEMO_GAME);

  clearscreen(0, 0, 0);
  updatescreen();

  /* Load images: */
  bkg_title = new Surface("images/title/background.jpg", IGNORE_ALPHA);
  logo = new Surface("images/title/logo.png", USE_ALPHA);
  img_choose_subset = new Surface("images/status/choose-level-subset.png", USE_ALPHA);

  /* Generating contrib maps by only using a string_list */
  // Since there isn't any world dir or anything, add a hardcoded entry for Bonus Island
  for (const std::string& map_file : FileSystem::get_files("levels/worldmaps"))
  {
    if (map_file == "world1.stwm" || FileSystem::extension(map_file) != ".stwm")
      continue;

    worldmaps.push_back(map_file);
  }

  /* --- Main title loop: --- */
  frame = 0;

  /* Draw the title background: */
  bkg_title->draw_bg();

  update_time = st_get_ticks();
  random_timer.start(rand() % 2000 + 2000);

  Menu::set_current(main_menu);
  while (Menu::current())
    {
      // if we spent to much time on a menu entry
      if( (update_time - last_update_time) > 1000)
        update_time = last_update_time = st_get_ticks();

      // Calculate the movement-factor
      double frame_ratio = ((double)(update_time-last_update_time))/((double)FRAME_RATE);
      if(frame_ratio > 1.5) /* Quick hack to correct the unprecise CPU clocks a little bit. */
        frame_ratio = 1.5 + (frame_ratio - 1.5) * 0.85;
      /* Lower the frame_ratio that Tux doesn't jump to hectically throught the demo. */
      frame_ratio /= 2;

      SDL_Event event;
      while (SDL_PollEvent(&event))
        {
          if (Menu::current())
            {
              Menu::current()->event(event);
            }
         // FIXME: QUIT signal should be handled more generic, not locally
          if (event.type == SDL_QUIT)
            Menu::set_current(0);
        }

      /* Draw the background: */
      draw_demo(&session, frame_ratio);
      
      if (Menu::current() == main_menu)
        logo->draw( 160, 30);

      white_small_text->draw(" SuperTux " VERSION "\n"
                             "Copyright (c) 2003 SuperTux Devel Team\n"
                             "This game comes with ABSOLUTELY NO WARRANTY. This is free software, and you\n"
                             "are welcome to redistribute it under certain conditions; see the file COPYING\n"
                             "for details.\n",
                             0, 420, 0);

      /* Don't draw menu, if quit is true */
      Menu* menu = Menu::current();
      if(menu)
        {
          menu->draw();
          menu->action();
        
          if(menu == main_menu)
            {
              MusicManager* music_manager;
	      MusicRef menu_song;
              switch (main_menu->check())
                {
                case MNID_STARTGAME:
                  // Start Game, ie. goto the slots menu
                  update_load_save_game_menu(load_game_menu);
                  break;
                case MNID_CONTRIB:
                  // Contrib Menu
                  puts("Entering contrib menu");
                  generate_contrib_menu();
                  break;
                case MNID_ADDONS:
                  // Addons Menu
                  puts("Entering addons menu");
                  generate_addons_menu();
                  break;
                case MNID_LEVELEDITOR:
                  leveleditor();
                  Menu::set_current(main_menu);
                  break;
                case MNID_CREDITS:
                  music_manager = new MusicManager();
                  menu_song  = music_manager->load_music("music/credits.ogg");
                  music_manager->halt_music();
                  music_manager->play_music(menu_song,0);
                  display_text_file("CREDITS", bkg_title, SCROLL_SPEED_CREDITS);
                  music_manager->halt_music();
                  menu_song = music_manager->load_music("music/theme.mod");
                  music_manager->play_music(menu_song);
                  Menu::set_current(main_menu);
                  break;
                case MNID_QUITMAINMENU:
                  Menu::set_current(0);
                  break;
                }
            }
          else if(menu == options_menu)
            {
              process_options_menu();
            }
          else if(menu == load_game_menu)
            {
              if(event.key.keysym.sym == SDLK_DELETE)
                {
                int slot = menu->get_active_item_id();
                char str[1024];
                sprintf(str,"Are you sure you want to delete slot %d?", slot);
                
                draw_background();

                if(confirm_dialog(str))
                  {
                  sprintf(str,"%s/slot%d.stsg", st_save_dir, slot);
                  printf("Removing: %s\n",str);
                  remove(str);
                  }

                update_load_save_game_menu(load_game_menu);
                update_time = st_get_ticks();
                Menu::set_current(main_menu);
                }
              else if (process_load_game_menu())
                {
                  // FIXME: shouldn't be needed if GameSession doesn't relay on global variables
                  // reset tux
                  scroll_x = 0;
                  //titletux.level_begin();
                  update_time = st_get_ticks();
                }
            }
          else if(menu == contrib_menu)
            {
              check_contrib_menu();
            }
          else if (menu == contrib_subset_menu)
            {
              check_contrib_subset_menu();
            }
          else if(menu == addons_menu)
            {
              check_addons_menu();
            }
        }

      mouse_cursor->draw();
      
      flipscreen();

      /* Set the time of the last update and the time of the current update */
      last_update_time = update_time;
      update_time = st_get_ticks();

      /* Pause: */
      frame++;
      SDL_Delay(25);
    }
  /* Free surfaces: */

  free_contrib_menu();
  delete bkg_title;
  delete logo;
  delete img_choose_subset;
}

// EOF //

