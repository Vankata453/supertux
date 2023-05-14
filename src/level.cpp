//  $Id$
// 
//  SuperTux
//  Copyright (C) 2004 SuperTux Development Team, see AUTHORS for details
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

#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "globals.h"
#include "screen.h"
#include "level.h"
#include "physic.h"
#include "scene.h"
#include "tile.h"
#include "resources.h"
#include "music_manager.h"
#include "file_system.hpp"
#include "reader/reader_document.hpp"
#include "reader/reader_mapping.hpp"
#include "reader/writer.hpp"

using namespace std;

LevelSubset::LevelSubset()
    : image(0), levels(0)
{
}

LevelSubset::~LevelSubset()
{
  delete image;
}

void LevelSubset::create(const std::string& subset_name)
{
  Level new_lev;
  LevelSubset new_subset;
  new_subset.name = subset_name;
  new_subset.title = "Unknown Title";
  new_subset.description = "No description so far.";
  new_subset.save();
  new_lev.init_defaults();
  new_lev.save(subset_name, 1);
}

void LevelSubset::parse(const ReaderMapping& reader)
{
  reader.get("title", title);
  reader.get("description", description);
}

void LevelSubset::load(const std::string& subset)
{
  char str[1024];
  name = subset;

  const std::string filename = "levels/" + subset + "/info";
  try
  {
    ReaderDocument doc = ReaderDocument::from_file(filename);
    ReaderObject root = doc.get_root();
    if (root.get_name() != "supertux-level-subset")
    {
      throw std::runtime_error("File is not a supertux-level-subset file");
    }
    ReaderMapping reader = root.get_mapping();

    parse(reader);

    if(FileSystem::file_exists(filename + ".png"))
    {
      delete image;
      image = new Surface(filename + ".png", IGNORE_ALPHA);
    }
    else
    {
      image = new Surface("images/status/level-subset-info.png",IGNORE_ALPHA);
    }
  }
  catch (std::exception& err)
  {
    std::cout << "Couldn't read all level subset data '" << filename << "': " << err.what() << std::endl;
  }

  int i;
  for (i=1; i != -1; ++i)
  {
    /* Get the number of levels in this subset */
    std::stringstream level_filename;
    level_filename << "levels/" << subset + "/level" << i << ".stl";
    if (!FileSystem::file_exists(level_filename.str()))
      break;
  }
  levels = --i;
}

void LevelSubset::save()
{
  FileSystem::mkdir("levels/" + name); // Create directory if it doesn't exist
  Writer writer("levels/" + name + "/info");

  writer.write_comment("SuperTux-Level-Subset");
  writer.start_list("supertux-level-subset");

  writer.write("title", title);
  writer.write("description", description);

  writer.end_list("supertux-level-subset");
}

Level::Level()
  : img_bkgd(0)
{
  init_defaults();
}

Level::Level(const std::string& subset, int level)
  : img_bkgd(0)
{
  if(load(subset, level) < 0)
    st_abort("Couldn't load level from subset", subset.c_str());
}

Level::Level(const std::string& filename)
  : img_bkgd(0)
{
  if(load(filename) < 0)
    st_abort("Couldn't load level " , filename.c_str());
}

Level::~Level()
{
  delete img_bkgd;
}

void
Level::init_defaults()
{
  name       = "UnNamed";
  author     = "UnNamed";
  song_title = "Mortimers_chipdisko.mod";
  bkgd_image = "arctis.png";
  width      = 21;
  start_pos_x = 100;
  start_pos_y = 170;
  time_left  = 100;
  gravity    = 10.;
  back_scrolling = false;
  hor_autoscroll_speed = 0;
  bkgd_speed = 50;
  bkgd_top.red   = 0;
  bkgd_top.green = 0;
  bkgd_top.blue  = 0;
  bkgd_bottom.red   = 255;
  bkgd_bottom.green = 255;
  bkgd_bottom.blue  = 255;

  for(int i = 0; i < 15; ++i)
    {
      ia_tiles[i].resize(width+1, 0);
      ia_tiles[i][width] = (unsigned int) '\0';

      for(int y = 0; y < width; ++y)
        ia_tiles[i][y] = 0;

      bg_tiles[i].resize(width+1, 0);
      bg_tiles[i][width] = (unsigned int) '\0';
      for(int y = 0; y < width; ++y)
        bg_tiles[i][y] = 0;

      fg_tiles[i].resize(width+1, 0);
      fg_tiles[i][width] = (unsigned int) '\0';
      for(int y = 0; y < width; ++y)
        fg_tiles[i][y] = 0;
    }
}

int
Level::load(const std::string& subset, int level)
{
  char filename[1024];

  // Load data file:
  snprintf(filename, 1024, "levels/%s/level%d.stl", subset.c_str(), level);

  return load(filename);
}

