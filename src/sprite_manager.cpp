//  $Id$
//
//  SuperTux
//  Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
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
#include "sprite_manager.h"
#include "reader/reader_document.hpp"
#include "reader/reader_mapping.hpp"

SpriteManager::SpriteManager(const std::string& filename)
{
  load_resfile(filename);
}

SpriteManager::~SpriteManager()
{
  for(std::map<std::string, Sprite*>::iterator i = sprites.begin();
      i != sprites.end(); ++i) {
    delete i->second;
  }
}

void
SpriteManager::load_resfile(const std::string& filename)
{
  try
  {
    ReaderDocument doc = ReaderDocument::from_file(filename);
    ReaderObject root = doc.get_root();
    if (root.get_name() != "supertux-resources")
    {
      throw std::runtime_error("File is not a supertux-resources file");
    }
    ReaderMapping reader = root.get_mapping();

    ReaderIterator iter = reader.get_iter();
    while (iter.next())
    {
      if (iter.get_key() == "sprite")
      {
        Sprite* sprite = new Sprite(iter.as_mapping());

        Sprites::iterator i = sprites.find(sprite->get_name());
        if (i == sprites.end())
        {
          sprites[sprite->get_name()] = sprite;
        }
        else
        {
          delete i->second;
          i->second = sprite;
          std::cout << "Warning: dulpicate entry: '" << sprite->get_name() << "'" << std::endl;
        }
      }
      else
      {
        std::cout << "SpriteManager: Unknown tag" << std::endl;
      }
    }
  }
  catch (std::exception& err)
  {
    std::cout << "SpriteManager: Couldn't load '" << filename << "': " << err.what() << std::endl;
  }
}

Sprite*
SpriteManager::load(const std::string& name)
{
  Sprites::iterator i = sprites.find(name);
  if (i != sprites.end())
    {
      return i->second;
    }
  else
    {
      std::cout << "SpriteManager: Sprite '" << name << "' not found" << std::endl;
      return 0;
    }
}

/* EOF */
