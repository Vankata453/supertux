//  $Id$
//
//  SuperTux -  A Jump'n Run
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
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

#include <assert.h>
#include <algorithm>
#include <iostream>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#ifndef NOOPENGL
#include <SDL2/SDL_opengl.h>
#endif
#include <physfs.h>

#ifndef WIN32
#include <linux/limits.h>
#include <libgen.h>
#endif
#include <ctype.h>

#include "defines.h"
#include "globals.h"
#include "setup.h"
#include "screen.h"
#include "texture.h"
#include "menu.h"
#include "gameloop.h"
#include "configfile.h"
#include "scene.h"
#include "worldmap.h"
#include "resources.h"
#include "intro.h"
#include "music_manager.h"
#include "downloader.h"
#include "title.h"

#include "player.h"

#ifdef WIN32
#define mkdir(dir, mode)    mkdir(dir)
// on win32 we typically don't want LFS paths
#undef DATA_PREFIX
#define DATA_PREFIX "./data/"
#endif

/* Local function prototypes: */

void seticon(void);
void usage(char * prog, int ret);

static PHYSFS_EnumerateCallbackResult physfs_enumerate_dirs(void* data, const char* origdir, const char* fname)
{
  const std::string full_path = std::string(origdir) + "/" + std::string(fname);

  PHYSFS_Stat stat;
  PHYSFS_stat(full_path.c_str(), &stat);
  if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY)
  {
    const auto* callback = static_cast<std::function<void(const char*)>*>(data);
    callback->operator()(fname);
  }
  return PHYSFS_ENUM_OK;
}

static PHYSFS_EnumerateCallbackResult physfs_enumerate_files(void* data, const char* origdir, const char* fname)
{
  const std::string full_path = std::string(origdir) + "/" + std::string(fname);

  PHYSFS_Stat stat;
  PHYSFS_stat(full_path.c_str(), &stat);
  if (stat.filetype == PHYSFS_FILETYPE_REGULAR)
  {
    const auto* callback = static_cast<std::function<void(const char*)>*>(data);
    callback->operator()(fname);
  }
  return PHYSFS_ENUM_OK;
}

/* Get all names of sub-directories in a certain directory. */
/* Returns the number of sub-directories found. */
/* Note: The user has to free the allocated space. */
string_list_type dsubdirs(const char *path, const char* expected_file)
{
  string_list_type sdirs;
  string_list_init(&sdirs);

  std::function<void(const char*)> callback =
    [&sdirs, path, expected_file](const char* fname)
    {
      if (expected_file &&
          PHYSFS_exists((std::string(path) + "/" +
            std::string(fname) + "/" + std::string(expected_file)).c_str()))
      {
        string_list_add_item(&sdirs, fname);
      }
    };
  PHYSFS_enumerate(path, &physfs_enumerate_dirs, &callback);

  return sdirs;
}

string_list_type dfiles(const char *path, const char* glob, const char* exception_str)
{
  string_list_type sfiles;
  string_list_init(&sfiles);

  std::function<void(const char*)> callback =
    [&sfiles, glob, exception_str](const char* fname)
    {
      if ((!exception_str || !strstr(fname, exception_str)) &&
          (!glob || strstr(fname, glob)))
      {
        string_list_add_item(&sfiles, fname);
      }
    };
  PHYSFS_enumerate(path, &physfs_enumerate_files, &callback);

  return sfiles;
}

void free_strings(char **strings, int num)
{
  int i;
  for(i=0; i < num; ++i)
    free(strings[i]);
}

