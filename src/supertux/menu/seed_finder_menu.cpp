//  SuperTux
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

#include "supertux/menu/seed_finder_menu.hpp"

#include "gui/item_action.hpp"
#include "gui/item_numfield.hpp"
#include "gui/item_stringselect.hpp"
#include "gui/item_hl.hpp"
#include "gui/menu_manager.hpp"
#include "gui/menu_select_item.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "util/gettext.hpp"

std::unique_ptr<SeedFinder> SeedFinderMenu::s_seed_finder = nullptr;

SeedFinderMenu::SeedFinderMenu() :
  m_import_file()
{
  if (!s_seed_finder) s_seed_finder.reset(new SeedFinder(0));

  refresh();
}

void
SeedFinderMenu::refresh()
{
  clear();

  add_label(_("Seed Finder"));
  add_hl();

  add_file(_("Choose randomizations file"), &m_import_file, { ".stsf" });
  add_entry(MNID_IMPORTRANDOMIZATIONS, _("Import randomizations"));
  add_entry(MNID_SAVERANDOMIZATIONS, _("Save randomizations"));
  add_hl();

  if (!s_seed_finder->s_randomization_log.empty())
  {
    add_entry(MNID_IMPORTLOGGED, _("Import logged randomizations"));
    add_entry(MNID_CLEARLOGGED, _("Clear logged randomizations"));
    add_hl();
  }

  add_intfield(_("Initial seed"), &s_seed_finder->m_init_seed);
  add_numfield(_("Search time"), &s_seed_finder->m_search_time);
  add_intfield(_("Search threads"), &s_seed_finder->m_search_threads_count);
  add_hl();

  add_entry(MNID_ADDRANDOMIZATION, _("Add randomization"));
  add_entry(MNID_REMOVELASTRANDOMIZATION, _("Remove last randomization"));
  add_hl();

  add_entry(MNID_FINDSEED, _("Find seed"));
  add_inactive("")->id = MNID_STATUS;
  update_status(); // Set status info.
  add_hl();

  add_back(_("Back"));

  import_randomizations();
}

void
SeedFinderMenu::update_status(const SeedFinder::Status& status)
{
  std::string text;
  switch (status)
  {
    case SeedFinder::STATUS_OK:
      text = _("Ready.");
      break;
    case SeedFinder::STATUS_INPROGRESS:
      text = _("Searching...");
      break;
    case SeedFinder::STATUS_FOUND:
      text = _("Found! Seed: ") + std::to_string(s_seed_finder->get_seed());
      break;
    case SeedFinder::STATUS_TIMEDOUT:
      text = _("Seed search timed out.");
      break;
    default:
      text = _("Unknown status.");
      break;
  }

  // Toggle the use seed button.
  if (has_item_with_id(MNID_USESEED) && status != SeedFinder::STATUS_FOUND)
    delete_item(get_item_pos(MNID_USESEED));
  else if (status == SeedFinder::STATUS_FOUND)
    add_item(std::unique_ptr<MenuItem>(new ItemAction(_("Use seed"), MNID_USESEED)), get_item_pos(MNID_STATUS) + 1);

  get_item_by_id(MNID_STATUS).change_text(text);
}

void
SeedFinderMenu::add_randomization()
{
  s_seed_finder->add_randomization(new SeedFinder::Randomization(0, 1, SeedFinder::Randomization::RANDTYPE_INT));
  import_randomization(s_seed_finder->m_randomizations.size() - 1);
}

void
SeedFinderMenu::import_randomization(const int index)
{
  int items_pos = get_item_pos(MNID_ADDRANDOMIZATION);
  auto* rand = s_seed_finder->m_randomizations[index].get();

  add_item(std::unique_ptr<MenuItem>(new ItemNumField(_("Range start"), &rand->m_range_start)), items_pos)
    ->id = -10 - (index + 1) * 2; // Set an ID to find the randomization on the menu with.
  add_item(std::unique_ptr<MenuItem>(new ItemNumField(_("Range end"), &rand->m_range_end)), items_pos + 1);
  add_item(std::unique_ptr<MenuItem>(new ItemStringSelect(_("Type"), SeedFinder::Randomization::s_rand_types,
                                     reinterpret_cast<int*>(&rand->m_type))), items_pos + 2);

  const int desired_value_id = -9 - (index + 1) * 2;
  add_item(std::unique_ptr<MenuItem>(new ItemAction(_("Set desired value..."),
                                     desired_value_id)), items_pos + 3);
  if (!rand->m_desired_values.empty()) // Check if a desired value isn't already set.
    set_desired_value(desired_value_id, &items_pos);

  add_item(std::unique_ptr<MenuItem>(new ItemHorizontalLine()), items_pos + 4);
}

