//  $Id$
//
//  SuperTux
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
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

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "defines.h"
#include "globals.h"
#include "menu.h"
#include "screen.h"
#include "setup.h"
#include "sound.h"
#include "scene.h"
#include "leveleditor.h"
#include "timer.h"
#include "high_scores.h"
#include "downloader.h"

#define FLICK_CURSOR_TIME 500

Surface* checkbox;
Surface* checkbox_checked;
Surface* back;
Surface* arrow_left;
Surface* arrow_right;
Surface* levelset_addon_icon;
Surface* resource_pack_addon_icon;

Menu* main_menu      = 0;
Menu* game_menu      = 0;
Menu* worldmap_menu  = 0;
Menu* options_menu   = 0;
Menu* options_keys_menu     = 0;
Menu* options_joystick_menu = 0;
Menu* highscore_menu = 0;
Menu* load_game_menu = 0;
Menu* save_game_menu = 0;
Menu* contrib_menu   = 0;
Menu* contrib_subset_menu   = 0;
Menu* addons_menu   = 0;
Menu* addons_download_menu  = 0;

Menu* restart_info_menu = 0;

std::vector<Menu*> Menu::last_menus;
Menu* Menu::current_ = 0;

/* just displays a Yes/No text that can be used to confirm stuff */
bool confirm_dialog(std::string text)
{
  Surface* cap_screen = Surface::CaptureScreen();
  
  Menu* dialog = new Menu;
  dialog->additem(MN_DEACTIVE, text,0,0);
  dialog->additem(MN_HL,"",0,0);
  dialog->additem(MN_ACTION,"Yes",0,0,true);
  dialog->additem(MN_ACTION,"No",0,0,false);
  dialog->additem(MN_HL,"",0,0);

  const int cursor_shown = SDL_ShowCursor(SDL_ENABLE);

  while (true)
  {
    SDL_Event event;
    while (poll_event(event))
    {
      dialog->event(event);
    }

    cap_screen->draw(0,0);

    dialog->draw();
    dialog->action();

    switch (dialog->check())
    {
    case true:
      SDL_ShowCursor(cursor_shown);
      delete cap_screen;
      delete dialog;
      return true;
      break;
    case false:
      SDL_ShowCursor(cursor_shown);
      delete cap_screen;
      delete dialog;
      return false;
      break;
    default:
      break;
    }

    mouse_cursor->update();
    flipscreen();
    SDL_Delay(25);
  }
  return false;
}