/* --- SETUP --- */
/* Set SuperTux configuration and save directories */
void st_directory_setup(int argc, char** const argv)
{
  char working_dir[PATH_MAX];
  if (!getcwd(working_dir, PATH_MAX))
    throw std::runtime_error("Couldn't get current working directory!");

  // Get custom datadir/userdir from command line arguments.
  // Must be parsed here early on, because config cannot be
  // loaded in parseargs() before PhysFS initialization.
  for (int i = 1; i + 1 < argc; i++)
  {
    if (strcmp(argv[i], "--datadir") == 0 ||
        strcmp(argv[i], "-d") == 0)
    {
      real_datadir = std::string(working_dir) + "/" + std::string(argv[i + 1]);
    }
    else if (strcmp(argv[i], "--userdir") == 0 ||
             strcmp(argv[i], "-u") == 0)
    {
      real_userdir = std::string(working_dir) + "/" + std::string(argv[i + 1]);
    }
  }

  if (!PHYSFS_init(argv[0]))
    throw std::runtime_error("Couldn't initialize PhysFS: " + std::string(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));

  if (real_datadir.empty())
  {
#ifndef WIN32
    // Detect datadir
    char exe_file[PATH_MAX];
    if (readlink("/proc/self/exe", exe_file, PATH_MAX) < 0)
      {
        puts("Couldn't read /proc/self/exe, using default path: " DATA_PREFIX);
        real_datadir = DATA_PREFIX;
      }
    else
      {
        std::string exedir = std::string(dirname(exe_file)) + "/";
        
        real_datadir = exedir + "../data"; // SuperTux run from source dir
        if (access(real_datadir.c_str(), F_OK) != 0)
          {
            real_datadir = exedir + "../share/supertux"; // SuperTux run from PATH
            if (access(real_datadir.c_str(), F_OK) != 0) 
              { // If all fails, fall back to compiled path
                real_datadir = DATA_PREFIX; 
              }
          }
      }
#else
    real_datadir = DATA_PREFIX;
#endif
  }
  printf("Datadir: %s\n", real_datadir.c_str());
  if (!PHYSFS_mount(real_datadir.c_str(), nullptr, 1))
    throw std::runtime_error("Couldn't add '" + real_datadir + "' to PhysFS searchpath: " + std::string(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));

  if (real_userdir.empty())
    real_userdir = PHYSFS_getPrefDir("SuperTux", "supertux_m1");
  else if (access(real_userdir.c_str(), F_OK) != 0)
    mkdir(real_userdir.c_str(), 0755);
  printf("Userdir: %s\n", real_userdir.c_str());
  if (!PHYSFS_setWriteDir(real_userdir.c_str()))
    throw std::runtime_error("Failed to set userdir directory: " + std::string(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
  if (!PHYSFS_mount(real_userdir.c_str(), nullptr, 0))
    throw std::runtime_error("Couldn't add '" + real_userdir + "' to PhysFS searchpath: " + std::string(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
}

// Generate valid add-on ID: Replace spaces with underscores, skip other invalid lisp symbol characters
std::string generate_addon_id(const std::string& archive)
{
  std::string addon_id;
  const std::string raw_addon_id = archive.substr(0, archive.size() - 4);
  for (char c : raw_addon_id)
  {
    if (c == ' ')
    {
      addon_id += '_';
    }
    else if (isalnum(c) ||
        c == '_' || c == '-' || c == '!' || c == '?' ||
        c == ':' || c == '+' || c == '*' || c == '/' ||
        c == '=' || c == '<' || c == '>' || c == '$' ||
        c == '%' || c == '&' || c == '~' || c == '^' ||
        c == '.')
    {
      addon_id += c;
    }
  }
  return addon_id;
}

bool st_addons_check(bool startup = false);

void st_addons_setup()
{
  if (!PHYSFS_exists("addons"))
  {
    PHYSFS_mkdir("addons");
    return;
  }

  st_addons_check(true);
}

void st_addons_enable(bool resource_packs, bool startup)
{
  // Resource packs should only be enabled on startup
  if (resource_packs)
    assert(startup);

  for (auto& addon_entry : addons)
  {
    Addon& addon = addon_entry.second;
    if (addon.resource_pack != resource_packs ||
        // Do not actually enable add-ons which override data after game has started
        (!startup && addon.overrides_data))
      continue;

    // Mount add-on, if enabled and not yet mounted
    if (addon.mounted || !addons_enabled[addon_entry.first]) continue;
    const std::string filepath = "addons/" + addon.filename;
    const char* realdir = PHYSFS_getRealDir(filepath.c_str());
    if (!realdir)
    {
      printf("[ADD-ONS] ERROR: PHYSFS_getRealDir() failed for 'addons/%s': %s\n", addon.filename.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
      continue;
    }
    if (!PHYSFS_mount((std::string(realdir) + "/" + filepath).c_str(), nullptr, !addon.overrides_data))
    {
      printf("[ADD-ONS] ERROR: Couldn't mount add-on archive '%s': %s\n", addon.filename.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
      continue;
    }
    addon.mounted = true;
    printf("[ADD-ONS] SUCCESS: Mounted add-on archive '%s' (resource pack: %s) (prepended: %s)\n", addon.filename.c_str(),
      addon.resource_pack ? "yes" : "no", addon.overrides_data ? "yes" : "no");
  }

  // Ensure each enabled add-on has its dependencies enabled, if they are installed
  for (const auto& addon_entry : addons)
  {
    if ((resource_packs && !addon_entry.second.resource_pack) ||
        !addons_enabled[addon_entry.first])
      continue;

    for (const std::string& dep_id : addon_entry.second.dependencies)
    {
      const auto dep_it = addons.find(dep_id);
      if (dep_it == addons.end())
      {
        printf("[ADD-ONS] WARNING: Dependency '%s' of add-on '%s' is not available!\n", dep_id.c_str(), addon_entry.first.c_str());
        continue;
      }

      Addon& dep_addon = dep_it->second;
      bool& dep_addon_enabled = addons_enabled[dep_id];
      if (dep_addon_enabled && dep_addon.mounted)
        continue;
      dep_addon_enabled = true;

      if (dep_addon.resource_pack != resource_packs ||
          // Do not actually enable add-ons which override data after game has started
          (!startup && dep_addon.overrides_data))
        continue;

      const std::string filepath = "addons/" + dep_addon.filename;
      const char* realdir = PHYSFS_getRealDir(filepath.c_str());
      if (!realdir)
      {
        printf("[ADD-ONS] ERROR: PHYSFS_getRealDir() failed for 'addons/%s': %s\n", dep_addon.filename.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        continue;
      }
      if (!PHYSFS_mount((std::string(realdir) + "/" + filepath).c_str(), nullptr, !dep_addon.overrides_data))
      {
        printf("[ADD-ONS] ERROR: Couldn't mount add-on archive '%s': %s\n", dep_addon.filename.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        continue;
      }
      dep_addon.mounted = true;
      printf("[ADD-ONS] SUCCESS: Mounted add-on archive '%s' (resource pack: %s) (prepended: %s)\n", dep_addon.filename.c_str(),
        dep_addon.resource_pack ? "yes" : "no", dep_addon.overrides_data ? "yes" : "no");
    }
  }
}

// RETURNS true if any newly detected add-on archives override data.
bool st_addons_check(bool startup)
{
  std::vector<std::string> archives;
  std::function<void(const char*)> callback =
    [&archives](const char* fname)
    {
      const char* dot = strrchr(fname, '.');
      if (dot && !strcmp(dot, ".zip"))
        archives.push_back(fname);
    };
  PHYSFS_enumerate("addons", &physfs_enumerate_files, &callback);

  bool new_archive_overrides_data = false;
  for (const std::string& archive : archives)
  {
    const std::string addon_id = generate_addon_id(archive);
    if (addon_id.empty())
    {
      printf("[ADD-ONS] ERROR: Couldn't process add-on archive '%s': Name leads to an empty ID!\n", archive.c_str());
      continue;
    }
    const auto addon_it = addons.find(addon_id);
    if (addon_it != addons.end())
    {
      if (addon_it->second.filename != archive)
        printf("[ADD-ONS] ERROR: Couldn't process add-on archive '%s': Add-on with the same ID ('%s') exists!\n", archive.c_str(), addon_id.c_str());
      continue;
    }

    // Get full real path to archive
    const std::string filepath = "addons/" + archive;
    const char* realdir = PHYSFS_getRealDir(filepath.c_str());
    if (!realdir)
    {
      printf("[ADD-ONS] ERROR: PHYSFS_getRealDir() failed for 'addons/%s': %s\n", archive.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
      continue;
    }
    const std::string full_archive_path = std::string(realdir) + "/" + filepath;

    // Mount add-on temporarily just to parse "info" file
    if (!PHYSFS_mount(full_archive_path.c_str(), ("addons/" + addon_id).c_str(), 0))
    {
      printf("[ADD-ONS] ERROR: Couldn't temporarily mount add-on archive '%s': %s\n", archive.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
      continue;
    }
    lisp_object_t* root_obj = lisp_read_from_file(("addons/" + addon_id + "/info").c_str());
    PHYSFS_unmount(full_archive_path.c_str());

    // Parse add-on "info"
    if (!root_obj)
    {
      printf("[ADD-ONS] ERROR: Couldn't find 'info' file in add-on archive '%s'!\n", archive.c_str());
      continue;
    }
    if (root_obj->type == LISP_TYPE_EOF || root_obj->type == LISP_TYPE_PARSE_ERROR)
    {
      printf("[ADD-ONS] ERROR: Couldn't parse 'info' file from add-on archive '%s'!\n", archive.c_str());
      continue;
    }
    if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-addoninfo") != 0)
    {
      printf("[ADD-ONS] ERROR: 'info' file from add-on archive '%s' is not declared 'supertux-addoninfo'!\n", archive.c_str());
      continue;
    }
    LispReader reader(lisp_cdr(root_obj));
    Addon addon(archive);
    if (!reader.read_string("title", &addon.title))
      addon.title = addon_id;
    if (!reader.read_string("author", &addon.author))
      addon.author = "Unknown";
    reader.read_bool("resource-pack", &addon.resource_pack);
    if (addon.resource_pack)
      addon.overrides_data = true; // Resource packs should always override data
    else
      reader.read_bool("overrides-data", &addon.overrides_data);
    reader.read_string_vector("dependencies", &addon.dependencies);
    lisp_free(root_obj);
    printf("[ADD-ONS] SUCCESS: Successfully parsed 'info' from add-on archive '%s' (id: '%s') (title: '%s')\n", archive.c_str(),
      addon_id.c_str(), addon.title.c_str());

    if (addon.overrides_data)
      new_archive_overrides_data = true;

    // Add add-on object to map
    addons.insert({ addon_id, std::move(addon) });
    addons_enabled.insert({ addon_id, false });
  }

  // On startup, enable resource pack add-ons first, so that levelset add-ons
  // which override data can override them as well.
  if (startup)
    st_addons_enable(true, true);
  st_addons_enable(false, startup);

  return new_archive_overrides_data;
}

bool st_fetch_addon_index()
{
  if (addon_index_fetched) return true;

  std::string index_buf;
  TransferStatusPtr status = downloader->request_download_string(
      "https://raw.githubusercontent.com/supertux-community/supertux-addons-M1/refs/heads/master/index.txt",
      index_buf);

  bool success = false;
  status->then([&success](bool success_) { success = success_; });

  draw_background();
  download_dialog(status);

  if (!success) return false;
  addon_index_fetched = true;

  // Parse add-on index
  lisp_object_t* root_obj = lisp_read_from_string(index_buf.c_str());
  if (!root_obj || root_obj->type == LISP_TYPE_EOF || root_obj->type == LISP_TYPE_PARSE_ERROR)
  {
    printf("[ADD-ONS] ERROR: Couldn't parse add-on index!\n");
    return true;
  }
  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-addonindex") != 0)
  {
    printf("[ADD-ONS] ERROR: Add-on index is not declared 'supertux-addonindex'!\n");
    return true;
  }
  lisp_object_t* cur = root_obj;
  while (!lisp_nil_p((cur = lisp_cdr(cur))))
  {
    lisp_object_t* lisp_el = lisp_car(cur);
    const char* el_id = lisp_symbol(lisp_car(lisp_el));
    if (!strcmp(el_id, "addon"))
    {
      lisp_object_t* addon_el = lisp_cdr(lisp_el);
      LispReader reader(addon_el);

      IndexAddon addon;
      if (!reader.read_string("title", &addon.title))
      {
        printf("[ADD-ONS] ERROR: Couldn't parse add-on from index: No 'title' available!\n");
        continue;
      }
      if (!reader.read_string("author", &addon.author))
        addon.author = "Unknown";
      reader.read_bool("resource-pack", &addon.resource_pack);
      if (reader.read_string("url", &addon.url))
      {
        addon.custom_url = true;
      }
      else if (!reader.read_string("file", &addon.url))
      {
        printf("[ADD-ONS] ERROR: Couldn't parse add-on from index: No 'file' or 'url' available!\n");
        continue;
      }
      reader.read_string_vector("dependencies", &addon.dependencies);

      const std::size_t url_last_slash_idx = addon.url.find_last_of("/");
      const std::string archive = (url_last_slash_idx == std::string::npos ? addon.url : addon.url.substr(url_last_slash_idx + 1));
      if (archive.empty())
      {
        printf("[ADD-ONS] ERROR: Couldn't parse add-on from index: Couldn't resolve add-on archive from 'file' or 'url'!\n");
        continue;
      }
      {
        const char* dot = strrchr(archive.c_str(), '.');
        if (!dot || strcmp(dot, ".zip"))
        {
          printf("[ADD-ONS] ERROR: Couldn't parse add-on '%s' from index: Add-on archive is not of '.zip' format!\n", archive.c_str());
          continue;
        }
      }

      const std::string addon_id = generate_addon_id(archive);
      if (addon_id.empty())
      {
        printf("[ADD-ONS] ERROR: Couldn't process add-on '%s' from index: Name leads to an empty ID!\n", archive.c_str());
        continue;
      }
      const auto addon_it = addon_index.find(addon_id);
      if (addon_it != addon_index.end())
      {
        printf("[ADD-ONS] ERROR: Couldn't process add-on '%s' from index: Add-on with the same ID ('%s') exists in index!\n", archive.c_str(), addon_id.c_str());
        continue;
      }

      printf("[ADD-ONS] SUCCESS: Successfully parsed add-on '%s' from index (title: '%s')\n",
          addon_id.c_str(), addon.title.c_str());

      addon_index.insert({ addon_id, std::move(addon) });
    }
    else if (!strcmp(el_id, "base-url"))
    {
      if (!addon_index_base_url.empty())
      {
        printf("[ADD-ONS] WARNING: Another 'base-url' provided in add-on index file. Ignoring.");
        continue;
      }

      lisp_object_t* base_url_val = lisp_car(lisp_cdr(lisp_el));

      if (!lisp_string_p(base_url_val))
        st_abort("LispReader expected type string at token: ", el_id);

      addon_index_base_url = lisp_string(base_url_val);
    }
    else
    {
      printf("[ADD-ONS] WARNING: Unknown token in add-on index file: '%s'", el_id);
    }
  }
  lisp_free(root_obj);
  return true;
}

/* Create and setup menus. */
void st_menu(void)
{
  main_menu      = new Menu();
  options_menu   = new Menu();
  options_keys_menu     = new Menu();
  options_joystick_menu = new Menu();
  load_game_menu = new Menu();
  save_game_menu = new Menu();
  game_menu      = new Menu();
  highscore_menu = new Menu();
  contrib_menu   = new Menu();
  contrib_subset_menu   = new Menu();
  addons_menu    = new Menu();
  addons_download_menu  = new Menu();
  worldmap_menu  = new Menu();
  restart_info_menu     = new Menu();

  main_menu->set_pos(screen_w()/2, 335);
  main_menu->additem(MN_GOTO, "Start Game",0,load_game_menu, MNID_STARTGAME);
  main_menu->additem(MN_GOTO, "Bonus Levels",0,contrib_menu, MNID_CONTRIB);
  main_menu->additem(MN_GOTO, "Add-ons",0,addons_menu, MNID_ADDONS);
  main_menu->additem(MN_GOTO, "Options",0,options_menu, MNID_OPTIONMENU);
  main_menu->additem(MN_ACTION,"Level Editor",0,0, MNID_LEVELEDITOR);
  main_menu->additem(MN_ACTION,"Credits",0,0, MNID_CREDITS);
  main_menu->additem(MN_ACTION,"Quit",0,0, MNID_QUITMAINMENU);

  options_menu->additem(MN_LABEL,"Options",0,0);
  options_menu->additem(MN_HL,"",0,0);
#ifndef NOOPENGL
  options_menu->additem(MN_TOGGLE,"OpenGL",use_gl,0, MNID_OPENGL);
#else
  options_menu->additem(MN_DEACTIVE,"OpenGL (not supported)",use_gl, 0, MNID_OPENGL);
#endif
  options_menu->additem(MN_TOGGLE,"Fullscreen",use_fullscreen,0, MNID_FULLSCREEN);
  if(audio_device)
    {
      options_menu->additem(MN_TOGGLE,"Sound     ", use_sound,0, MNID_SOUND);
      options_menu->additem(MN_TOGGLE,"Music     ", use_music,0, MNID_MUSIC);
    }
  else
    {
      options_menu->additem(MN_DEACTIVE,"Sound     ", false,0, MNID_SOUND);
      options_menu->additem(MN_DEACTIVE,"Music     ", false,0, MNID_MUSIC);
    }
  options_menu->additem(MN_TOGGLE,"Show FPS  ",show_fps,0, MNID_SHOWFPS);
  options_menu->additem(MN_TOGGLE,"Back scrolling", back_scrolling, 0, MNID_BACKSCROLLING);
  options_menu->additem(MN_GOTO,"Keyboard Setup",0,options_keys_menu);

  //if(use_joystick)
  //  options_menu->additem(MN_GOTO,"Joystick Setup",0,options_joystick_menu);

  options_menu->additem(MN_HL,"",0,0);
  options_menu->additem(MN_BACK,"Back",0,0);
  
  options_keys_menu->additem(MN_LABEL,"Key Setup",0,0);
  options_keys_menu->additem(MN_HL,"",0,0);
  options_keys_menu->additem(MN_CONTROLFIELD,"Left move", 0,0, 0,&keymap.left);
  options_keys_menu->additem(MN_CONTROLFIELD,"Right move", 0,0, 0,&keymap.right);
  options_keys_menu->additem(MN_CONTROLFIELD,"Jump", 0,0, 0,&keymap.jump);
  options_keys_menu->additem(MN_CONTROLFIELD,"Duck", 0,0, 0,&keymap.duck);
  options_keys_menu->additem(MN_CONTROLFIELD,"Power/Run", 0,0, 0,&keymap.fire);
  options_keys_menu->additem(MN_TOGGLE, "Run by default", run_by_default, 0, MNID_RUN_BY_DEFAULT);
  options_keys_menu->additem(MN_HL,"",0,0);
  options_keys_menu->additem(MN_BACK,"Back",0,0);

  if(use_joystick)
    {
    options_joystick_menu->additem(MN_LABEL,"Joystick Setup",0,0);
    options_joystick_menu->additem(MN_HL,"",0,0);
    options_joystick_menu->additem(MN_CONTROLFIELD,"X axis", 0,0, 0,&joystick_keymap.x_axis);
    options_joystick_menu->additem(MN_CONTROLFIELD,"Y axis", 0,0, 0,&joystick_keymap.y_axis);
    options_joystick_menu->additem(MN_CONTROLFIELD,"A button", 0,0, 0,&joystick_keymap.a_button);
    options_joystick_menu->additem(MN_CONTROLFIELD,"B button", 0,0, 0,&joystick_keymap.b_button);
    options_joystick_menu->additem(MN_CONTROLFIELD,"Start", 0,0, 0,&joystick_keymap.start_button);
    options_joystick_menu->additem(MN_CONTROLFIELD,"DeadZone", 0,0, 0,&joystick_keymap.dead_zone);
    options_joystick_menu->additem(MN_HL,"",0,0);
    options_joystick_menu->additem(MN_BACK,"Back",0,0);
    }
  
  load_game_menu->additem(MN_LABEL,"Start Game",0,0);
  load_game_menu->additem(MN_HL,"",0,0);
  load_game_menu->additem(MN_DEACTIVE,"Slot 1",0,0, 1);
  load_game_menu->additem(MN_DEACTIVE,"Slot 2",0,0, 2);
  load_game_menu->additem(MN_DEACTIVE,"Slot 3",0,0, 3);
  load_game_menu->additem(MN_DEACTIVE,"Slot 4",0,0, 4);
  load_game_menu->additem(MN_DEACTIVE,"Slot 5",0,0, 5);
  load_game_menu->additem(MN_HL,"",0,0);
  load_game_menu->additem(MN_BACK,"Back",0,0);

  save_game_menu->additem(MN_LABEL,"Save Game",0,0);
  save_game_menu->additem(MN_HL,"",0,0);
  save_game_menu->additem(MN_DEACTIVE,"Slot 1",0,0, 1);
  save_game_menu->additem(MN_DEACTIVE,"Slot 2",0,0, 2);
  save_game_menu->additem(MN_DEACTIVE,"Slot 3",0,0, 3);
  save_game_menu->additem(MN_DEACTIVE,"Slot 4",0,0, 4);
  save_game_menu->additem(MN_DEACTIVE,"Slot 5",0,0, 5);
  save_game_menu->additem(MN_HL,"",0,0);
  save_game_menu->additem(MN_BACK,"Back",0,0);

  game_menu->additem(MN_LABEL,"Pause",0,0);
  game_menu->additem(MN_HL,"",0,0);
  game_menu->additem(MN_ACTION,"Continue",0,0,MNID_CONTINUE);
  game_menu->additem(MN_GOTO,"Options",0,options_menu);
  game_menu->additem(MN_HL,"",0,0);
  game_menu->additem(MN_ACTION,"Abort Level",0,0,MNID_ABORTLEVEL);

  worldmap_menu->additem(MN_LABEL,"Pause",0,0);
  worldmap_menu->additem(MN_HL,"",0,0);
  worldmap_menu->additem(MN_ACTION,"Continue",0,0,MNID_RETURNWORLDMAP);
  worldmap_menu->additem(MN_GOTO,"Options",0,options_menu);
  worldmap_menu->additem(MN_HL,"",0,0);
  worldmap_menu->additem(MN_ACTION,"Quit Game",0,0,MNID_QUITWORLDMAP);

  highscore_menu->additem(MN_TEXTFIELD,"Enter your name:",0,0);

  restart_info_menu->additem(MN_LABEL, "Restart required", 0, 0);
  restart_info_menu->additem(MN_HL, "", 0, 0);
  restart_info_menu->additem(MN_DEACTIVE, "You must restart the game", 0, 0, 0);
  restart_info_menu->additem(MN_DEACTIVE, "for the changes to take effect.", 0, 0, 0);
  restart_info_menu->additem(MN_HL, "", 0, 0);
  restart_info_menu->additem(MN_BACK, "Back", 0, 0);
}

static int addons_menu_page = 0;
static int addons_download_menu_page = 0;
constexpr int addons_per_page = 10;

// Variables related to trimming add-on title/author on menu
constexpr int addon_text_min_visible = 5;
constexpr int addon_text_no_trim_threshold = 8;
constexpr int addon_text_extra_horizontal_space = 100;

void generate_addons_menu(bool addons_check)
{
  addons_menu->clear();

  // Look for new add-on archives
  if (addons_check)
    st_addons_check();

  addons_menu->additem(MN_LABEL,"Add-ons (Page "
      + (addons.empty() ? "0" : std::to_string(addons_menu_page + 1)) + "/"
      + std::to_string(static_cast<int>(addons.size()) / addons_per_page
          + (static_cast<int>(addons.size()) % addons_per_page > 0 ? 1 : 0))
      + ")", 0, 0);
  addons_menu->additem(MN_HL, "", 0, 0);

  if (!addons.empty())
  {
    const int remaining_data_len = (screen_w() - strlen("\"\"") * white_text->w - addon_text_extra_horizontal_space) / white_text->w;

    int idx = addons_menu_page * addons_per_page;
    auto addon_it = addons.begin();
    std::advance(addon_it, idx);
    for (; addon_it != addons.end(); ++addon_it)
    {
      const Addon& addon = addon_it->second;

      // Trim add-on title if the text wouldn't fit on screen
      std::string text = "\"" + addon.title + "\"";
      if (static_cast<int>(text.size()) * white_text->w + addon_text_extra_horizontal_space > screen_w() &&
          static_cast<int>(addon.title.size()) > addon_text_no_trim_threshold)
      {
        text = "\"" + addon.title.substr(0, std::max(addon_text_min_visible, remaining_data_len - 3)) + "...\"";
      }

      addons_menu->additem(MN_TOGGLE, text, addons_enabled[addon_it->first], 0, idx++,
          nullptr, addon.resource_pack ? resource_pack_addon_icon : levelset_addon_icon);

      if (idx >= (addons_menu_page + 1) * addons_per_page)
        break;
    }

    addons_menu->additem(MN_HL, "", 0, 0);
  }

  if (addons_menu_page > 0)
    addons_menu->additem(MN_PURE_ACTION, "Previous page", 0, 0, MNID_PREV_PAGE);
  else
    addons_menu->additem(MN_DEACTIVE, "Previous page", 0, 0, MNID_PREV_PAGE);

  if (static_cast<int>(addons.size()) > (addons_menu_page + 1) * addons_per_page)
    addons_menu->additem(MN_PURE_ACTION, "Next page", 0, 0, MNID_NEXT_PAGE);
  else
    addons_menu->additem(MN_DEACTIVE, "Next page", 0, 0, MNID_NEXT_PAGE);

  addons_menu->additem(MN_HL, "", 0, 0);
  addons_menu->additem(MN_PURE_ACTION, "Download Add-ons", 0, 0, MNID_DOWNLOAD_ADDONS);

  addons_menu->additem(MN_HL, "", 0, 0);
  addons_menu->additem(MN_BACK, "Back", 0, 0);
}

void generate_addons_download_menu()
{
  addons_download_menu->clear();

  addons_download_menu->additem(MN_LABEL, "", 0, 0); // Will be set later
  addons_download_menu->additem(MN_HL, "", 0, 0);

  int addon_count = 0;
  if (std::any_of(addon_index.begin(), addon_index.end(),
        [](const auto& addon_entry)
        {
          return addons.find(addon_entry.first) == addons.end();
        }))
  {
    const int remaining_data_len = (screen_w() - strlen("\"\" by \"\"") * white_text->w - addon_text_extra_horizontal_space) / white_text->w;

    int map_idx = -1;
    int idx = -1;
    for (const auto& addon_entry : addon_index)
    {
      ++map_idx;

      // Skip add-ons which are already installed
      if (addons.find(addon_entry.first) != addons.end())
        continue;

      ++addon_count;

      if (++idx < addons_download_menu_page * addons_per_page ||
            idx >= (addons_download_menu_page + 1) * addons_per_page)
        continue;

      const IndexAddon& addon = addon_entry.second;

      // Trim add-on title and/or author if the text wouldn't fit on screen
      std::string text = "\"" + addon.title + "\" by \"" + addon.author + "\"";
      if (static_cast<int>(text.size()) * white_text->w + addon_text_extra_horizontal_space > screen_w() &&
          (static_cast<int>(addon.title.size()) > addon_text_no_trim_threshold || static_cast<int>(addon.author.size()) > addon_text_no_trim_threshold))
      {
        const std::string& title = addon.title;
        const std::string& author = addon.author;
        std::string trimmed_title = title;
        std::string trimmed_author = author;

        if (remaining_data_len >= static_cast<int>(title.size()) + addon_text_min_visible)
        {
          // Title and the minimum required for author will fit - trim author only
          if (static_cast<int>(author.size()) > addon_text_no_trim_threshold)
            trimmed_author = author.substr(0, std::max(addon_text_min_visible, remaining_data_len - static_cast<int>(title.size()) - 3)) + "...";
        }
        else
        {
          // Full title won't fit, so trim author first
          if (static_cast<int>(author.size()) > addon_text_no_trim_threshold)
            trimmed_author = author.substr(0, addon_text_min_visible) + "...";

          // Title gets all the remaining space
          if (static_cast<int>(title.size()) > addon_text_no_trim_threshold)
            trimmed_title = title.substr(0, std::max(addon_text_min_visible, remaining_data_len - static_cast<int>(trimmed_author.size()) - 3)) + "...";
        }

        text = "\"" + trimmed_title + "\" by \"" + trimmed_author + "\"";
      }

      addons_download_menu->additem(MN_PURE_ACTION, text, 0, 0, map_idx,
          nullptr, addon.resource_pack ? resource_pack_addon_icon : levelset_addon_icon);
    }

    addons_download_menu->additem(MN_HL, "", 0, 0);
  }

  addons_download_menu->get_item(0).change_text(("Download Add-ons (Page "
      + (addon_count == 0 ? "0" : std::to_string(addons_download_menu_page + 1)) + "/"
      + std::to_string(addon_count / addons_per_page
          + (addon_count % addons_per_page > 0 ? 1 : 0))
      + ")").c_str());

  if (addons_download_menu_page > 0)
    addons_download_menu->additem(MN_PURE_ACTION, "Previous page", 0, 0, MNID_PREV_PAGE);
  else
    addons_download_menu->additem(MN_DEACTIVE, "Previous page", 0, 0, MNID_PREV_PAGE);

  if (addon_count > (addons_download_menu_page + 1) * addons_per_page)
    addons_download_menu->additem(MN_PURE_ACTION, "Next page", 0, 0, MNID_NEXT_PAGE);
  else
    addons_download_menu->additem(MN_DEACTIVE, "Next page", 0, 0, MNID_NEXT_PAGE);

  addons_download_menu->additem(MN_HL, "", 0, 0);
  addons_download_menu->additem(MN_BACK, "Back", 0, 0);
}

void update_load_save_game_menu(Menu* pmenu)
{
  for(int i = 2; i < 7; ++i)
    {
      // FIXME: Insert a real savegame struct/class here instead of
      // doing string vodoo
      std::string tmp = slotinfo(i - 1);
      pmenu->item[i].kind = MN_ACTION;
      pmenu->item[i].change_text(tmp.c_str());
    }
}

void process_addons_menu()
{
  const int idx = addons_menu->check();
  if (idx < 0)
  {
    switch (idx)
    {
      case MNID_PREV_PAGE:
        assert(addons_menu_page > 0);
        --addons_menu_page;
        generate_addons_menu(false);
        addons_menu->set_active_item_id(MNID_PREV_PAGE);
        break;
      case MNID_NEXT_PAGE:
        assert(static_cast<int>(addons.size()) > (addons_menu_page + 1) * addons_per_page);
        ++addons_menu_page;
        generate_addons_menu(false);
        addons_menu->set_active_item_id(MNID_NEXT_PAGE);
        break;
      case MNID_DOWNLOAD_ADDONS:
        if (st_fetch_addon_index())
        {
          generate_addons_download_menu();
          Menu::push_current(addons_download_menu);
        }
        break;
    }
    return;
  }

  auto addon_it = addons.begin();
  std::advance(addon_it, idx);

  bool& addon_enabled = addons_enabled[addon_it->first];
  if (addon_enabled == addons_menu->isToggled(idx))
    return;
  addon_enabled = !addon_enabled;

  Addon& addon = addon_it->second;

  // Notify user a restart is required to enable/disable add-ons which override data
  if (addon.overrides_data)
  {
    if (addon.mounted != addon_enabled)
      Menu::push_current(restart_info_menu);
    return;
  }

  // Get full real path to archive
  const std::string filepath = "addons/" + addon.filename;
  const char* realdir = PHYSFS_getRealDir(filepath.c_str());
  if (!realdir)
  {
    printf("[ADD-ONS] ERROR: PHYSFS_getRealDir() failed for 'addons/%s': %s\n", addon.filename.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    return;
  }
  const std::string full_archive_path = std::string(realdir) + "/" + filepath;

  if (addon_enabled) /* ENABLE */
  {
    // Mount the add-on
    if (!PHYSFS_mount(full_archive_path.c_str(), nullptr, 1))
    {
      printf("[ADD-ONS] ERROR: Couldn't mount add-on archive '%s': %s\n", addon.filename.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
      return;
    }
    addon.mounted = true;
    printf("[ADD-ONS] SUCCESS: Mounted add-on archive '%s' (prepended: %s)\n", addon.filename.c_str(), addon.overrides_data ? "yes" : "no");

    // Ensure the add-on's dependencies are also enabled.
    // Notify user a restart is required if any disabled dependency overrides data.
    bool dep_overrides_data = false;
    for (const std::string& dep_id : addon.dependencies)
    {
      const auto dep_it = addons.find(dep_id);
      if (dep_it == addons.end())
      {
        printf("[ADD-ONS] WARNING: Dependency '%s' of add-on '%s' is not available!\n", dep_id.c_str(), addon_it->first.c_str());
        continue;
      }

      bool& dep_addon_enabled = addons_enabled[dep_id];
      if (dep_addon_enabled)
        continue;
      dep_addon_enabled = true;

      Addon& dep_addon = dep_it->second;

      // Do not actually enable add-ons which override data after game has started.
      // Notify the user later instead.
      if (dep_addon.overrides_data)
      {
        if (dep_addon.mounted != dep_addon_enabled)
          dep_overrides_data = true;
        continue;
      }

      const std::string filepath = "addons/" + dep_addon.filename;
      const char* realdir = PHYSFS_getRealDir(filepath.c_str());
      if (!realdir)
      {
        printf("[ADD-ONS] ERROR: PHYSFS_getRealDir() failed for 'addons/%s': %s\n", dep_addon.filename.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        continue;
      }
      if (!PHYSFS_mount((std::string(realdir) + "/" + filepath).c_str(), nullptr, !dep_addon.overrides_data))
      {
        printf("[ADD-ONS] ERROR: Couldn't mount add-on archive '%s': %s\n", dep_addon.filename.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        continue;
      }
      dep_addon.mounted = true;
      printf("[ADD-ONS] SUCCESS: Mounted add-on archive '%s' (prepended: %s)\n", dep_addon.filename.c_str(), dep_addon.overrides_data ? "yes" : "no");
    }
    if (dep_overrides_data)
      Menu::push_current(restart_info_menu);

    generate_addons_menu(false);
  }
  else /* DISABLE */
  {
    // Unmount the add-on
    if (!PHYSFS_unmount(full_archive_path.c_str()))
    {
      printf("[ADD-ONS] ERROR: Couldn't unmount add-on archive '%s': %s\n", addon.filename.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
      return;
    }
    addon.mounted = false;
    printf("[ADD-ONS] SUCCESS: Unmounted add-on archive '%s'\n", addon.filename.c_str());
  }
}

void process_addons_download_menu()
{
  const int idx = addons_download_menu->check();
  if (idx < 0)
  {
    switch (idx)
    {
      case MNID_PREV_PAGE:
        assert(addons_download_menu_page > 0);
        --addons_download_menu_page;
        generate_addons_download_menu();
        addons_download_menu->set_active_item_id(MNID_PREV_PAGE);
        break;
      case MNID_NEXT_PAGE:
        assert(std::count_if(addon_index.begin(), addon_index.end(),
            [](const auto& addon_entry)
            {
              return addons.find(addon_entry.first) == addons.end();
            }) > (addons_download_menu_page + 1) * addons_per_page);
        ++addons_download_menu_page;
        generate_addons_download_menu();
        addons_download_menu->set_active_item_id(MNID_NEXT_PAGE);
        break;
    }
    return;
  }

  auto addon_it = addon_index.begin();
  std::advance(addon_it, idx);

  assert(addons.find(addon_it->first) == addons.end());
  const IndexAddon& addon = addon_it->second;

  // Attempt to install all dependencies of the add-on, which are not installed
  for (const std::string& dep_id : addon.dependencies)
  {
    if (addons.find(dep_id) != addons.end())
      continue;

    const auto dep_it = addon_index.find(dep_id);
    if (dep_it == addon_index.end())
    {
      printf("[ADD-ONS] ERROR: Couldn't find dependency '%s' of add-on '%s' in index!\n", dep_id.c_str(), addon_it->first);
      continue;
    }

    const IndexAddon& dep_addon = dep_it->second;
    assert(!dep_addon.url.empty());

    TransferStatusPtr status = downloader->request_download_file(
      dep_addon.custom_url ? dep_addon.url : addon_index_base_url + "/" + dep_addon.url,
      "addons/" + dep_it->first + ".zip");
    draw_background();
    download_dialog(status);
  }

  // Install the add-on
  TransferStatusPtr status = downloader->request_download_file(
    addon.custom_url ? addon.url : addon_index_base_url + "/" + addon.url,
    "addons/" + addon_it->first + ".zip");

  bool success = false;
  status->then([&success](bool success_) { success = success_; });

  draw_background();
  download_dialog(status);

  // If download was successful, mark the add-on as to-be-enabled
  if (success)
    addons_enabled[addon_it->first] = true;

  // Detect new add-ons, enable dependencies.
  // Notify user a restart is required if any of the newly added add-ons
  // cannot be mounted, as it overrides data.
  if (st_addons_check())
    Menu::push_current(restart_info_menu);

  // Regenerate menus
  generate_addons_menu(false);
  generate_addons_download_menu();
}

bool process_load_game_menu()
{
  int slot = load_game_menu->check();

  if(slot != -1 && load_game_menu->get_item_by_id(slot).kind == MN_ACTION)
    {
      char slotfile[1024];
      snprintf(slotfile, 1024, "save/slot%d.stsg", slot);

      if (!PHYSFS_exists(slotfile))
        draw_intro("intro.txt");

      fadeout();
      WorldMapNS::WorldMap worldmap;
      
      //TODO: Define the circumstances under which BonusIsland is chosen
      worldmap.set_map_file("world1.stwm");
      worldmap.load_map();
     
      // Load the game or at least set the savegame_file variable
      worldmap.loadgame(slotfile);

      worldmap.display();
      
      Menu::set_current(main_menu);

      st_pause_ticks_stop();
      return true;
    }
  else
    {
      return false;
    }
}

/* Handle changes made to global settings in the options menu. */
void process_options_menu(void)
{
  switch (options_menu->check())
    {
    case MNID_OPENGL:
#ifndef NOOPENGL
      if(use_gl != options_menu->isToggled(MNID_OPENGL))
        {
          use_gl = !use_gl;
          st_video_setup();
        }
#else
      options_menu->get_item_by_id(MNID_OPENGL).toggled = false;
#endif
      break;
    case MNID_FULLSCREEN:
      if(use_fullscreen != options_menu->isToggled(MNID_FULLSCREEN))
        {
          use_fullscreen = !use_fullscreen;
          if (use_fullscreen)
          {
            if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) < 0)
            {
              printf("Failed to set window to fullscreen mode: %s\n", SDL_GetError());
              use_fullscreen = false;
              options_menu->get_item_by_id(MNID_FULLSCREEN).toggled = false;
            }
          }
          else if (SDL_SetWindowFullscreen(window, 0) < 0)
          {
            printf("Failed to set window to windowed mode: %s\n", SDL_GetError());
            use_fullscreen = true;
            options_menu->get_item_by_id(MNID_FULLSCREEN).toggled = true;
          }
        }
      break;
    case MNID_SOUND:
      if(use_sound != options_menu->isToggled(MNID_SOUND))
        use_sound = !use_sound;
      break;
    case MNID_MUSIC:
      if(use_music != options_menu->isToggled(MNID_MUSIC))
        {
          use_music = !use_music;
          music_manager->enable_music(use_music);
        }
      break;
    case MNID_SHOWFPS:
      if(show_fps != options_menu->isToggled(MNID_SHOWFPS))
        show_fps = !show_fps;
      break;
    case MNID_BACKSCROLLING:
      if(back_scrolling != options_menu->isToggled(MNID_BACKSCROLLING))
        back_scrolling = !back_scrolling;
      break;
    }
}

/* Handle changes made to global settings in the options keys menu. */
void process_options_keys_menu(void)
{
  switch (options_keys_menu->check())
    {
    case MNID_RUN_BY_DEFAULT:
      if(run_by_default != options_keys_menu->isToggled(MNID_RUN_BY_DEFAULT))
        run_by_default = !run_by_default;
      break;
    }
}

void st_general_setup(void)
{
  /* Seed random number generator: */

  srand(SDL_GetTicks());

  /* Initialize SDL2_mixer with required codecs */

  const int mixer_flags = MIX_INIT_OGG | MIX_INIT_MOD;
  if (Mix_Init(mixer_flags) != mixer_flags)
  {
    fprintf(stderr, "Mix_Init: Failed to init required codecs! %s\n", Mix_GetError());
    st_shutdown();
    abort();
    return;
  }

  /* Load global images: */

  black_text  = new Text("/images/status/letters-black.png", TEXT_TEXT, 16,18);
  gold_text   = new Text("/images/status/letters-gold.png", TEXT_TEXT, 16,18);
  silver_text = new Text("/images/status/letters-silver.png", TEXT_TEXT, 16,18);
  blue_text   = new Text("/images/status/letters-blue.png", TEXT_TEXT, 16,18);
  red_text    = new Text("/images/status/letters-red.png", TEXT_TEXT, 16,18);
  green_text  = new Text("/images/status/letters-green.png", TEXT_TEXT, 16,18);
  white_text  = new Text("/images/status/letters-white.png", TEXT_TEXT, 16,18);
  white_small_text = new Text("/images/status/letters-white-small.png", TEXT_TEXT, 8,9);
  white_big_text   = new Text("/images/status/letters-white-big.png", TEXT_TEXT, 20,22);
  yellow_nums = new Text("/images/status/numbers.png", TEXT_NUM, 32,32);

  /* Load GUI/menu images: */
  checkbox = new Surface("/images/status/checkbox.png", USE_ALPHA);
  checkbox_checked = new Surface("/images/status/checkbox-checked.png", USE_ALPHA);
  back = new Surface("/images/status/back.png", USE_ALPHA);
  arrow_left = new Surface("/images/icons/left.png", USE_ALPHA);
  arrow_right = new Surface("/images/icons/right.png", USE_ALPHA);
  levelset_addon_icon = new Surface("/images/icons/levelset.png", USE_ALPHA);
  resource_pack_addon_icon = new Surface("/images/icons/resource_pack.png", USE_ALPHA);

  /* Load the mouse-cursor */
  mouse_cursor = new MouseCursor("/images/status/mousecursor.png", 1);
  MouseCursor::set_current(mouse_cursor);

  /* Initialize downloader */
  downloader = new Downloader();
}

void st_general_free(void)
{

  /* Free global images: */
  delete black_text;
  delete gold_text;
  delete silver_text;
  delete white_text;
  delete blue_text;
  delete red_text;
  delete green_text;
  delete white_small_text;
  delete white_big_text;
  delete yellow_nums;

  /* Free GUI/menu images: */
  delete checkbox;
  delete checkbox_checked;
  delete back;
  delete arrow_left;
  delete arrow_right;
  delete levelset_addon_icon;
  delete resource_pack_addon_icon;

  /* Free mouse-cursor */
  delete mouse_cursor;
  
  /* Free menus */
  delete main_menu;
  delete game_menu;
  delete options_menu;
  delete highscore_menu;
  delete save_game_menu;
  delete load_game_menu;
  delete addons_menu;
  delete addons_download_menu;
  delete restart_info_menu;
}

void st_video_setup_gl(int wnd_x, int wnd_y);
void st_video_setup_sdl(int wnd_x, int wnd_y);

void st_video_setup(void)
{
  /* Init SDL Video: */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
	  char err[256];
	  sprintf(err, "Error: I could not initialize video!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", err, window);
      exit(1);
    }

  int wnd_x, wnd_y = SDL_WINDOWPOS_CENTERED;
  SDL_Rect bounds;
  if (SDL_GetDisplayBounds(display_idx, &bounds) == 0)
  {
    wnd_x = bounds.x + (bounds.w - SCREEN_W) / 2;
    wnd_y = bounds.y + (bounds.h - SCREEN_H) / 2;
  }
  else
  {
    printf("Failed to get bounds for display %d: %s\n", display_idx, SDL_GetError());
    display_idx = 0;
  }

  /* Open display: */
  if(use_gl)
    st_video_setup_gl(wnd_x, wnd_y);
  else
    st_video_setup_sdl(wnd_x, wnd_y);

  seticon();
  if (use_fullscreen)
  {
    if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) < 0)
    {
      printf("Failed to set window to fullscreen mode: %s\n", SDL_GetError());
      use_fullscreen = false;
    }
  }

  Surface::reload_all();
}

void st_video_setup_sdl(int wnd_x, int wnd_y)
{
  // Destroy OpenGL video
#ifndef NOOPENGL
  if (!screen)
    SDL_GL_DeleteContext(glcontext);
#endif
  SDL_DestroyWindow(window);

  window = SDL_CreateWindow("SuperTux " VERSION, wnd_x, wnd_y, SCREEN_W, SCREEN_H,
      SDL_WINDOW_RESIZABLE);
  if (window == NULL)
  {
    char err[256];
    sprintf(err, "Error: I could not set up video for 640x480 mode.\n"
                "The Simple DirectMedia error that occured was:\n"
                "%s\n\n", SDL_GetError());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", err, window);

    exit(1);
  }

  renderer = SDL_CreateRenderer(window, -1, 0);
  screen = SDL_CreateRGBSurface(0, SCREEN_W, SCREEN_H, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
  sdl_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);

  SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
  SDL_RenderSetIntegerScale(renderer, SDL_TRUE);
}

void st_video_setup_gl(int wnd_x, int wnd_y)
{
#ifndef NOOPENGL

  // Destroy SDL video
  SDL_DestroyTexture(sdl_texture);
  SDL_FreeSurface(screen);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window = SDL_CreateWindow("SuperTux " VERSION, wnd_x, wnd_y, SCREEN_W, SCREEN_H,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  if (window == NULL)
  {
    char err[256];
    sprintf(err, "Error: I could not set up video for 640x480 mode in OpenGL.\n"
                "The Simple DirectMedia error that occured was:\n"
                "%s\n\n", SDL_GetError());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", err, window);

    exit(1);
  }

  glcontext = SDL_GL_CreateContext(window);

  /*
   * Set up OpenGL for 2D rendering.
   */
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glViewport(0, 0, SCREEN_W, SCREEN_H);
  glviewport_x = 0;
  glviewport_y = 0;
  glviewport_w = SCREEN_W;
  glviewport_h = SCREEN_H;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, SCREEN_W, SCREEN_H, 0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, 0.0f);

#endif
}

int poll_event(SDL_Event& ev)
{
  const int result = SDL_PollEvent(&ev);
#ifndef NOOPENGL
  if (result)
  {
    if (use_gl)
    {
      // OpenGL: Center and resize the logical screen on window resize
      if (ev.type == SDL_WINDOWEVENT &&
          ev.window.event == SDL_WINDOWEVENT_RESIZED)
      {
        int win_w, win_h;
        SDL_GetWindowSize(window, &win_w, &win_h);

        const float scale_x = win_w / screen_w();
        const float scale_y = win_h / screen_h();
        const float scale = scale_x < scale_y ? scale_x : scale_y;

        glviewport_w = static_cast<int>(screen_w() * scale);
        glviewport_h = static_cast<int>(screen_h() * scale);
        glviewport_x = (win_w - glviewport_w) / 2;
        glviewport_y = (win_h - glviewport_h) / 2;

        clearscreen(0, 0, 0);
        glViewport(glviewport_x, glviewport_y, glviewport_w, glviewport_h);
      }
      // OpenGL: Translate mouse motion event mouse position to logical screen
      else if (ev.type == SDL_MOUSEMOTION)
      {
        ev.motion.x = static_cast<int>((ev.motion.x - glviewport_x) * (static_cast<float>(screen_w()) / glviewport_w));
        ev.motion.y = static_cast<int>((ev.motion.y - glviewport_y) * (static_cast<float>(screen_h()) / glviewport_h));
      }
      else if (ev.type == SDL_MOUSEBUTTONUP || ev.type == SDL_MOUSEBUTTONDOWN)
      {
        ev.button.x = static_cast<int>((ev.motion.x - glviewport_x) * (static_cast<float>(screen_w()) / glviewport_w));
        ev.button.y = static_cast<int>((ev.motion.y - glviewport_y) * (static_cast<float>(screen_h()) / glviewport_h));
      }
    }
  }
#endif
  return result;
}

void st_joystick_setup(void)
{

  /* Init Joystick: */

  use_joystick = true;

  if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
    {
      fprintf(stderr, "Warning: I could not initialize joystick!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());

      use_joystick = false;
    }
  else
    {
      /* Open joystick: */
      if (SDL_NumJoysticks() <= 0)
        {
          fprintf(stderr, "Warning: No joysticks are available.\n");

          use_joystick = false;
        }
      else
        {
          js = SDL_JoystickOpen(joystick_num);

          if (js == NULL)
            {
              fprintf(stderr, "Warning: Could not open joystick %d.\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", joystick_num, SDL_GetError());

              use_joystick = false;
            }
          else
            {
              if (SDL_JoystickNumAxes(js) < 2)
                {
                  fprintf(stderr,
                          "Warning: Joystick does not have enough axes!\n");

                  use_joystick = false;
                }
              else
                {
                  if (SDL_JoystickNumButtons(js) < 2)
                    {
                      fprintf(stderr,
                              "Warning: "
                              "Joystick does not have enough buttons!\n");

                      use_joystick = false;
                    }
                }
            }
        }
    }
}

void st_audio_setup(void)
{

  /* Init SDL Audio silently even if --disable-sound : */

  if (audio_device)
    {
      if (SDL_Init(SDL_INIT_AUDIO) < 0)
        {
          /* only print out message if sound or music
             was not disabled at command-line
           */
          if (use_sound || use_music)
            {
              fprintf(stderr,
                      "\nWarning: I could not initialize audio!\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", SDL_GetError());
            }
          /* keep the programming logic the same :-)
             because in this case, use_sound & use_music' values are ignored
             when there's no available audio device
          */
          use_sound = false;
          use_music = false;
          audio_device = false;
        }
    }


  /* Open sound silently regarless the value of "use_sound": */

  if (audio_device)
    {
      if (open_audio(44100, AUDIO_S16, 2, 2048) < 0)
        {
          /* only print out message if sound or music
             was not disabled at command-line
           */
          if (use_sound || use_music)
            {
              fprintf(stderr,
                      "\nWarning: I could not set up audio for 44100 Hz "
                      "16-bit stereo.\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", SDL_GetError());
            }
          use_sound = false;
          use_music = false;
          audio_device = false;
        }
    }

}


/* --- SHUTDOWN --- */

void st_shutdown(void)
{
  display_idx = SDL_GetWindowDisplayIndex(window);

  // Destroy video
#ifndef NOOPENGL
  if (use_gl)
    SDL_GL_DeleteContext(glcontext);
  else
  {
#endif
    SDL_DestroyTexture(sdl_texture);
    SDL_FreeSurface(screen);
    SDL_DestroyRenderer(renderer);
#ifndef NOOPENGL
  }
#endif
  SDL_DestroyWindow(window);

  close_audio();
  SDL_Quit();
  saveconfig();
  PHYSFS_deinit();
}

/* --- ABORT! --- */

void st_abort(const std::string& reason, const std::string& details)
{
  fprintf(stderr, "\nError: %s\n%s\n\n", reason.c_str(), details.c_str());
  st_shutdown();
  abort();
}

/* Set Icon (private) */

void seticon(void)
{
//  int masklen;
//  Uint8 * mask;
  SDL_Surface * icon = raw_sdl_surface_from_file("images/icon.xpm");
  if (icon == NULL)
    {
      fprintf(stderr,
              "\nError: I could not load the icon image: %s\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", "images/icon.xpm", SDL_GetError());
      exit(1);
    }


  /* Create mask: */
/*
  masklen = (((icon -> w) + 7) / 8) * (icon -> h);
  mask = (Uint8*) malloc(masklen * sizeof(Uint8));
  memset(mask, 0xFF, masklen);
*/

  /* Set icon: */

  SDL_SetWindowIcon(window, icon);

  /* Free icon surface & mask: */

  SDL_FreeSurface(icon);
}


/* Parse command-line arguments: */

void parseargs(int argc, char * argv[])
{
  int i;

  loadconfig();

  /* Parse arguments: */

  for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "--fullscreen") == 0 ||
          strcmp(argv[i], "-f") == 0)
        {
          /* Use full screen: */

          use_fullscreen = true;
        }
      else if (strcmp(argv[i], "--joystick") == 0 || strcmp(argv[i], "-j") == 0)
        {
          assert(i+1 < argc);
          joystick_num = atoi(argv[++i]);
        }
      else if (strcmp(argv[i], "--joymap") == 0)
        {
          assert(i+1 < argc);
          if (sscanf(argv[++i],
                     "%d:%d:%d:%d:%d", 
                     &joystick_keymap.x_axis, 
                     &joystick_keymap.y_axis, 
                     &joystick_keymap.a_button, 
                     &joystick_keymap.b_button, 
                     &joystick_keymap.start_button) != 5)
            {
              puts("Warning: Invalid or incomplete joymap, should be: 'XAXIS:YAXIS:A:B:START'");
            }
          else
            {
              std::cout << "Using new joymap:\n"
                        << "  X-Axis:       " << joystick_keymap.x_axis << "\n"
                        << "  Y-Axis:       " << joystick_keymap.y_axis << "\n"
                        << "  A-Button:     " << joystick_keymap.a_button << "\n"
                        << "  B-Button:     " << joystick_keymap.b_button << "\n"
                        << "  Start-Button: " << joystick_keymap.start_button << std::endl;
            }
        }
      else if (strcmp(argv[i], "--leveleditor") == 0)
        {
          launch_leveleditor_mode = true;
        }
      else if (strcmp(argv[i], "--datadir") == 0 ||
               strcmp(argv[i], "-d") == 0)
        {
          // Datadir is parsed in st_directory_setup().
          assert(i+1 < argc);
          ++i;
        }
      else if (strcmp(argv[i], "--userdir") == 0 ||
               strcmp(argv[i], "-u") == 0)
        {
          // Userdir is parsed in st_directory_setup().
          assert(i+1 < argc);
          ++i;
        }
      else if (strcmp(argv[i], "--show-fps") == 0)
        {
          /* Use full screen: */

          show_fps = true;
        }
      else if (strcmp(argv[i], "--opengl") == 0 ||
               strcmp(argv[i], "-gl") == 0)
        {
#ifndef NOOPENGL
          /* Use OpengGL: */

          use_gl = true;
#endif
        }
      else if (strcmp(argv[i], "--sdl") == 0)
          {
            use_gl = false;
          }
      else if (strcmp(argv[i], "--usage") == 0)
        {
          /* Show usage: */

          usage(argv[0], 0);
        }
      else if (strcmp(argv[i], "--version") == 0)
        {
          /* Show version: */
          printf("SuperTux " VERSION "\n");
          exit(0);
        }
      else if (strcmp(argv[i], "--disable-sound") == 0)
        {
          /* Disable the compiled in sound feature */
          printf("Sounds disabled \n");
          use_sound = false;
          audio_device = false;
        }
      else if (strcmp(argv[i], "--disable-music") == 0)
        {
          /* Disable the compiled in sound feature */
          printf("Music disabled \n");
          use_music = false;
        }
      else if (strcmp(argv[i], "--debug-mode") == 0)
        {
          /* Enable the debug-mode */
          debug_mode = true;

        }
      else if (strcmp(argv[i], "--help") == 0)
        {     /* Show help: */
          puts("Super Tux " VERSION "\n"
               "  Please see the file \"README.txt\" for more details.\n");
          printf("Usage: %s [OPTIONS] FILENAME\n\n", argv[0]);
          puts("Display Options:\n"
               "  --fullscreen        Run in fullscreen mode.\n"
               "  --opengl            If opengl support was compiled in, this will enable\n"
               "                      the OpenGL mode.\n"
               "  --sdl               Use non-opengl renderer\n"
               "\n"
               "Sound Options:\n"
               "  --disable-sound     If sound support was compiled in,  this will\n"
               "                      disable sound for this session of the game.\n"
               "  --disable-music     Like above, but this will disable music.\n"
               "\n"
               "Misc Options:\n"
               "  -j, --joystick NUM  Use joystick NUM (default: 0)\n" 
               "  --joymap XAXIS:YAXIS:A:B:START\n"
               "  --leveleditor       Opens the leveleditor in a file. (Only works when a file is provided.)\n"
               "                      Define how joystick buttons and axis should be mapped\n"
               "  -d, --datadir DIR   Load Game data from DIR [RELATIVE to working directory] (default: automatic)\n"
               "  -u, --userdir DIR   Read/write user data (addons, levels, savefiles...) to DIR [RELATIVE to working directory] (default: automatic)\n"
               "  --debug-mode        Enables the debug-mode, which is useful for developers.\n"
               "  --help              Display a help message summarizing command-line\n"
               "                      options, license and game controls.\n"
               "  --usage             Display a brief message summarizing command-line options.\n"
               "  --version           Display the version of SuperTux you're running.\n\n"
               );
          exit(0);
        }
      else if (argv[i][0] != '-')
        {
          level_startup_file = argv[i];
        }
      else
        {
          /* Unknown - complain! */

          usage(argv[0], 1);
        }
    }
}


/* Display usage: */

void usage(char * prog, int ret)
{
  FILE * fi;


  /* Determine which stream to write to: */

  if (ret == 0)
    fi = stdout;
  else
    fi = stderr;


  /* Display the usage message: */

  fprintf(fi, "Usage: %s [--fullscreen] [--opengl] [--disable-sound] [--disable-music] [--debug-mode] | [--usage | --help | --version] FILENAME\n",
          prog);


  /* Quit! */

  exit(ret);
}