void
SeedFinderMenu::import_randomizations()
{
  if (s_seed_finder->m_randomizations.size() <= 0)
  {
    add_randomization();
  }
  else
  {
    const int rands_size = static_cast<int>(s_seed_finder->m_randomizations.size());
    for (int i = (rands_size < 10 ? 0 : rands_size - 10); i < rands_size; i++)
    {
      import_randomization(i);
    }
  }
}

void
SeedFinderMenu::remove_last_randomization(bool force)
{
  if (!force && s_seed_finder->m_randomizations.size() <= 1)
  {
    log_warning << "Cannot remove the only available randomization." << std::endl;
    return;
  }

  // Negative item ID, even and smaller than -10, indicates the beginning of a randomization in the menu.
  delete_until_hl_from(-10 - s_seed_finder->m_randomizations.size() * 2);

  if (!force) set_active_item(MNID_REMOVELASTRANDOMIZATION);

  s_seed_finder->m_randomizations.pop_back();
}

void
SeedFinderMenu::import_logged_randomizations()
{
  std::vector<std::string> rand_strings;
  for (auto& rand : s_seed_finder->s_randomization_log)
    rand_strings.push_back(rand->to_string());

  auto callback = [this](int selected)
    {
      s_seed_finder->import_logged_randomizations(selected);
      refresh();
    };
  auto* select_menu = new MenuSelectItem(rand_strings, std::move(callback));

  select_menu->change_label(_("Choose Last Randomization To Import"));
  MenuManager::instance().push_menu(std::unique_ptr<Menu>(select_menu));
}

void
SeedFinderMenu::clear_logged_randomizations()
{
  s_seed_finder->s_randomization_log.clear();

  delete_until_hl_from(MNID_IMPORTLOGGED);
}

void
SeedFinderMenu::set_desired_value(const int item_id, int* pos_)
{
  const int randomization_id = (item_id + 9) / -2 - 1;
  const int item_pos = get_item_pos(item_id);
  auto* rand = s_seed_finder->m_randomizations[randomization_id].get();

  if (rand->m_desired_values.empty())
    rand->m_desired_values.push_back(1);

  delete_item(item_pos);
  add_item(std::unique_ptr<MenuItem>(new ItemNumField(_("Desired value"), &(rand->m_desired_values[0]))), item_pos)->id = item_id;
  add_item(std::unique_ptr<MenuItem>(new ItemNumField(_("Precision"), &rand->m_precision)), item_pos + 1);
  if (pos_) (*pos_) += 1; // Shift item positions.
  set_active_item(item_id);
}

void
SeedFinderMenu::use_seed()
{
  g_config->random_seed = gameRandom.srand(s_seed_finder->get_seed());
  assert(g_config->random_seed == s_seed_finder->get_seed());

  g_config->random_seed_history.push_back(g_config->random_seed);
  while (g_config->random_seed_history.size() > 100)
    g_config->random_seed_history.erase(g_config->random_seed_history.begin());

  MenuManager::instance().pop_menu();
}

void
SeedFinderMenu::menu_action(MenuItem* item)
{
  // Negative ID of an ItemAction, odd and smaller than -10, indicates setting a desired value for a certain randomization.
  if (item->id < -10 && item->id % 2 != 0 && dynamic_cast<ItemAction*>(item))
  {
    set_desired_value(item->id);
    return;
  }

  switch (item->id)
  {
    case MNID_IMPORTLOGGED:
      import_logged_randomizations();
      break;
    case MNID_CLEARLOGGED:
      clear_logged_randomizations();
      break;
    case MNID_ADDRANDOMIZATION:
      add_randomization();
      break;
    case MNID_REMOVELASTRANDOMIZATION:
      remove_last_randomization();
      break;
    case MNID_FINDSEED:
      s_seed_finder->find_seed();
      break;
    case MNID_USESEED:
      use_seed();
      break;
    case MNID_IMPORTRANDOMIZATIONS:
      if (m_import_file.empty())
      {
        auto dialog = std::unique_ptr<Dialog>(new Dialog(false));
        dialog->set_text(_("No import file selected."));
        dialog->add_button(_("OK"), []() { MenuManager::instance().set_dialog({}); });
        MenuManager::instance().set_dialog(std::move(dialog));
        return;
      }
      s_seed_finder->read(m_import_file);
      m_import_file.clear();
      refresh();
      break;
    case MNID_SAVERANDOMIZATIONS:
      s_seed_finder->save();

      auto dialog = std::unique_ptr<Dialog>(new Dialog(false));
      dialog->set_text(_("Saved under \"/seedfinder\"."));
      dialog->add_button(_("OK"), []() { MenuManager::instance().set_dialog({}); });
      MenuManager::instance().set_dialog(std::move(dialog));
      break;
  }
}

/* EOF */
