//  $Id$
//
//  SuperTux -  A Jump'n Run
//  Copyright (C) 2004 Ricardo Cruz <rick2@aeiou.pt>
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

#include "screen.h"
#include "mousecursor.h"

MouseCursor* MouseCursor::current_ = 0;

void MouseCursor::set_current(MouseCursor* pcursor)
{
  current_ = pcursor;
  if (pcursor)
    pcursor->update_cursor();
  else
    SDL_ShowCursor(SDL_DISABLE);
}


MouseCursor::MouseCursor(const std::string& cursor_file, int frames, int mid_x_, int mid_y_) :
  frame_w(0),
  frame_h(0),
  mid_x(mid_x_),
  mid_y(mid_y_)
{
  cur_state = MC_NORMAL;
  cur_frame = 0;
  tot_frames = frames;

  Surface surface(cursor_file, USE_ALPHA);

  const_cast<int&>(frame_w) = surface.w / tot_frames;
  const_cast<int&>(frame_h) = surface.h / MC_STATES_NB;

  for (int y = 0; y < MC_STATES_NB; ++y)
  {
    cursor_states[y] = std::vector<CursorData>(tot_frames, CursorData());
    std::vector<CursorData>& cursors = cursor_states[y];
    for (int x = 0; x < tot_frames; ++x)
    {
      CursorData& cursor = cursors[x];
      cursor.surface = new Surface(surface.impl->get_sdl_surface(), x * frame_w, y * frame_h, frame_w, frame_h, USE_ALPHA);
      cursor.cursor = SDL_CreateColorCursor(cursor.surface->impl->get_sdl_surface(), mid_x, mid_y);
      if (!cursor.cursor)
        printf("SDL_CreateColorCursor() failed: %s\n", SDL_GetError());
    }
  }

  timer.init(false);
  timer.start(MC_FRAME_PERIOD);
}

MouseCursor::~MouseCursor()
{
  if (current_ == this)
  {
    current_ = nullptr;
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetCursor(NULL);
  }

  for (auto& cursors : cursor_states)
  {
    for (CursorData& cursor : cursors)
    {
      SDL_FreeCursor(cursor.cursor);
      delete cursor.surface;
    }
  }
}

void MouseCursor::update_cursor()
{
  if (current_ != this)
    return;

  SDL_SetCursor(cursor_states[cur_state][cur_frame].cursor);

  // Force cursor refresh
  SDL_ShowCursor(SDL_DISABLE);
  SDL_ShowCursor(SDL_ENABLE);
}

void MouseCursor::set_state(int nstate)
{
  if (nstate == cur_state) return;
  cur_state = nstate;

  update_cursor();
}

void MouseCursor::update()
{
  int x, y;
  const int pressed = SDL_GetMouseState(&x, &y);
  if(pressed &SDL_BUTTON(1) || pressed &SDL_BUTTON(2))
  {
    if(cur_state != MC_CLICK)
    {
      state_before_click = cur_state;
      cur_state = MC_CLICK;

      update_cursor();
    }
  }
  else if(cur_state == MC_CLICK)
  {
    cur_state = state_before_click;

    update_cursor();
  }

  if(timer.get_left() < 0 && tot_frames > 1)
  {
    if(cur_frame++ >= tot_frames)
      cur_frame = 0;

    update_cursor();

    timer.start(MC_FRAME_PERIOD);
  }
}
