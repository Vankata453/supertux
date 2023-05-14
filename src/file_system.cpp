//  TuxJump
//  Copyright (C) 2022 Vankata453
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

#include "file_system.hpp"

#include <iostream>
#include <physfs.h>

#include "globals.h"

// Initalize a PhysicsFS error.
FileSystem::PhysfsError::PhysfsError(const std::string& message, const std::string& action) throw() :
  m_message()
{
  const PHYSFS_ErrorCode code = PHYSFS_getLastErrorCode();
  m_message = message + ": PHYSFS_" + action + " failed: " +
              PHYSFS_getErrorByCode(code) + " (" + std::to_string(code) + ")";
}

// Initialize PhysicsFS.
void
FileSystem::init()
{
  if (!PHYSFS_init(NULL))
    throw PhysfsError("Cannot initialize PhysicsFS", "init");

  // Locate and set user directory.
  const char* userdir = PHYSFS_getPrefDir("SuperTux Team", "SuperTux 0.1.3");
  if (userdir == NULL)
    throw PhysfsError("Error locating user data directory", "getPrefDir");
  if (!PHYSFS_setWriteDir(userdir))
    throw PhysfsError("Error setting user data directory as write directory", "setWriteDir");

  // Mount the default data directory.
  if (!PHYSFS_mount(datadir.c_str(), NULL, 1))
    throw PhysfsError("Cannot mount 'data' directory", "mount");

  datadir.clear();

  // Mount the user directory.
  if (!PHYSFS_mount(userdir, NULL, 1))
    throw PhysfsError("Cannot mount user data directory", "mount");

  // DEBUG
  std::cout << "Found files: ";
  for (const auto& file : get_files())
  {
    std::cout << file << ", ";
  }
  std::cout << "end" << std::endl;
}

// De-initialize PhysicsFS.
void
FileSystem::deinit()
{
  if (!PHYSFS_deinit())
    throw PhysfsError("Cannot properly de-initialize PhysicsFS", "deinit");
}


// Get all files and folders in a directory.
std::vector<std::string>
FileSystem::get_files(const std::string dir)
{
  char **rc = PHYSFS_enumerateFiles(dir.c_str());
  if (rc == NULL)
    throw PhysfsError("Error enumerating files in directory '" + dir + "'", "enumerateFiles");

  std::vector<std::string> files;
  char **i;
  for (i = rc; *i != NULL; i++)
    files.push_back(*i);

  PHYSFS_freeList(rc);
  return files;
}

bool
FileSystem::file_exists(const std::string& file)
{
  return PHYSFS_exists(file.c_str());
}


// Read a specified file.
std::string
FileSystem::read_file(const std::string& file)
{
  // Check if the file exists.
  if (!PHYSFS_exists(file.c_str()))
    return "";

  PHYSFS_file* handle = PHYSFS_openRead(file.c_str());
  const int length = PHYSFS_fileLength(handle);

  char* buffer = new char[length + 1];
  buffer[length] = 0; // Terminate string at the end.
  if (PHYSFS_readBytes(handle, buffer, length) <= 0)
    throw PhysfsError("Cannot read any data from file '" + file + "'", "readBytes");

  const std::string str = buffer;
  PHYSFS_close(handle);
  return str;
}

// Write to a specified file.
void
FileSystem::write_file(const std::string& file, const std::string& data)
{
  PHYSFS_file* handle = PHYSFS_openWrite(file.c_str());
  const int length = data.size();

  const char* buffer = data.c_str();
  if (PHYSFS_writeBytes(handle, buffer, length) < length)
    throw PhysfsError("Cannot write all data to file '" + file + "'", "writeBytes");

  PHYSFS_close(handle);
}


void
FileSystem::mkdir(const std::string& name)
{
  if (!PHYSFS_mkdir(name.c_str()))
    throw PhysfsError("Error creating folder '" + name + "'", "mkdir");
}


// Join two parts of a path together.
std::string
FileSystem::join(const std::string& lhs, const std::string& rhs)
{
  return lhs + "/" + rhs;
}

std::string
FileSystem::extension(const std::string& filename)
{
  std::string::size_type p = filename.find_last_of('.');
  if (p == std::string::npos)
    return "";

  return filename.substr(p);
}
