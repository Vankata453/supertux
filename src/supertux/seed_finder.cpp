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

#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <time.h>

#include <physfs.h>

#include "gui/menu_manager.hpp"
#include "supertux/menu/seed_finder_menu.hpp"
#include "util/gettext.hpp"
#include "util/log.hpp"
#include "util/reader_document.hpp"

const std::vector<std::string> SeedFinder::Randomization::s_rand_types = { _("Integer"), _("Float") };

SeedFinder::Randomization::Randomization(float range_start, float range_end, RandType type, float time,
                                         boost::optional<float> desired_value, float precision) :
  m_range_start(range_start),
  m_range_end(range_end),
  m_time(time),
  m_type(type),
  m_value_type(RANDVALUE_EQUAL),
  m_desired_values(),
  m_precision(precision),
  m_value(),
  m_temp_time(),
  m_pilot_timeframe(false),
  m_pilot_timeframe_time(-1.f)
{
  reset();

  if (desired_value)
    m_desired_values.push_back(desired_value.get());
}

SeedFinder::Randomization::Randomization(ReaderMapping& mapping) :
  m_range_start(),
  m_range_end(),
  m_time(-1.f),
  m_type(),
  m_value_type(),
  m_desired_values(),
  m_precision(0.01f),
  m_value(),
  m_temp_time(),
  m_pilot_timeframe(false),
  m_pilot_timeframe_time(-1.f)
{
  reset();

  mapping.get("range-start", m_range_start);
  mapping.get("range-end", m_range_end);
  mapping.get("time", m_time);
  mapping.get("type", m_type);
  mapping.get("value-type", m_value_type);

  if (!mapping.get("desired-values", m_desired_values))
    mapping.get("desired-value", m_desired_values); // Retro-compatibility

  mapping.get("precision", m_precision);

  mapping.get("pilot-timeframe", m_pilot_timeframe);
  mapping.get("pilot-timeframe-time", m_pilot_timeframe_time);
}

void
SeedFinder::Randomization::save(Writer& writer)
{
  writer.write("range-start", m_range_start);
  writer.write("range-end", m_range_end);

  if (m_time >= 0.f)
    writer.write("time", m_time);

  writer.write("type", m_type);

  if (!m_desired_values.empty())
  {
    writer.write("value-type", m_value_type);
    writer.write("desired-values", m_desired_values);
    writer.write("precision", m_precision);
  }

  if (m_pilot_timeframe)
  {
    writer.write("pilot-timeframe", m_pilot_timeframe);
    if (m_pilot_timeframe_time > 0.f)
      writer.write("pilot-timeframe-time", m_pilot_timeframe_time);
  }
}

void
SeedFinder::Randomization::rand(RandomGenerator& rng)
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
SeedFinder::Randomization::reset()
{
  m_value.reset();
}

std::string
SeedFinder::Randomization::to_string() const
{
  const std::string start_str = std::to_string(m_range_start);
  const std::string end_str = std::to_string(m_range_end);

  return (m_type == RANDTYPE_INT ? _("Integer") : _("Float"))
         + " " + start_str.substr(0, start_str.find('.') + 3)
          + "-" + end_str.substr(0, end_str.find('.') + 3);
}

bool
SeedFinder::Randomization::has_match() const
{
  if (m_desired_values.empty())
    return true;

  for (float desired_value : m_desired_values)
  {
    if (has_value_match(desired_value))
      return true;
  }
  return false;
}

bool
SeedFinder::Randomization::has_value_match(float desired_value) const
{
  if (m_value == boost::none)
    return false;

  switch (m_value_type)
  {
    case RANDVALUE_EQUAL:
      return std::fabs(*m_value - desired_value) < m_precision;
    case RANDVALUE_LESSTHAN:
      return *m_value <= desired_value;
    case RANDVALUE_MORETHAN:
      return *m_value >= desired_value;
  }
  return false;
}


SeedFinder::RandomizationLog SeedFinder::s_randomization_log = {};

SeedFinder::SeedFinder(int init_seed) :
  m_randomizations(),
  m_init_seed(init_seed),
  m_search_time(5.0f),
  m_search_threads_count(2),
  m_in_progress(false),
  m_search_timer(),
  m_search_threads(),
  m_seed(0)
{
  // Add initial randomization
  m_randomizations.push_back(std::unique_ptr<Randomization>(new Randomization(0, 1, Randomization::RANDTYPE_INT)));
}

void
SeedFinder::import_logged_randomizations(const int& selected)
{
  if (s_randomization_log.size() <= 0) return;

  for (int i = 0; i < static_cast<int>(s_randomization_log.size()); i++)
  {
    m_randomizations.push_back(std::move(s_randomization_log[i]));
    if (i == selected) break;
  }

  s_randomization_log.clear();
}

void
SeedFinder::read(const std::string& filename)
{
  m_randomizations.clear();

  try
  {
    auto doc = ReaderDocument::parse(filename);
    auto root = doc.get_root();
    if (root.get_name() != "supertux-randomizations")
      throw std::runtime_error("File is not a 'supertux-randomizations' file.");

    auto iter = root.get_mapping().get_iter();
    while (iter.next())
    {
      if (iter.get_key() != "randomization")
      {
        log_warning << "Unknown key '" << iter.get_key() << "' in seed finder randomizations data." << std::endl;
        continue;
      }

      auto mapping = iter.as_mapping();
      m_randomizations.push_back(std::unique_ptr<Randomization>(new Randomization(mapping)));
    }
  }
  catch (const std::exception& err)
  {
    log_warning << "Error parsing seed finder randomizations from file '" << filename
                << "': " << err.what() << std::endl;
  }
}