int 
Level::load(const std::string& filename)
{
  try
  {
    ReaderDocument doc = ReaderDocument::from_file(filename);
    ReaderObject root = doc.get_root();
    if (root.get_name() != "supertux-level")
    {
      throw std::runtime_error("File is not a supertux-level file");
    }
    ReaderMapping reader = root.get_mapping();

    vector<int> ia_tm;
    vector<int> bg_tm;
    vector<int> fg_tm;

    int version = 0;

    reader.read_int("version",  &version);
    if(!reader.read_int("width",  &width))
      st_abort("No width specified for level.", "");
    if (!reader.read_int("start_pos_x", &start_pos_x)) start_pos_x = 100;
    if (!reader.read_int("start_pos_y", &start_pos_y)) start_pos_y = 170;
    time_left = 500;
    if(!reader.read_int("time",  &time_left)) {
      printf("Warning no time specified for level.\n");
    }

    back_scrolling = false;
    reader.read_bool("back_scrolling",  &back_scrolling);

    hor_autoscroll_speed = 0;
    reader.read_float("hor_autoscroll_speed",  &hor_autoscroll_speed);

    bkgd_speed = 50;
    reader.read_int("bkgd_speed",  &bkgd_speed);


    bkgd_top.red = bkgd_top.green = bkgd_top.blue = 0;
    reader.read_int("bkgd_red_top",  &bkgd_top.red);
    reader.read_int("bkgd_green_top",  &bkgd_top.green);
    reader.read_int("bkgd_blue_top",  &bkgd_top.blue);

    bkgd_bottom.red = bkgd_bottom.green = bkgd_bottom.blue = 0;
    reader.read_int("bkgd_red_bottom",  &bkgd_bottom.red);
    reader.read_int("bkgd_green_bottom",  &bkgd_bottom.green);
    reader.read_int("bkgd_blue_bottom",  &bkgd_bottom.blue);

    gravity = 10;
    reader.read_float("gravity",  &gravity);
    name = "Noname";
    reader.read_string("name",  &name);
    author = "unknown author";
    reader.read_string("author", &author);
    song_title = "";
    reader.read_string("music",  &song_title);
    bkgd_image = "";
    reader.read_string("background",  &bkgd_image);
    particle_system = "";
    reader.read_string("particle_system", &particle_system);

    reader.read_int_vector("background-tm",  &bg_tm);

    if (!reader.read_int_vector("interactive-tm", &ia_tm))
      reader.read_int_vector("tilemap", &ia_tm);

    reader.read_int_vector("foreground-tm",  &fg_tm);

    ReaderIterator iter = reader.get_iter();
    while (iter.next())
    {
      if (iter.get_key() == "reset-points") // Read ResetPoints
      {
        ReaderIterator rp_iter = iter.as_mapping().get_iter();
        while (rp_iter.next())
        {
          ReaderMapping rp_reader = rp_iter.as_mapping();

          ResetPoint pos;
          if (rp_reader.read_int("x", &pos.x)
              && rp_reader.read_int("y", &pos.y))
          {
            reset_points.push_back(pos);
          }
        }
      }
      else if (iter.get_key() == "objects") // Read BadGuys
      {
        ReaderIterator obj_iter = iter.as_mapping().get_iter();
        while (obj_iter.next())
        {
          ReaderMapping obj_reader = obj_iter.as_mapping();

          BadGuyData bg_data;
          bg_data.kind = badguykind_from_string(obj_iter.get_key());
          obj_reader.read_int("x", &bg_data.x);
          obj_reader.read_int("y", &bg_data.y);
          obj_reader.read_bool("stay-on-platform", &bg_data.stay_on_platform);

          badguy_data.push_back(bg_data);
        }
      }
    }

    // Convert old levels to the new tile numbers
    if (version == 0)
      {
        std::map<char, int> transtable;
        transtable['.'] = 0;
        transtable['x'] = 104;
        transtable['X'] = 77;
        transtable['y'] = 78;
        transtable['Y'] = 105;
        transtable['A'] = 83;
        transtable['B'] = 102;
        transtable['!'] = 103;
        transtable['a'] = 84;
        transtable['C'] = 85;
        transtable['D'] = 86;
        transtable['E'] = 87;
        transtable['F'] = 88;
        transtable['c'] = 89;
        transtable['d'] = 90;
        transtable['e'] = 91;
        transtable['f'] = 92;

        transtable['G'] = 93;
        transtable['H'] = 94;
        transtable['I'] = 95;
        transtable['J'] = 96;

        transtable['g'] = 97;
        transtable['h'] = 98;
        transtable['i'] = 99;
        transtable['j'] = 100
                          ;
        transtable['#'] = 11;
        transtable['['] = 13;
        transtable['='] = 14;
        transtable[']'] = 15;
        transtable['$'] = 82;
        transtable['^'] = 76;
        transtable['*'] = 80;
        transtable['|'] = 79;
        transtable['\\'] = 81;
        transtable['&'] = 75;

        int x = 0;
        int y = 0;
        for(std::vector<int>::iterator i = ia_tm.begin(); i != ia_tm.end(); ++i)
          {
            if (*i == '0' || *i == '1' || *i == '2')
              {
                badguy_data.push_back(BadGuyData(static_cast<BadGuyKind>(*i-'0'),
                                                 x*32, y*32, false));
                *i = 0;
              }
            else
              {
                std::map<char, int>::iterator j = transtable.find(*i);
                if (j != transtable.end())
                  *i = j->second;
                else
                  printf("Error: conversion will fail, unsupported char: '%c' (%d)\n", *i, *i);
              }
            ++x;
            if (x >= width)
              {
                x = 0;
                ++y;
              }
          }
      }

    for(int i = 0; i < 15; ++i)
      {
        ia_tiles[i].resize(width + 1, 0);
        bg_tiles[i].resize(width + 1, 0);
        fg_tiles[i].resize(width + 1, 0);
      }

    int i = 0;
    int j = 0;
    for(vector<int>::iterator it = ia_tm.begin(); it != ia_tm.end(); ++it, ++i)
      {
        if(j >= 15)
          {
          std::cerr << "Warning: Level higher than 15 interactive tiles."
                       "Ignoring by cutting tiles.\n"
                       "The level might not be finishable anymore!\n";
          break;
          }

        ia_tiles[j][i] = (*it);
        if(i == width - 1)
          {
            i = -1;
            ++j;
          }
      }

    i = j = 0;
    for(vector<int>::iterator it = bg_tm.begin(); it != bg_tm.end(); ++it, ++i)
      {
        if(j >= 15)
          {
          std::cerr << "Warning: Level higher than 15 background tiles."
                       "Ignoring by cutting tiles.\n";
          break;
          }

        bg_tiles[j][i] = (*it);
        if(i == width - 1)
          {
            i = -1;
            ++j;
          }
      }

    i = j = 0;
    for(vector<int>::iterator it = fg_tm.begin(); it != fg_tm.end(); ++it, ++i)
      {
        if(j >= 15)
          {
          std::cerr << "Warning: Level higher than 15 foreground tiles."
                       "Ignoring by cutting tiles.\n";
          break;
          }

        fg_tiles[j][i] = (*it);
        if(i == width - 1)
          {
            i = -1;
            ++j;
          }
      }

    return 0;
  }
  catch (std::exception& err)
  {
    log_warning << "Error loading level '" << filename << "': " << err.what() << std::endl;
    return 1;
  }
}

