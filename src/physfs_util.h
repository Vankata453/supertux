//  SuperTux -  A Jump'n Run - PhysFS utilities
//  Copyright (C) 2025 Vankata453 & other SuperTux M2 contributors
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

#ifndef SUPERTUX_PHYSFS_UTIL_H
#define SUPERTUX_PHYSFS_UTIL_H

#include <cstdarg>
#include <vector>

#include <physfs.h>

struct SDL_RWops;

// Class to handle parsing a file char-by-char in 4KB chunks.
class PHYSFS_FileCharReader final
{
private:
  PHYSFS_File* file;
  char buffer[4096];
  std::vector<int> bytes_read;
  size_t next_idx;

public:
  PHYSFS_FileCharReader(PHYSFS_File* file);

  bool next_char(char& c);
  bool prev_char(char& c);
};

template<size_t len>
PHYSFS_sint64 PHYSFS_writeFormatted(PHYSFS_File* file, const char (&str)[len], ...)
{
  const size_t bufsize = len + 128;
  char buffer[bufsize];

  va_list args;
  va_start(args, str);
  const int written = vsnprintf(buffer, bufsize, str, args);
  va_end(args);

  if (written < 0)
    return -1;

  return PHYSFS_writeBytes(file, buffer,
    static_cast<int>(written < sizeof(buffer) ? written : sizeof(buffer) - 1));
}

SDL_RWops* get_physfs_SDLRWops(const char* filename);
SDL_RWops* get_writable_physfs_SDLRWops(const char* filename);

#endif

/* EOF */

