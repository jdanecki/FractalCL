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

const char* font_file = "FreeMono.ttf";
TTF_Font* font;

SDL_Renderer* main_window;
SDL_Texture* texture;

void init_font()
{
    if (TTF_Init() == -1)
    {
        printf("TTF_Init: %s\n", TTF_GetError());
        exit(0);
    }

    font = TTF_OpenFont(font_file, FONT_SIZE);
    if (font == NULL)
    {
        printf("TTF_OpenFont(%s) : %s\n", font_file, TTF_GetError());
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
    shade_color.b = 0;

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
    app_window = SDL_CreateWindow("FractalCL", SDL_WINDOWPOS_CENTERED | SDL_WINDOW_OPENGL, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    main_window = SDL_CreateRenderer(app_window, -1, SDL_RENDERER_ACCELERATED);
#else
    app_window = SDL_CreateWindow("FractalCL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    main_window = SDL_CreateRenderer(app_window, -1, SDL_RENDERER_SOFTWARE);
#endif

    texture = SDL_CreateTexture(main_window, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);
    init_font();
    return 0;
}

unsigned long get_time_usec()
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return (t.tv_sec * 1000000 + t.tv_nsec / 1000);
}