void
SeedFinder::save()
{
  // Make sure "rng" directory exists in the root
  if (!PHYSFS_exists("seedfinder") && !PHYSFS_mkdir("seedfinder"))
  {
    log_warning << "Couldn't create directory for seed finder randomizations 'seedfinder': " << PHYSFS_getLastError();
    return;
  }

  Writer writer("seedfinder/rands_" + std::to_string(time(nullptr)) + ".stsf");
  writer.start_list("supertux-randomizations");

  for (const auto& randomization : m_randomizations)
  {
    writer.start_list("randomization");
    randomization->save(writer);
    writer.end_list("randomization");
  }

  writer.end_list("supertux-randomizations");
}

std::string
SeedFinder::values_to_string(const std::vector<Randomization*>& rands)
{
  if (rands.empty())
    return "";

  std::stringstream stream;

  for (auto& rand : rands)
  {
    if (!rand->has_value())
      break;

    stream << rand->get_value() << " (" << rand->get_temp_time() << ")" << ", ";
  }

  const std::string result = stream.str();
  return result.substr(0, result.size() - 2); // Remove the last space and comma characters.
}

void
SeedFinder::find_seed()
{
  if (m_in_progress) return;

  if (m_search_threads_count < 1)
  {
    log_warning << "Search threads must be no lower than 1." << std::endl;
    return;
  }

  float latest_time = 0.f;
  for (auto& rand : m_randomizations)
  {
    rand->reset();

    // Ensure every randomization has time set, preserving current order.
    if (rand->get_time() < 0.f)
    {
      rand->set_temp_time(latest_time);
    }
    else
    {
      rand->set_temp_time(rand->get_time());
      latest_time = rand->get_time();
    }
  }

  // Sort all randomizations by their assigned temporary randomization time.
  std::stable_sort(m_randomizations.begin(), m_randomizations.end(),
    [](const std::unique_ptr<Randomization>& lhs, const std::unique_ptr<Randomization>& rhs) {
      return lhs->get_temp_time() < rhs->get_temp_time();
    });

  m_in_progress = true;
  m_search_timer.start(m_search_time - 1.0f); // The search time variable stores real time seconds.
  MenuManager::instance().set_dialog(std::unique_ptr<Dialog>(new SeedFinderDialog(this)));

  // Run threads to find the seed
  for (int i = 0; i < m_search_threads_count; i++)
    m_search_threads.emplace_back(&SeedFinder::finder, this);
}

void
SeedFinder::finder()
{
  RandomGenerator rng;
  rng.srand(m_init_seed);

  // Copy all randomizations over
  std::vector<Randomization> randomizations_local;
  for (auto& rand : m_randomizations)
    randomizations_local.emplace_back(*rand);

  while (m_in_progress)
  {
    const int seed = rng.srand(rng.rand());

    std::vector<Randomization*> randomizations;
    std::vector<Randomization*> randomizations_cleanup;

    for (Randomization& rand : randomizations_local)
    {
      rand.reset();
      randomizations.push_back(&rand);
    }

    std::vector<int> excluded_timeframes;

    bool has_match = true;
    for (size_t i = 0; i < randomizations.size(); i++)
    {
      Randomization* rand = randomizations[i];
      rand->rand(rng);
      if (!rand->has_match())
      {
        has_match = false;
        break;
      }

      if (rand->has_pilot_timeframe()) // Timeframe for pilot puff timer
      {
        // If time until update is more than the maximum allowed time, do not add pilot update cycle.
        if (rand->has_pilot_timeframe_time() && rand->get_value() > rand->get_pilot_timeframe_time())
        {
          excluded_timeframes.push_back(static_cast<int>(i));
          continue;
        }

        int update_time = rand->get_time() + rand->get_value();

        auto new_rand = new Randomization(-10, 10, Randomization::RANDTYPE_FLOAT);
        new_rand->set_temp_time(update_time);
        randomizations.push_back(new_rand);
        randomizations_cleanup.push_back(new_rand);

        auto new_rand2 = new Randomization(4.0f, 8.0f, Randomization::RANDTYPE_FLOAT);
        new_rand2->set_temp_time(update_time);
        randomizations.push_back(new_rand2);
        randomizations_cleanup.push_back(new_rand2);

        auto new_rand3 = new Randomization(0.95, 1.05, Randomization::RANDTYPE_FLOAT);
        new_rand3->set_temp_time(update_time);
        randomizations.push_back(new_rand3);
        randomizations_cleanup.push_back(new_rand3);

        // Re-sort randomizations, including the newly added ones.
        std::stable_sort(randomizations.begin(), randomizations.end(),
          [](const Randomization* lhs, const Randomization* rhs) {
            return lhs->get_temp_time() < rhs->get_temp_time();
          });
      }
    }

    {
      std::stringstream out;
      out << "SEED FINDER: " << seed << " -> " << values_to_string(randomizations) << std::endl;
      std::cout << out.str();
    }

    // Cleanup
    for (auto& rand : randomizations_cleanup)
      delete rand;

    if (has_match)
    {
      log_warning << "Found seed: " << seed << std::endl;
      if (!excluded_timeframes.empty())
      {
        std::stringstream out;
        out << "Excluded timeframes: ";
        for (const int& rand_id : excluded_timeframes)
          out << rand_id << " ";

        log_warning << out.str() << std::endl;
      }
      m_seed = seed;
      m_search_timer.stop();
      m_in_progress = false;
    }
  }
}

void
SeedFinder::update()
{
  if (m_in_progress && m_search_timer.check())
  {
    log_warning << "Seed search timed out: Search took longer than " << m_search_time << " seconds." << std::endl;
    m_seed = -1; // Mark seed as not found due to time out.
    m_in_progress = false;
  }
  if (!m_in_progress && !m_search_threads.empty())
  {
    for (std::thread& thread : m_search_threads)
      thread.detach();
    m_search_threads.clear();
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
