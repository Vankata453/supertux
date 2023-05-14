//  $Id$
// 
//  SuperTux
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

#include "tile.h"
#include "scene.h"
#include "assert.h"
#include "reader/reader_document.hpp"
#include "reader/reader_mapping.hpp"

TileManager* TileManager::instance_  = 0;
std::set<TileGroup>* TileManager::tilegroups_  = 0;

Tile::Tile()
{
}

Tile::~Tile()
{
  for(std::vector<Surface*>::iterator i = images.begin(); i != images.end();
      ++i) {
    delete *i;
  }
  for(std::vector<Surface*>::iterator i = editor_images.begin();
      i != editor_images.end(); ++i) {
    delete *i;                                                                
  }
}

//---------------------------------------------------------------------------

TileManager::TileManager()
{
  load_tileset("images/tilesets/supertux.stgt");
}

TileManager::~TileManager()
{
  for(std::vector<Tile*>::iterator i = tiles.begin(); i != tiles.end(); ++i) {
    delete *i;                                                                  
  }
}

void TileManager::load_tileset(const std::string& filename)
{
  if(filename == current_tileset)
    return;
  
  // free old tiles
  for(std::vector<Tile*>::iterator i = tiles.begin(); i != tiles.end(); ++i) {
    delete *i;
  }
  tiles.clear();

  try
  {
    ReaderDocument doc = ReaderDocument::from_file(filename);
    ReaderObject root = doc.get_root();
    if (root.get_name() != "supertux-tiles")
    {
      throw std::runtime_error("File is not a supertux-tiles file");
    }
    ReaderIterator iter = root.get_mapping().get_iter();

    int tileset_id = 0;
    while (iter.next())
    {
      if (iter.get_key() == "tile")
      {
        ReaderMapping reader = iter.as_mapping();

        Tile* tile = new Tile;
        tile->id      = -1;
        tile->solid   = false;
        tile->brick   = false;
        tile->ice     = false;
        tile->water   = false;
        tile->fullbox = false;
        tile->distro  = false;
        tile->goal    = false;
        tile->data    = 0;
        tile->next_tile  = 0;
        tile->anim_speed = 25;

        assert(reader.read_int("id",  &tile->id));
        reader.read_bool("solid",     &tile->solid);
        reader.read_bool("brick",     &tile->brick);
        reader.read_bool("ice",       &tile->ice);
        reader.read_bool("water",     &tile->water);
        reader.read_bool("fullbox",   &tile->fullbox);
        reader.read_bool("distro",    &tile->distro);
        reader.read_bool("goal",      &tile->goal);
        reader.read_int("data",       &tile->data);
        reader.read_int("anim-speed", &tile->anim_speed);
        reader.read_int("next-tile",  &tile->next_tile);
        reader.read_string_vector("images",  &tile->filenames);
        reader.read_string_vector("editor-images", &tile->editor_filenames);

        for(std::vector<std::string>::iterator it = tile->
            filenames.begin();
            it != tile->filenames.end();
            ++it)
        {
          Surface* cur_image;
          tile->images.push_back(cur_image);
          tile->images[tile->images.size()-1] = new Surface(
                       "images/tilesets/" + (*it),
                       USE_ALPHA);
        }
        for(std::vector<std::string>::iterator it = tile->editor_filenames.begin();
            it != tile->editor_filenames.end();
            ++it)
        {
          Surface* cur_image;
          tile->editor_images.push_back(cur_image);
          tile->editor_images[tile->editor_images.size()-1] = new Surface(
                       "images/tilesets/" + (*it),
                       USE_ALPHA);
        }

        if (tile->id + tileset_id >= int(tiles.size()))
          tiles.resize(tile->id + tileset_id+1);

        tiles[tile->id + tileset_id] = tile;
      }
      else if (iter.get_key() == "tileset")
      {
        ReaderMapping reader = iter.as_mapping();

        std::string filename;
        reader.read_string("file",  &filename);
        filename = "images/tilesets/" + filename;
        load_tileset(filename);
      }
      else if (iter.get_key() == "tilegroup")
      {
        ReaderMapping reader = iter.as_mapping();

        TileGroup new_;
        reader.read_string("name",  &new_.name);
        reader.read_int_vector("tiles", &new_.tiles);	      
        if(!tilegroups_)
          tilegroups_ = new std::set<TileGroup>;
        tilegroups_->insert(new_).first;
      }
      else if (iter.get_key() == "properties")
      {
        ReaderMapping reader = iter.as_mapping();

        reader.read_int("id",  &tileset_id);
        tileset_id *= 1000;
      }
      else
      {
        puts("Unhandled symbol");
      }
    }

    current_tileset = filename;
  }
  catch (std::exception& err)
  {
    std::cout << "Couldn't load tileset '" << filename << "': " << err.what() << std::endl;
  }
}

void
Tile::draw(float x, float y, unsigned int c, Uint8 alpha)
{
  if (c != 0)
    {
      Tile* ptile = TileManager::instance()->get(c);
      if(ptile)
        {
          if(ptile->images.size() > 1)
            {
              ptile->images[( ((global_frame_counter*25) / ptile->anim_speed) % (ptile->images.size()))]->draw(x,y, alpha);
            }
          else if (ptile->images.size() == 1)
            {
              ptile->images[0]->draw(x,y, alpha);
            }
          else
            {
              //printf("Tile not dravable %u\n", c);
            }
        }
    }
}

void
Tile::draw_stretched(float x, float y, int w, int h, unsigned int c, Uint8 alpha)
{
  if (c != 0)
    {
      Tile* ptile = TileManager::instance()->get(c);
      if(ptile)
        {
          if(ptile->images.size() > 1)
            {
              ptile->images[( ((global_frame_counter*25) / ptile->anim_speed) % (ptile->images.size()))]->draw_stretched(x,y,w,h, alpha);
            }
          else if (ptile->images.size() == 1)
            {
              ptile->images[0]->draw_stretched(x,y, w, h, alpha);
            }
          else
            {
              //printf("Tile not dravable %u\n", c);
            }
        }
    }
}

// EOF //