/* Save data for level: */

void 
Level::save(const std::string& subset, int level)
{
  FileSystem::mkdir("levels/" + subset); // Create subset directory if it doesn't exist

  std::stringstream filename;
  filename << "levels/" << subset << "/level" << level << ".stl";

  Writer writer(filename.str());

  writer.write_comment("SuperTux-Level");
  writer.start_list("supertux-level");

  writer.write("version", 1);
  writer.write("name", name);
  writer.write("author", author);
  writer.write("music", song_title);
  writer.write("background", bkgd_image);
  writer.write("particle_system", particle_system);
  writer.write("bkgd_speed", bkgd_speed);
  writer.write("bkgd_red_top", bkgd_top.red);
  writer.write("bkgd_green_top", bkgd_top.green);
  writer.write("bkgd_blue_top", bkgd_top.blue);
  writer.write("bkgd_red_bottom", bkgd_bottom.red);
  writer.write("bkgd_green_bottom", bkgd_bottom.green);
  writer.write("bkgd_blue_bottom", bkgd_bottom.blue);
  writer.write("time", time_left);
  writer.write("width", width);
  writer.write("back_scrolling", back_scrolling);
  writer.write("hor_autoscroll_speed", hor_autoscroll_speed);
  writer.write("gravity", gravity);

  std::stringstream bg_tiles_out;
  for (const auto& tile_row : bg_tiles)
    for (int i = 0; i < width; i++)
      bg_tiles_out << tile_row[i] << " ";

  std::stringstream ia_tiles_out;
  for (const auto& tile_row : ia_tiles)
    for (int i = 0; i < width; i++)
      ia_tiles_out << tile_row[i] << " ";

  std::stringstream fg_tiles_out;
  for (const auto& tile_row : fg_tiles)
    for (int i = 0; i < width; i++)
      fg_tiles_out << tile_row[i] << " ";

  writer.write_raw("background-tm", bg_tiles_out.str());
  writer.write_raw("interactive-tm", ia_tiles_out.str());
  writer.write_raw("foreground-tm", fg_tiles_out.str());

  writer.start_list("reset-points");
  for(std::vector<ResetPoint>::iterator it = reset_points.begin();
     it != reset_points.end(); ++it)
  {
    writer.start_list("point");
    {
      writer.write("x", it->x);
      writer.write("y", it->y);
    }
    writer.end_list("point");
  }
  writer.end_list("reset-points");

  writer.start_list("objects");
  for(std::vector<BadGuyData>::iterator it = badguy_data.begin();
      it != badguy_data.end(); ++it)
  {
    const std::string kind = badguykind_to_string(it->kind);
    writer.start_list(kind);
    {
      writer.write("x", it->x);
      writer.write("y", it->y);
      writer.write("stay-on-platform", it->stay_on_platform);
    }
    writer.end_list(kind);
  }
  writer.end_list("objects");

  writer.end_list("supertux-level");
}