/* displays a dialog that updates and shows download progress with an "Abort" button */
void download_dialog(TransferStatusPtr status)
{
  Surface* cap_screen = Surface::CaptureScreen();

  const std::string remote_file = status->file;

  Menu* dialog = new Menu;
  dialog->additem(MN_LABEL, "Downloading \""
      + (remote_file.size() > 18 ? remote_file.substr(0, 15) + "..." : remote_file)
      + "\"" ,0,0,0);
  dialog->additem(MN_HL,"",0,0);
  dialog->additem(MN_DEACTIVE, "-/- kB" ,0,0,1);
  dialog->additem(MN_DEACTIVE, "0%" ,0,0,2);
  dialog->additem(MN_HL,"",0,0);
  dialog->additem(MN_PURE_ACTION,"Abort",0,0,3);
  dialog->additem(MN_HL,"",0,0);

  bool complete, success, aborted = false;
  std::string error_msg;
  status->then([&status, &complete, &success, &error_msg](bool success_)
    {
      complete = true;
      success = success_;
      if (!success)
        error_msg = status->error_msg;
    });

  const int cursor_shown = SDL_ShowCursor(SDL_ENABLE);

  while (!complete)
  {
    SDL_Event event;
    while (poll_event(event))
    {
      dialog->event(event);
    }

    cap_screen->draw(0,0);

    dialog->draw();
    dialog->action();

    if (dialog->check() == 3)
    {
      status->abort();
      assert(complete && !success);
      aborted = true;
      break;
    }

    status->update();

    dialog->get_item_by_id(1).change_text((std::to_string((complete ? status->dltotal : status->dlnow) / 1000)
        + "/" + std::to_string(status->dltotal / 1000) + " kB").c_str());
    dialog->get_item_by_id(2).change_text((std::to_string(complete ? 100 :
          (status->dltotal <= 0 ? 0 : static_cast<int>(roundf(100 * (status->dlnow / static_cast<float>(status->dltotal)))))) + "%").c_str());

    mouse_cursor->update();
    flipscreen();
    SDL_Delay(25);
  }

  // On unsuccessful download, display an error dialog
  if (!success && !aborted)
  {
    Menu* error_dialog = new Menu(true);
    error_dialog->additem(MN_LABEL, "Error downloading \""
        + (remote_file.size() > 12 ? remote_file.substr(0, 9) + "..." : remote_file)
        + "\"" ,0,0,0);
    error_dialog->additem(MN_HL,"",0,0);
    error_dialog->additem(MN_DEACTIVE, error_msg.empty() ? "Unknown error!" :
        (static_cast<int>(error_msg.size()) * white_text->w > screen_w() ?
          error_msg.substr(0, screen_w() / white_text->w - 3) + "..." : error_msg),0,0,0);
    error_dialog->additem(MN_HL,"",0,0);
    error_dialog->additem(MN_BACK,"Close",0,0,0);
    error_dialog->additem(MN_HL,"",0,0);

    Menu::push_current(error_dialog);
  }

  SDL_ShowCursor(cursor_shown);

  delete cap_screen;
  delete dialog;
}

void
Menu::push_current(Menu* pmenu)
{
  if (current_)
    last_menus.push_back(current_);

  current_ = pmenu;
  current_->effect.start(500);
}

void
Menu::pop_current()
{
  if (current_ && current_->free_on_close)
    delete current_;

  if (!last_menus.empty())
  {
    current_ = last_menus.back();
    current_->effect.start(500);

    last_menus.pop_back();
  }
  else
  {
    current_ = 0;
  }
}

void
Menu::set_current(Menu* menu)
{
  if (current_ && current_->free_on_close)
    delete current_;

  for (Menu* prev_menu : last_menus)
  {
    if (prev_menu->free_on_close)
      delete prev_menu;
  }
  last_menus.clear();

  if (menu)
    menu->effect.start(500);

  current_ = menu;
}

/* Return a pointer to a new menu item */
MenuItem*
MenuItem::create(MenuItemKind kind_, const char *text_, int init_toggle_, Menu* target_menu_, int id, int* int_p_, Surface* icon)
{
  MenuItem *pnew_item = new MenuItem;

  pnew_item->kind = kind_;
  pnew_item->text = (char*) malloc(sizeof(char) * (strlen(text_) + 1));
  strcpy(pnew_item->text, text_);

  if(kind_ == MN_TOGGLE)
    pnew_item->toggled = init_toggle_;
  else
    pnew_item->toggled = false;

  pnew_item->target_menu = target_menu_;
  pnew_item->input = (char*) malloc(sizeof(char));
  pnew_item->input[0] = '\0';

  if(kind_ == MN_STRINGSELECT)
  {
    pnew_item->list = (string_list_type*) malloc(sizeof(string_list_type));
    string_list_init(pnew_item->list);
  }
  else
    pnew_item->list = NULL;

  pnew_item->id = id;
  pnew_item->int_p = int_p_;

  pnew_item->icon = icon;

  pnew_item->input_flickering = false;
  pnew_item->input_flickering_timer.init(true);
  pnew_item->input_flickering_timer.start(FLICK_CURSOR_TIME);

  return pnew_item;
}

void
MenuItem::change_text(const  char *text_)
{
  if (text_)
  {
    free(text);
    text = (char*) malloc(sizeof(char )*(strlen(text_)+1));
    strcpy(text, text_);
  }
}

