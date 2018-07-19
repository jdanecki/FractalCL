/*
    Copyright (C) 2018  Jacek Danecki <jacek.m.danecki@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL.h>
#include <SDL_ttf.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "window.h"

extern SDL_Renderer* main_window;
extern SDL_Texture* texture;

int init_window();
void write_text(const char* t, int x, int y);
unsigned long get_time_usec();
void draw_box(int x, int y, int w, int h, int r, int g, int b);
void clear_window();

void draw_double(int y, char* txt, double val);
void draw_int(int y, char* txt, int val);
void draw_string(int y, char* txt, char* val);
void draw_hex(int y, char* txt, int val);
void draw_long(int y, char* txt, unsigned long val);
void draw_2long(int y, char* txt1, unsigned long val1, char* txt2, unsigned long val2);
