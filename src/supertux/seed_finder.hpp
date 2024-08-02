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
#include <thread>

#include <boost/optional.hpp>

#include "gui/dialog.hpp"

#include "math/random_generator.hpp"
#include "supertux/timer.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"

class SeedFinder final
{
  friend class SeedFinderMenu;

public:
  class Randomization final
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
      RANDVALUE_MORETHAN,
      RANDVALUE_BETWEEN
    };
    static const std::vector<std::string> s_rand_types;

  private:
    float m_range_start;
    float m_range_end;
    float m_time;

    int m_type;
    int m_value_type;
    std::vector<float> m_desired_values;
    float m_precision;

    boost::optional<float> m_value;
    float m_temp_time;

    bool m_pilot_timeframe;
    float m_pilot_timeframe_time;

  public:
    Randomization(float range_start, float range_end, RandType type, float time = -1.f,
                  boost::optional<float> desired_value = boost::none,
                  float precision = 0.01f);
    Randomization(ReaderMapping& mapping);

    void rand(RandomGenerator& rng);
    void reset();

    void save(Writer& writer);

    std::string to_string() const;

    float get_time() const { return m_time; }
    float get_value() const { return *m_value; }
    bool has_value() const { return m_value != boost::none; }
    bool has_match() const;

    bool has_pilot_timeframe() const { return m_pilot_timeframe; }
    bool has_pilot_timeframe_time() const { return m_pilot_timeframe_time > 0.f; }
    float get_pilot_timeframe_time() const { return m_pilot_timeframe_time; }

    float get_temp_time() const { return m_temp_time; }
    void set_temp_time(const float& temp_time) { m_temp_time = temp_time; }

  private:
    bool has_value_match(float desired_value) const;
  };

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
  int m_search_threads_count;

  bool m_in_progress;
  Timer m_search_timer;
  std::vector<std::thread> m_search_threads;
  int m_seed;

public:
  SeedFinder(int init_seed = 0);

  void add_randomization(Randomization* rand) { m_randomizations.push_back(std::unique_ptr<Randomization>(rand)); }
  void import_logged_randomizations(const int& selected);

  void read(const std::string& filename);
  void save();

  void find_seed();
  void update();

  int get_seed() const { return m_seed; }
  Status get_status() const;

private:
  static std::string values_to_string(const std::vector<Randomization*>& rands);

  void finder();

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