void
MenuItem::change_input(const  char *text_)
{
  if(text)
  {
    free(input);
    input = (char*) malloc(sizeof(char )*(strlen(text_)+1));
    strcpy(input, text_);
  }
}

std::string MenuItem::get_input_with_symbol(bool active_item)
{
  if(!active_item)
    input_flickering = true;
  else
  {
    if(input_flickering_timer.get_left() < 0)
    {
      if(input_flickering)
        input_flickering = false;
      else
        input_flickering = true;
      input_flickering_timer.start(FLICK_CURSOR_TIME);
    }
  }

  char str[1024];
  if(input_flickering)
    sprintf(str,"%s_",input);
  else
    sprintf(str,"%s ",input);

  std::string string = str;

  return string;
}

bool
MenuItem::is_active() const
{
  return kind != MN_DEACTIVE &&
    kind != MN_LABEL &&
    kind != MN_HL;
}

/* Set ControlField a key */
void Menu::get_controlfield_key_into_input(MenuItem *item)
{
  switch(*item->int_p)
  {
  case SDLK_UP:
    item->change_input("Up cursor");
    break;
  case SDLK_DOWN:
    item->change_input("Down cursor");
    break;
  case SDLK_LEFT:
    item->change_input("Left cursor");
    break;
  case SDLK_RIGHT:
    item->change_input("Right cursor");
    break;
  case SDLK_RETURN:
    item->change_input("Return");
    break;
  case SDLK_SPACE:
    item->change_input("Space");
    break;
  case SDLK_RSHIFT:
    item->change_input("Right Shift");
    break;
  case SDLK_LSHIFT:
    item->change_input("Left Shift");
    break;
  case SDLK_RCTRL:
    item->change_input("Right Control");
    break;
  case SDLK_LCTRL:
    item->change_input("Left Control");
    break;
  case SDLK_RALT:
    item->change_input("Right Alt");
    break;
  case SDLK_LALT:
    item->change_input("Left Alt");
    break;
  default:
    {
      char tmp[64];
      snprintf(tmp, 64, "%d", *item->int_p);
      item->change_input(tmp);
    }
    break;
  }
}

/* Free a menu and all its items */
Menu::~Menu()
{
  if(item.size() != 0)
  {
    for(unsigned int i = 0; i < item.size(); ++i)
    {
      free(item[i].text);
      free(item[i].input);
      string_list_free(item[i].list);
    }
  }
}


Menu::Menu(bool free_on_close_) :
  free_on_close(free_on_close_)
{
  hit_item = -1;
  menuaction = MENU_ACTION_NONE;
  delete_character = 0;
  mn_input_chars[0] = '\0';

  pos_x        = screen_w()/2;
  pos_y        = screen_h()/2;
  arrange_left = 0;
  active_item  = 0;
  effect.init(false);
}

void Menu::set_pos(int x, int y, float rw, float rh)
{
  pos_x = x + (int)((float)get_width() * rw);
  pos_y = y + (int)((float)get_height() * rh);
}

void
Menu::additem(MenuItemKind kind_, const std::string& text_, int toggle_, Menu* menu_, int id, int* int_p, Surface* icon)
{
  additem(MenuItem::create(kind_, text_.c_str(), toggle_, menu_, id, int_p, icon));
}

/* Add an item to a menu */
void
Menu::additem(MenuItem* pmenu_item)
{
  item.push_back(*pmenu_item);
  delete pmenu_item;
}

void
Menu::clear()
{
  item.clear();
}

