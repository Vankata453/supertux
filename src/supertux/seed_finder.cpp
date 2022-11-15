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

#include "supertux/seed_finder.hpp"

#include <limits>
#include <sstream>

#include "gui/menu_manager.hpp"
#include "supertux/menu/seed_finder_menu.hpp"
#include "util/gettext.hpp"
#include "util/log.hpp"

const std::vector<std::string> Randomization::s_rand_types = { _("Integer"), _("Float") };

Randomization::Randomization(float range_start, float range_end, RandType type,
                             boost::optional<float> desired_value, float precision) :
  m_range_start(range_start),
  m_range_end(range_end),
  m_type(type),
  m_desired_value(desired_value),
  m_precision(precision),
  m_value()
{
  reset();
}

void
Randomization::rand(RandomGenerator& rng)
{
  switch (m_type)
  {
    case RANDTYPE_INT:
      m_value = rng.rand(m_range_start, m_range_end);
      break;
    case RANDTYPE_FLOAT:
      m_value = rng.randf(m_range_start, m_range_end);
      break;
  }
}

void
Randomization::reset()
{
  m_value = std::numeric_limits<int>::min();
}

std::string
Randomization::to_string() const
{
  const std::string start_str = std::to_string(m_range_start);
  const std::string end_str = std::to_string(m_range_end);

  return (m_type == RANDTYPE_INT ? _("Integer") : _("Float"))
         + " " + start_str.substr(0, start_str.find('.') + 3)
          + "-" + end_str.substr(0, end_str.find('.') + 3);
}

bool
Randomization::has_match() const
{
  return m_desired_value == boost::none || std::fabs(m_value - m_desired_value.get()) < m_precision;
}


SeedFinder::RandomizationLog SeedFinder::s_randomization_log = {};

SeedFinder::SeedFinder(int init_seed) :
  m_randomizations(),
  m_init_seed(init_seed),
  m_search_time(5.0f),
  m_rng(),
  m_in_progress(false),
  m_search_timer(),
  m_seed(0)
{
  // Add initial randomization
  m_randomizations.push_back(std::unique_ptr<Randomization>(new Randomization(0, 1, Randomization::RANDTYPE_INT)));
}

void
SeedFinder::import_logged_randomizations(const int& selected)
{
  if (s_randomization_log.size() <= 0) return;

  m_randomizations.clear();

  for (int i = 0; i < static_cast<int>(s_randomization_log.size()); i++)
  {
    m_randomizations.push_back(std::move(s_randomization_log[i]));
    if (i == selected) break;
  }

  s_randomization_log.clear();
}

bool
SeedFinder::all_match() const
{
  for (auto& rand : m_randomizations)
    if (!rand->has_match()) return false;

  return true;
}

std::string
SeedFinder::values_to_string() const
{
  std::stringstream stream;

  for (auto& rand : m_randomizations)
    stream << rand->get_value() << ' ';

  std::string result = stream.str();
  result.pop_back(); // Remove the last space character.
  return result;
}

void
SeedFinder::find_seed()
{
  if (m_in_progress) return;

  m_rng.srand(m_init_seed);
  for (auto& rand : m_randomizations)
    rand->reset();

  m_in_progress = true;
  m_search_timer.start(m_search_time - 1.0f); // The search time variable stores real time seconds.
  MenuManager::instance().set_dialog(std::unique_ptr<Dialog>(new SeedFinderDialog(this)));
}

void
SeedFinder::update()
{
  if (!m_in_progress) return;

  m_seed = m_rng.srand(m_rng.rand());

  for (auto& rand : m_randomizations)
    rand->rand(m_rng);

  log_warning << "SEED FINDER: " << m_seed << " -> " << values_to_string() << std::endl;

  if (all_match())
  {
    log_warning << "Found seed: " << m_seed << std::endl;
    m_search_timer.stop();
    m_in_progress = false;
  }
  if (m_search_timer.check())
  {
    log_warning << "Seed search timed out: Search took longer than " << m_search_time << " seconds." << std::endl;
    m_seed = -1; // Mark seed as not found due to time out.
    m_in_progress = false;
  }
}

SeedFinder::Status
SeedFinder::get_status() const
{
  if (m_in_progress)
    return STATUS_INPROGRESS;
  else if (m_seed > 0)
    return STATUS_FOUND;
  else if (m_seed == -1)
    return STATUS_TIMEDOUT;

  return STATUS_OK;
}


SeedFinderDialog::SeedFinderDialog(SeedFinder* seed_finder) :
  Dialog(true),
  m_seed_finder(seed_finder),
  m_last_finder_status(seed_finder->get_status())
{
  assert(m_last_finder_status == SeedFinder::STATUS_INPROGRESS);

  update_menu_status();
  set_text(_("Searching for seed..."));
}

void
SeedFinderDialog::update()
{
  m_seed_finder->update();
  const auto status = m_seed_finder->get_status();

  if (status != m_last_finder_status)
  {
    // Update the seed finder status on the menu.
    m_last_finder_status = status;
    update_menu_status();

    // Check if the seed finder isn't in progress status anymore.
    if (status != SeedFinder::STATUS_INPROGRESS)
      MenuManager::instance().set_dialog({}); // Close the dialog.
  }
}

void
SeedFinderDialog::update_menu_status()
{
  if (!MenuManager::instance().is_active()) return;

  auto* menu = dynamic_cast<SeedFinderMenu*>(MenuManager::instance().current_menu());
  if (menu) menu->update_status(m_last_finder_status);
}

/* EOF */
