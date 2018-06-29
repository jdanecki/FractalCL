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

#include "gui.h"
#include "fractal_ocl.h"

const char* font_file = "FreeMono.ttf";
TTF_Font* font;

SDL_Renderer* main_window;
SDL_Texture* texture;

void init_font()
{
    char p[256];
    if (TTF_Init() == -1)
    {
        printf("TTF_Init: %s\n", TTF_GetError());
        exit(0);
    }

    sprintf(p, "%s/%s", STRING_MACRO(DATA_PATH), font_file);
    font = TTF_OpenFont(p, FONT_SIZE);
    if (font == NULL)
    {
        printf("TTF_OpenFont(%s) : %s\n", p, TTF_GetError());
        exit(0);
    }
}

void write_text(const char* t, int x, int y)
{
    SDL_Surface* temp;
    SDL_Texture* text_box;
    SDL_Rect dest;
    SDL_Color text_color;
    SDL_Color shade_color;

    text_color.r = 255;
    text_color.b = 255;
    text_color.g = 255;

    shade_color.r = 0;
    shade_color.g = 0;
    shade_color.b = 60;

    temp = TTF_RenderUTF8_Shaded(font, t, text_color, shade_color);

    if (!temp)
    {
        printf("TTF_RenderUTF8_Solid error\n");
        exit(0);
    }

    text_box = SDL_CreateTextureFromSurface(main_window, temp);
    SDL_FreeSurface(temp);

    dest.x = x;
    dest.y = y;
    dest.w = temp->w;
    dest.h = temp->h;

    SDL_RenderCopy(main_window, text_box, NULL, &dest);
    SDL_DestroyTexture(text_box);
}

int init_window()
{
    SDL_Window* app_window;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
#ifdef SDL_ACCELERATED
    app_window = SDL_CreateWindow("FractalCL", SDL_WINDOWPOS_CENTERED | SDL_WINDOW_OPENGL, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, HEIGHT, 0);
    main_window = SDL_CreateRenderer(app_window, -1, SDL_RENDERER_ACCELERATED);
#else
    app_window = SDL_CreateWindow("FractalCL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, HEIGHT, 0);
    main_window = SDL_CreateRenderer(app_window, -1, SDL_RENDERER_SOFTWARE);
#endif

    texture = SDL_CreateTexture(main_window, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    init_font();
    draw_box(WIDTH, 0, RIGTH_PANEL_WIDTH, HEIGHT, 0, 0, 60);
    return 0;
}

void draw_box(int x, int y, int w, int h, int r, int g, int b)
{
    SDL_Rect dst;
    dst.w = w;
    dst.h = h;
    dst.x = x;
    dst.y = y;
    SDL_SetRenderDrawColor(main_window, r, g, b, 255);
    SDL_RenderFillRect(main_window, &dst);
}

void draw_double(int y, char* txt, double val)
{
    char buf[256];
    sprintf(buf, "%s=%2.15f  ", txt, val);
    write_text(buf, WIDTH, FONT_SIZE * y + 2);
}

void draw_int(int y, char* txt, int val)
{
    char buf[256];
    sprintf(buf, "%s=%d  ", txt, val);
    write_text(buf, WIDTH, FONT_SIZE * y + 2);
}

void draw_long(int y, char* txt, unsigned long val)
{
    char buf[256];
    sprintf(buf, "%s=%lu  ", txt, val);
    write_text(buf, WIDTH, FONT_SIZE * y + 2);
}

void draw_2long(int y, char* txt1, unsigned long val1, char* txt2, unsigned long val2)
{
    char buf[256];
    sprintf(buf, "%s=%lu %s=%lu     ", txt1, val1, txt2, val2);
    write_text(buf, WIDTH, FONT_SIZE * y + 2);
}

void draw_hex(int y, char* txt, int val)
{
    char buf[256];
    sprintf(buf, "%s=%-8x", txt, val);
    write_text(buf, WIDTH, FONT_SIZE * y + 2);
}

void draw_string(int y, char* txt, char* val)
{
    char buf[256];
    sprintf(buf, "%s=%s", txt, val);
    write_text(buf, WIDTH, FONT_SIZE * y + 2);
}

unsigned long get_time_usec()
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return (t.tv_sec * 1000000 + t.tv_nsec / 1000);
}