/* Process actions done on the menu */
void
Menu::action()
{
  hit_item = -1;
  if(item.size() != 0)
  {
    switch(menuaction)
    {
    case MENU_ACTION_UP:
    {
      if (active_item > 0)
        --active_item;
      else
        active_item = int(item.size())-1;

      // Skip inactive/decorative menu items
      const MenuItem& new_item = item[active_item];
      if (!new_item.is_active())
      {
        if (item.size() > 1 &&
            std::any_of(item.begin(), item.end(), [](const MenuItem& item) { return item.is_active(); }))
          action();
      }
      break;
    }

    case MENU_ACTION_DOWN:
    {
      if(active_item < int(item.size())-1)
        ++active_item;
      else
        active_item = 0;

      // Skip inactive/decorative menu items
      const MenuItem& new_item = item[active_item];
      if (!new_item.is_active())
      {
        if (item.size() > 1 &&
            std::any_of(item.begin(), item.end(), [](const MenuItem& item) { return item.is_active(); }))
          action();
      }
      break;
    }

    case MENU_ACTION_LEFT:
      if(item[active_item].kind == MN_STRINGSELECT
          && item[active_item].list->num_items != 0)
      {
        if(item[active_item].list->active_item > 0)
          --item[active_item].list->active_item;
        else
          item[active_item].list->active_item = item[active_item].list->num_items-1;
      }
      break;

    case MENU_ACTION_RIGHT:
      if(item[active_item].kind == MN_STRINGSELECT
          && item[active_item].list->num_items != 0)
      {
        if(item[active_item].list->active_item < item[active_item].list->num_items-1)
          ++item[active_item].list->active_item;
        else
          item[active_item].list->active_item = 0;
      }
      break;

    case MENU_ACTION_HIT:
      {
        const MenuItem& hovered_item = item[active_item];
        if (hovered_item.kind == MN_DEACTIVE ||
            hovered_item.kind == MN_LABEL ||
            hovered_item.kind == MN_HL)
          break;

        hit_item = active_item;
        switch (item[active_item].kind)
        {
        case MN_GOTO:
          if (item[active_item].target_menu != NULL)
            Menu::push_current(item[active_item].target_menu);
          else
            puts("NULLL");
          break;

        case MN_TOGGLE:
          item[active_item].toggled = !item[active_item].toggled;
          break;

        case MN_ACTION:
          Menu::set_current(0);
          item[active_item].toggled = true;
          break;
        case MN_TEXTFIELD:
        case MN_NUMFIELD:
          menuaction = MENU_ACTION_DOWN;
          action();
          break;

        case MN_BACK:
          Menu::pop_current();
          break;
        default:
          break;
        }
      }
      break;

    case MENU_ACTION_REMOVE:
      if(item[active_item].kind == MN_TEXTFIELD
          || item[active_item].kind == MN_NUMFIELD)
      {
        if(item[active_item].input != NULL)
        {
          int i = strlen(item[active_item].input);

          while(delete_character > 0)	/* remove charactes */
          {
            item[active_item].input[i-1] = '\0';
            delete_character--;
          }
        }
      }
      break;

    case MENU_ACTION_INPUT:
      if(item[active_item].kind == MN_TEXTFIELD ||
         item[active_item].kind == MN_NUMFIELD) //&& mn_input_char >= '0' && mn_input_char <= '9'))
      {
        char input_chars[16];
        if (item[active_item].kind == MN_NUMFIELD)
        {
          int input_idx = 0;
          for (char& c : mn_input_chars)
          {
            if (c >= '0' && c <= '9')
              input_chars[input_idx++] = c;
          }
          input_chars[input_idx] = '\0';
        }
        else
        {
          strcpy(input_chars, mn_input_chars);
        }

        const int input_len = strlen(input_chars);
        if(item[active_item].input != NULL)
        {
          const int i = strlen(item[active_item].input);
          item[active_item].input = (char*) realloc(item[active_item].input,sizeof(char) * (i + input_len + 1));
          strcpy(item[active_item].input + i, input_chars);
          item[active_item].input[i + input_len] = '\0';
        }
        else
        {
          item[active_item].input = (char*) malloc(sizeof(char) * (input_len + 1));
          strcpy(item[active_item].input, input_chars);
          item[active_item].input[input_len] = '\0';
        }
      }

    case MENU_ACTION_NONE:
      break;
    }
  }

  menuaction = MENU_ACTION_NONE;
}

