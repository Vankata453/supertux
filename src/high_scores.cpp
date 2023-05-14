//  $Id$
// 
//  SuperTux
//  Copyright (C) 2004 Adam Czachorowski <gislan@o2.pl>
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

/* Open the highscore file: */

#include <iostream>
#include <string.h>
#include <stdlib.h>

#include "globals.h"
#include "high_scores.h"
#include "menu.h"
#include "screen.h"
#include "texture.h"
#include "reader/reader_document.hpp"
#include "reader/reader_mapping.hpp"
#include "reader/writer.hpp"

#ifdef WIN32
const char * highscore_filename = "/st_highscore.dat";
#else
const char * highscore_filename = "/highscore";
#endif

int hs_score;
std::string hs_name; /* highscores global variables*/

/* Load data from high score file: */

void load_hs(void)
{
  hs_score = 100;
  hs_name  = "Grandma";

  try
  {
    ReaderDocument doc = ReaderDocument::from_file(highscore_filename);
    ReaderObject root = doc.get_root();
    if (root.get_name() != "supertux-highscore")
    {
      throw std::runtime_error("File is not a supertux-highscore file");
    }
    ReaderMapping reader = root.get_mapping();

    reader.read_int("score",  &hs_score);
    reader.read_string("name", &hs_name);
  }
  catch (std::exception& err)
  {
    std::cout << "Couldn't load highscore data from '" << highscore_filename << "': " << err.what() << std::endl;
  }
}

void save_hs(int score)
{
  char str[80];

  Surface* bkgd;
  SDL_Event event;

  bkgd = new Surface("images/highscore/highscore.png", IGNORE_ALPHA);

  hs_score = score;

  Menu::set_current(highscore_menu);

  if(!highscore_menu->item[0].input)
    highscore_menu->item[0].input = (char*) malloc(strlen(hs_name.c_str()) + 1);

  strcpy(highscore_menu->item[0].input,hs_name.c_str());

  /* ask for player's name */
  while(Menu::current())
    {
      bkgd->draw_bg();

      blue_text->drawf("Congratulations", 0, 130, A_HMIDDLE, A_TOP, 2, NO_UPDATE);
      blue_text->draw("Your score:", 150, 180, 1, NO_UPDATE);
      sprintf(str, "%d", hs_score);
      yellow_nums->draw(str, 350, 170, 1, NO_UPDATE);

      Menu::current()->draw();
      Menu::current()->action();

      flipscreen();

      while(SDL_PollEvent(&event))
        if(event.type == SDL_KEYDOWN)
          Menu::current()->event(event);

      switch (highscore_menu->check())
        {
        case 0:
          if(highscore_menu->item[0].input != NULL)
            hs_name = highscore_menu->item[0].input;
          break;
        }

      SDL_Delay(25);
    }


  /* Save to file: */
  Writer writer(highscore_filename);

  writer.write_comment("SuperTux HighScores");
  writer.start_list("supertux-highscore");

  writer.write("name", hs_name);
  writer.write("score", hs_score);

  writer.end_list("supertux-highscore");
}
