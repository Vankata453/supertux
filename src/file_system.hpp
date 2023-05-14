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

#ifndef SUPERTUX_FILE_SYSTEM_HEADER
#define SUPERTUX_FILE_SYSTEM_HEADER

#include <json/json.h>

#include <string>
#include <vector>

namespace FileSystem
{
  // A class that takes the last PhysicsFS error and converts it to a readable message.
  class PhysfsError final : public std::exception
  {
  private:
    std::string m_message;

  public:
    PhysfsError(const std::string& message, const std::string& action) throw();

    const char* what() const throw() { return m_message.c_str(); }
  };

  // General file system management functions.
  void init();
  void deinit();

  std::vector<std::string> get_files(const std::string dir = "");
  bool file_exists(const std::string& file);

  std::string read_file(const std::string& file);
  void write_file(const std::string& file, const std::string& data);

  void mkdir(const std::string& name);

  std::string join(const std::string& lhs, const std::string& rhs);
  std::string extension(const std::string& filename);
}

#endif
