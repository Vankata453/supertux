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

#include "physfs_util.h"

#include <cassert>
#include <stdexcept>

#include <SDL.h>

PHYSFS_FileCharReader::PHYSFS_FileCharReader(PHYSFS_File* file_) :
  file(file_),
  buffer(),
  bytes_read(),
  next_idx(0)
{}

bool
PHYSFS_FileCharReader::next_char(char& c)
{
  // Reached next chunk?
  if (bytes_read.empty() || next_idx >= bytes_read.back())
  {
    bytes_read.push_back(static_cast<int>(PHYSFS_readBytes(file, &buffer, sizeof(buffer))));
    next_idx = 1;

    // Reached EOF or error?
    if (bytes_read.back() <= 0)
      return false;

    c = buffer[0];
    return true;
  }

  c = buffer[next_idx++];
  return true;
}

bool
PHYSFS_FileCharReader::prev_char(char& c)
{
  if (bytes_read.empty()) return false;

  assert(next_idx != 0);

  // Going back to previous chunk?
  if (next_idx == 1 && bytes_read.size() >= 2)
  {
    const PHYSFS_sint64 pos = PHYSFS_tell(file);
    assert(pos > 0);

    const int last_bytes_read = bytes_read.back();
    bytes_read.pop_back();
    PHYSFS_seek(file, pos - last_bytes_read - bytes_read.back());

    bytes_read.back() = static_cast<int>(PHYSFS_readBytes(file, &buffer, sizeof(buffer)));

    next_idx = bytes_read.back();
    return buffer[next_idx - 1];
  }

  return buffer[--next_idx - 1];
}


int funcSeek(struct SDL_RWops* context, int offset, int whence)
{
  PHYSFS_file* file = static_cast<PHYSFS_file*>(context->hidden.unknown.data1);
  int res;
  switch (whence) {
    case SEEK_SET:
      res = PHYSFS_seek(file, offset);
      break;
    case SEEK_CUR:
      res = PHYSFS_seek(file, PHYSFS_tell(file) + offset);
      break;
    case SEEK_END:
      res = PHYSFS_seek(file, PHYSFS_fileLength(file) + offset);
      break;
    default:
      assert(false);
      break;
  }
  if (res == 0) {
    throw std::runtime_error("Error seeking in file: " + std::string(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
  }
  return static_cast<int>(PHYSFS_tell(file));
}

int funcRead(struct SDL_RWops* context, void* ptr, int size, int maxnum)
{
  PHYSFS_file* file = static_cast<PHYSFS_file*>(context->hidden.unknown.data1);

  const int res = static_cast<int>(PHYSFS_readBytes(file, ptr, size * maxnum));
  if (res < 0)
    return 0;
  else
    return res / size;
}

int funcWrite(struct SDL_RWops* context, const void* ptr, int size, int num)
{
  PHYSFS_file* file = static_cast<PHYSFS_file*>(context->hidden.unknown.data1);

  const int res = static_cast<int>(PHYSFS_writeBytes(file, ptr, size * num));
  if (res < 0)
    return 0;
  else
    return res / size;
}

int funcClose(struct SDL_RWops* context)
{
  PHYSFS_close(static_cast<PHYSFS_file*>(context->hidden.unknown.data1));
  delete context;

  return 0;
}


SDL_RWops* get_physfs_SDLRWops(const char* filename)
{
  // check this as PHYSFS seems to be buggy and still returns a
  // valid pointer in this case
  if (!filename)
    throw std::runtime_error("Couldn't open file: empty filename");

  PHYSFS_file* file = static_cast<PHYSFS_file*>(PHYSFS_openRead(filename));
  if (!file)
    throw std::runtime_error("Couldn't open '" + std::string(filename) + "': " + std::string(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));

  SDL_RWops* ops = new SDL_RWops;
  ops->seek = funcSeek;
  ops->read = funcRead;
  ops->write = funcWrite;
  ops->close = funcClose;
  ops->type = 0;
  ops->hidden.unknown.data1 = file;

  return ops;
}

SDL_RWops* get_writable_physfs_SDLRWops(const char* filename)
{
  // check this as PHYSFS seems to be buggy and still returns a
  // valid pointer in this case
  if (!filename)
    throw std::runtime_error("Couldn't open file: empty filename");

  PHYSFS_file* file = static_cast<PHYSFS_file*>(PHYSFS_openWrite(filename));
  if (!file)
    throw std::runtime_error("Couldn't open '" + std::string(filename) + "' for writing: " + std::string(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));

  SDL_RWops* ops = new SDL_RWops;
  ops->seek = funcSeek;
  ops->read = funcRead;
  ops->write = funcWrite;
  ops->close = funcClose;
  ops->type = 0;
  ops->hidden.unknown.data1 = file;

  return ops;
}

/* EOF */
