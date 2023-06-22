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

#ifndef HEADER_SUPERTUX_SUPERTUX_MENU_SEED_FINDER_MENU_HPP
#define HEADER_SUPERTUX_SUPERTUX_MENU_SEED_FINDER_MENU_HPP

#include "gui/menu.hpp"

#include "supertux/seed_finder.hpp"

class SeedFinderMenu : public Menu
{
private:
  static std::unique_ptr<SeedFinder> s_seed_finder;

public:
  static SeedFinder* get_seed_finder() { return s_seed_finder.get(); }

public:
  SeedFinderMenu();

  void refresh() override;

  void update_status(const SeedFinder::Status& status = s_seed_finder->get_status());

  void add_randomization();
  void import_randomization(const int index);
  void import_randomizations();
  void remove_last_randomization(bool force = false);

  void import_logged_randomizations();
  void clear_logged_randomizations();

  void set_desired_value(const int item_id, int* pos_ = nullptr);
  void use_seed();

  void menu_action(MenuItem* item) override;

private:
  enum SeedFinderMenuIDs
  {
    MNID_IMPORTLOGGED,
    MNID_CLEARLOGGED,
    MNID_ADDRANDOMIZATION,
    MNID_REMOVELASTRANDOMIZATION,
    MNID_FINDSEED,
    MNID_USESEED,
    MNID_STATUS
  };

private:
  SeedFinderMenu(const SeedFinderMenu&);
  SeedFinderMenu& operator=(const SeedFinderMenu&);
};

#endif

/* EOF */
