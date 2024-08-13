//  SuperTux
//  Copyright (C) 2024 Vankata453
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

#include "addon/addon_index.hpp"

#include <sstream>

#include "addon/addon.hpp"
#include "util/log.hpp"
#include "util/reader_document.hpp"
#include "util/reader_iterator.hpp"
#include "util/reader_mapping.hpp"

std::unique_ptr<AddonIndex>
AddonIndex::parse(const std::string& index)
{
  try
  {
    //register_translation_directory(filename);
    auto doc = ReaderDocument::from_string(index);
    auto root = doc.get_root();
    if (root.get_name() != "supertux-addons")
    {
      throw std::runtime_error("Not a \"supertux-addons\" index!");
    }

    return std::make_unique<AddonIndex>(root.get_mapping());
  }
  catch (const std::exception& e)
  {
    std::stringstream msg;
    msg << "Problem when parsing add-on index: " << e.what();
    throw std::runtime_error(msg.str());
  }
}

std::unique_ptr<Addon>
AddonIndex::parse_addon(const std::string& index, const std::string& addon_id)
{
  try
  {
    //register_translation_directory(filename);
    auto doc = ReaderDocument::from_string(index);
    auto root = doc.get_root();
    if (root.get_name() != "supertux-addons")
    {
      throw std::runtime_error("Not a \"supertux-addons\" index!");
    }

    auto iter = root.get_mapping().get_iter();
    while (iter.next())
    {
      if (iter.get_key() != "supertux-addoninfo")
        continue;

      const ReaderMapping& addon_mapping = iter.as_mapping();

      std::string id;
      addon_mapping.get("id", id);
      if (id == addon_id)
        return std::make_unique<Addon>(addon_mapping);
    }
  }
  catch (const std::exception& e)
  {
    std::stringstream msg;
    msg << "Problem when parsing add-on index: " << e.what();
    throw std::runtime_error(msg.str());
  }
  return nullptr;
}

AddonIndex::AddonIndex(const ReaderMapping& mapping) :
  m_addons(),
  m_previous_page_url(),
  m_next_page_url(),
  m_total_pages()
{
  auto iter = mapping.get_iter();
  while (iter.next())
  {
    const std::string key = iter.get_key();
    if (key == "previous-page")
      iter.get(m_previous_page_url);
    else if (key == "next-page")
      iter.get(m_next_page_url);
    else if (key == "total-pages")
      iter.get(m_total_pages);
    else if (key == "supertux-addoninfo")
    {
      try
      {
        m_addons.push_back(std::make_unique<Addon>(iter.as_mapping()));
      }
      catch (const std::exception& err)
      {
        log_warning << "Error parsing add-on from index: " << err.what() << std::endl;
      }
    }
    else
      throw std::runtime_error("Unknown entry '" + key + "'!");
  }
}

/* EOF */
