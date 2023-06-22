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

#ifndef HEADER_SUPERTUX_SUPERTUX_SEED_FINDER_HPP
#define HEADER_SUPERTUX_SUPERTUX_SEED_FINDER_HPP

#include <vector>
#include <memory>
#include <boost/optional.hpp>

#include "gui/dialog.hpp"

#include "math/random_generator.hpp"
#include "supertux/timer.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"

class Randomization
{
  friend class SeedFinderMenu;

public:
  enum RandType
  {
    RANDTYPE_INT,
    RANDTYPE_FLOAT
  };
  enum RandValue
  {
    RANDVALUE_EQUAL,
    RANDVALUE_LESSTHAN,
    RANDVALUE_MORETHAN
  };
  static const std::vector<std::string> s_rand_types;

private:
  float m_range_start;
  float m_range_end;
  int m_type;
  int m_value_type;
  boost::optional<float> m_desired_value;
  float m_precision;

  float m_value;

public:
  Randomization(float range_start, float range_end, RandType type,
                boost::optional<float> desired_value = boost::none,
                float precision = 0.01f);
  Randomization(ReaderMapping& mapping);

  void rand(RandomGenerator& rng);
  void reset();

  void save(Writer& writer);

  std::string to_string() const;

  float get_value() const { return m_value; }
  bool has_match() const;

private:
  Randomization(const Randomization&);
  Randomization& operator=(const Randomization&);
};


class SeedFinder
{
  friend class SeedFinderMenu;

public:
  typedef std::vector<std::unique_ptr<Randomization>> RandomizationLog; 

private:
  static RandomizationLog s_randomization_log;

public:
  enum Status
  {
    STATUS_OK,
    STATUS_INPROGRESS,
    STATUS_FOUND,
    STATUS_TIMEDOUT
  };

  static void log_randomization(Randomization* rand) { s_randomization_log.push_back(std::unique_ptr<Randomization>(rand)); }

private:
  RandomizationLog m_randomizations;
  int m_init_seed;
  float m_search_time;
  RandomGenerator m_rng;

  bool m_in_progress;
  Timer m_search_timer;
  int m_seed;

public:
  SeedFinder(int init_seed = 0);

  void add_randomization(Randomization* rand) { m_randomizations.push_back(std::unique_ptr<Randomization>(rand)); }
  void import_logged_randomizations(const int& selected);

  void read();
  void save();

  std::string values_to_string() const;

  void find_seed();
  void update();

  int get_seed() const { return m_seed; }
  Status get_status() const;

private:
  SeedFinder(const SeedFinder&);
  SeedFinder& operator=(const SeedFinder&);
};


class SeedFinderDialog : public Dialog
{
private:
  SeedFinder* m_seed_finder;
  SeedFinder::Status m_last_finder_status;

public:
  SeedFinderDialog(SeedFinder* seed_finder);

  void update() override;

private:
  void update_menu_status();

private:
  SeedFinderDialog(const SeedFinderDialog&);
  SeedFinderDialog& operator=(const SeedFinderDialog&);
};

#endif

/* EOF */