/* Unload data for this level: */

void
Level::cleanup()
{
  for(int i=0; i < 15; ++i)
    {
      bg_tiles[i].clear();
      ia_tiles[i].clear();
      fg_tiles[i].clear();
    }

  reset_points.clear();
  name = "";
  author = "";
  song_title = "";
  bkgd_image = "";

  badguy_data.clear();
}

void 
Level::load_gfx()
{
  if(!bkgd_image.empty())
    {
      char fname[1024];
      snprintf(fname, 1024, "%s/background/%s", st_dir, bkgd_image.c_str());
      if(!FileSystem::file_exists(fname))
        snprintf(fname, 1024, "%s/images/background/%s", datadir.c_str(), bkgd_image.c_str());
      delete img_bkgd;
      img_bkgd = new Surface(fname, IGNORE_ALPHA);
    }
  else
    {
      delete img_bkgd;
      img_bkgd = 0;
    }
}

/* Load a level-specific graphic... */
void Level::load_image(Surface** ptexture, string theme,const  char * file, int use_alpha)
{
  char fname[1024];

  snprintf(fname, 1024, "%s/themes/%s/%s", st_dir, theme.c_str(), file);
  if(!FileSystem::file_exists(fname))
    snprintf(fname, 1024, "%s/images/themes/%s/%s", datadir.c_str(), theme.c_str(), file);

  *ptexture = new Surface(fname, use_alpha);
}

/* Change the size of a level (width) */
void 
Level::change_size (int new_width)
{
  if(new_width < 21)
    new_width = 21;

  for(int y = 0; y < 15; ++y)
    {
      ia_tiles[y].resize(new_width, 0);
      bg_tiles[y].resize(new_width, 0);
      fg_tiles[y].resize(new_width, 0);
    }

  width = new_width;
}

void
Level::change(float x, float y, int tm, unsigned int c)
{
  int yy = ((int)y / 32);
  int xx = ((int)x / 32);

  if (yy >= 0 && yy < 15 && xx >= 0 && xx <= width)
    {
      switch(tm)
        {
        case TM_BG:
          bg_tiles[yy][xx] = c;
          break;
        case TM_IA:
          ia_tiles[yy][xx] = c;
          break;
        case TM_FG:
          fg_tiles[yy][xx] = c;
          break;
        }
    }
}

void
Level::load_song()
{
  char* song_path;
  char* song_subtitle;

  level_song = music_manager->load_music("music/" + song_title);

  song_path = (char *) malloc(sizeof(char) * datadir.length() +
                              strlen(song_title.c_str()) + 8 + 5);
  song_subtitle = strdup(song_title.c_str());
  strcpy(strstr(song_subtitle, "."), "\0");
  sprintf(song_path, "%s/music/%s-fast%s", datadir.c_str(), 
          song_subtitle, strstr(song_title.c_str(), "."));
  if(!music_manager->exists_music(song_path)) {
    level_song_fast = level_song;
  } else {
    level_song_fast = music_manager->load_music(song_path);
  }
  free(song_subtitle);
  free(song_path);
}

MusicRef
Level::get_level_music()
{
  return level_song;
}

MusicRef
Level::get_level_music_fast()
{
  return level_song_fast;
}

unsigned int 
Level::gettileid(float x, float y) const
{
  int xx, yy;
  unsigned int c;

  yy = ((int)y / 32);
  xx = ((int)x / 32);

  if (yy >= 0 && yy < 15 && xx >= 0 && xx <= width)
    c = ia_tiles[yy][xx];
  else
    c = 0;

  return c;
}

unsigned int
Level::get_tile_at(int x, int y) const
{
  if(x < 0 || x > width || y < 0 || y > 14)
    return 0;
  
  return ia_tiles[y][x];
}

/* EOF */