int
Menu::check()
{
  if (hit_item != -1)
    return item[hit_item].id;
  else
    return -1;
}

void
Menu::draw_item(int index, // Position of the current item in the menu
                int menu_width,
                int menu_height,
                int scroll_offset)
{
  MenuItem& pitem = item[index];

  int effect_offset = 0;
  {
    int effect_time = 0;

    if(effect.check())
      effect_time = effect.get_left() / 4;

    effect_offset = (index % 2) ? effect_time : -effect_time;
  }

  int x_pos       = pos_x;
  int y_pos       = pos_y + 24 * index - menu_height / 2 - scroll_offset + 12 + effect_offset;
  int shadow_size = 2;

  Text* text_font = white_text;
  const int font_width = text_font->w;
  int text_width  = strlen(pitem.text) * font_width;
  int input_width = (strlen(pitem.input)+ 1) * font_width;
  int list_width  = strlen(string_list_active(pitem.list)) * font_width;

  if (arrange_left)
    x_pos += 24 - menu_width/2 + (text_width + input_width + list_width)/2;

  if(index == active_item)
  {
    shadow_size = 3;
    text_font = blue_text;
  }

  if (pitem.icon)
    pitem.icon->draw(x_pos - text_width / 2 - 20 - pitem.icon->impl->w / 2, y_pos - 8);

  switch (pitem.kind)
  {
  case MN_DEACTIVE:
    {
      black_text->draw_align(pitem.text,
                             x_pos, y_pos,
                             A_HMIDDLE, A_VMIDDLE, 2);
      break;
    }

  case MN_HL:
    {
      int x = pos_x - menu_width/2;
      int y = y_pos - 12 - effect_offset;
      /* Draw a horizontal line with a little 3d effect */
      fillrect(x, y + 6,
               menu_width, 4,
               150,200,255,225);
      fillrect(x, y + 6,
               menu_width, 2,
               255,255,255,255);
      break;
    }
  case MN_LABEL:
    {
      white_big_text->draw_align(pitem.text,
                                 x_pos, y_pos,
                                 A_HMIDDLE, A_VMIDDLE, 2);
      break;
    }
  case MN_TEXTFIELD:
  case MN_NUMFIELD:
  case MN_CONTROLFIELD:
    {
      int input_pos = input_width/2;
      int text_pos  = (text_width + font_width)/2;

      fillrect(x_pos - input_pos + text_pos - 1, y_pos - 10,
               input_width + font_width + 2, 20,
               255,255,255,255);
      fillrect(x_pos - input_pos + text_pos, y_pos - 9,
               input_width + font_width, 18,
               0,0,0,128);

      if(pitem.kind == MN_CONTROLFIELD)
        get_controlfield_key_into_input(&pitem);

      if(pitem.kind == MN_TEXTFIELD || pitem.kind == MN_NUMFIELD)
      {
        if(active_item == index)
          gold_text->draw_align((pitem.get_input_with_symbol(true)).c_str(), x_pos + text_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
        else
          gold_text->draw_align((pitem.get_input_with_symbol(false)).c_str(), x_pos + text_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
      }
      else
        gold_text->draw_align(pitem.input,
                              x_pos + text_pos, y_pos,
                              A_HMIDDLE, A_VMIDDLE, 2);

      text_font->draw_align(pitem.text,
                            x_pos - (input_width + font_width)/2, y_pos,
                            A_HMIDDLE, A_VMIDDLE, shadow_size);
      break;
    }
  case MN_STRINGSELECT:
    {
      int list_pos_2 = list_width + font_width;
      int list_pos   = list_width/2;
      int text_pos   = (text_width + font_width)/2;

      /* Draw arrows */
      arrow_left->draw(  x_pos - list_pos + text_pos - 17, y_pos - 8);
      arrow_right->draw( x_pos - list_pos + text_pos - 1 + list_pos_2, y_pos - 8);

      /* Draw input background */
      fillrect(x_pos - list_pos + text_pos - 1, y_pos - 10,
               list_pos_2 + 2, 20,
               255,255,255,255);
      fillrect(x_pos - list_pos + text_pos, y_pos - 9,
               list_pos_2, 18,
               0,0,0,128);

      gold_text->draw_align(string_list_active(pitem.list),
                            x_pos + text_pos, y_pos,
                            A_HMIDDLE, A_VMIDDLE,2);

      text_font->draw_align(pitem.text,
                            x_pos - list_pos_2/2, y_pos,
                            A_HMIDDLE, A_VMIDDLE, shadow_size);
      break;
    }
  case MN_BACK:
    {
      text_font->draw_align(pitem.text, x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      back->draw( x_pos + text_width/2  + font_width, y_pos - 8);
      break;
    }

  case MN_TOGGLE:
    {
      x_pos -= checkbox->impl->w / 2;
      text_font->draw_align(pitem.text, x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);

      if(pitem.toggled)
        checkbox_checked->draw(
          x_pos + (text_width+font_width)/2,
          y_pos - 8);
      else
        checkbox->draw(
          x_pos + (text_width+font_width)/2,
          y_pos - 8);
      break;
    }
  case MN_ACTION:
  case MN_PURE_ACTION:
    text_font->draw_align(pitem.text, x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
    break;

  case MN_GOTO:
    text_font->draw_align(pitem.text, x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
    break;
  }
}

int
Menu::get_width() const
{
  /* The width of the menu has to be more than the width of the text
     with the most characters */
  int menu_width = 0;
  for(unsigned int i = 0; i < item.size(); ++i)
  {
    int w = strlen(item[i].text) + (item[i].input ? strlen(item[i].input) + 1 : 0) + strlen(string_list_active(item[i].list));
    if( w > menu_width )
    {
      menu_width = w;
      if(item[i].kind == MN_TOGGLE)
        menu_width += 2;
      if(item[i].icon)
        menu_width += 2;
    }
  }

  return (menu_width * 16 + 24);
}

int
Menu::get_height() const
{
  return item.size() * 24;
}

int
Menu::get_scroll_offset() const
{
  const int height_diff = get_height() - screen_h();
  return height_diff > 0 ?
    // This contains a few magic values, but it seems to work fine so I won't complain...
    -height_diff / 2 - 40 + ((height_diff + 150) / static_cast<int>(item.size())) * active_item
    : 0;
}

/* Draw the current menu. */
void
Menu::draw()
{
  const int menu_width  = get_width();
  const int menu_height = get_height();
  const int scroll_offset = get_scroll_offset();

  /* Draw a transparent background */
  fillrect(pos_x - menu_width / 2,
           pos_y - menu_height / 2 - scroll_offset - 10,
           menu_width,menu_height + 20,
           150,180,200,125);

  for(unsigned int i = 0; i < item.size(); ++i)
  {
    draw_item(i, menu_width, menu_height, scroll_offset);
  }
}

MenuItem&
Menu::get_item_by_id(int id)
{
  for(std::vector<MenuItem>::iterator i = item.begin(); i != item.end(); ++i)
  {
    if(i->id == id)
      return *i;
  }

  assert(false);
  static MenuItem dummyitem;
  return dummyitem;
}

int
Menu::get_active_item_id() const
{
  return item.at(active_item).id;
}

void
Menu::set_active_item_id(int id)
{
  for (int i = 0; i < static_cast<int>(item.size()); ++i)
  {
    if (item[i].id == id)
    {
      active_item = i;
      break;
    }
  }
}

bool
Menu::isToggled(int id)
{
  return get_item_by_id(id).toggled;
}

/* Check for menu event */
void
Menu::event(SDL_Event& event)
{
  SDL_Keycode key;
  switch(event.type)
  {
  case SDL_KEYDOWN:
    key = event.key.keysym.sym;
    SDL_Keymod keymod;
    keymod = SDL_GetModState();
    int x,y;

    if(item[active_item].kind == MN_CONTROLFIELD)
    {
      if(key == SDLK_ESCAPE)
      {
        Menu::pop_current();
        return;
      }
      *item[active_item].int_p = key;
      menuaction = MENU_ACTION_DOWN;
      return;
    }

    switch(key)
    {
    case SDLK_UP:		/* Menu Up */
      menuaction = MENU_ACTION_UP;
      break;
    case SDLK_DOWN:		/* Menu Down */
      menuaction = MENU_ACTION_DOWN;
      break;
    case SDLK_LEFT:		/* Menu Up */
      menuaction = MENU_ACTION_LEFT;
      break;
    case SDLK_RIGHT:		/* Menu Down */
      menuaction = MENU_ACTION_RIGHT;
      break;
    case SDLK_SPACE:
      if(item[active_item].kind == MN_TEXTFIELD)
      {
        menuaction = MENU_ACTION_INPUT;
        mn_input_chars[0] = ' ';
        mn_input_chars[1] = '\0';
        break;
      }
    case SDLK_RETURN: /* Menu Hit */
      menuaction = MENU_ACTION_HIT;
      break;
    case SDLK_DELETE:
    case SDLK_BACKSPACE:
      menuaction = MENU_ACTION_REMOVE;
      delete_character++;
      break;
    case SDLK_ESCAPE:
      Menu::pop_current();
      break;
    }
    break;
  case SDL_TEXTINPUT:
    menuaction = MENU_ACTION_INPUT;
    strcpy(mn_input_chars, event.text.text);
    break;
  case  SDL_JOYHATMOTION:
      if(event.jhat.value == SDL_HAT_UP)
           menuaction = MENU_ACTION_UP;
      if(event.jhat.value == SDL_HAT_DOWN)
           menuaction = MENU_ACTION_DOWN;
       break;
  case  SDL_JOYAXISMOTION:
    if(event.jaxis.axis == joystick_keymap.y_axis)
    {
      if (event.jaxis.value > 1024)
        menuaction = MENU_ACTION_DOWN;
      else if (event.jaxis.value < -1024)
        menuaction = MENU_ACTION_UP;
    }
    break;
  case  SDL_JOYBUTTONDOWN:
    menuaction = MENU_ACTION_HIT;
    break;
  case SDL_MOUSEBUTTONDOWN:
  {
    // Do not trigger on mouse wheel scroll
    if (event.button.button > 3)
      break;

    const int menu_width  = get_width();
    const int menu_height = get_height();
    const int scroll_offset = get_scroll_offset();

    x = event.motion.x;
    y = event.motion.y;
    if(x > pos_x - menu_width / 2 &&
        x < pos_x + menu_width / 2 &&
        y > pos_y - (menu_height / 2 - scroll_offset) &&
        y < pos_y + (menu_height / 2 - scroll_offset))
    {
      menuaction = MENU_ACTION_HIT;
    }
    break;
  }
  case SDL_MOUSEMOTION:
  {
    const int menu_width  = get_width();
    const int menu_height = get_height();
    const int scroll_offset = get_scroll_offset();

    x = event.motion.x;
    y = event.motion.y;
    if(x > pos_x - menu_width / 2 &&
        x < pos_x + menu_width / 2 &&
        y > pos_y - (menu_height / 2 - scroll_offset) &&
        y < pos_y + (menu_height / 2 - scroll_offset))
    {
      active_item = ((y - (pos_y - (menu_height / 2))) + scroll_offset) / 24;

      // Only set cursor to "Link" state when hovering over active items
      const MenuItem& new_item = item[active_item];
      mouse_cursor->set_state(new_item.is_active() ? MC_LINK : MC_NORMAL);
    }
    else
    {
      mouse_cursor->set_state(MC_NORMAL);
    }
    break;
  }
  default:
    break;
  }
}


// EOF //
