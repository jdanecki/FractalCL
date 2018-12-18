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
#include "timer.h"

#include <SDL2_framerate.h>
#include <SDL2_gfxPrimitives.h>

#define ITERATIONS 10000
#define SIZE 10

struct circle_data
{
    int x, y, dir_x, dir_y;
    unsigned int color;
} circles[ITERATIONS];

void draw_circle(int i)
{
    int x = circles[i].x;
    int y = circles[i].y;
    int dir_x = circles[i].dir_x;
    int dir_y = circles[i].dir_y;

    if (!x)
    {
        x = random() % (WIDTH - SIZE);
        dir_x = -1 - random() % 3;
        if (!dir_x) dir_x = 1;
        circles[i].color = random();
    }
    if (!y)
    {
        y = random() % (HEIGHT - SIZE);
        dir_y = -1 - random() % 3;
        if (!dir_y) dir_y = -dir_x;
    }

    filledCircleColor(main_window, x, y, SIZE, circles[i].color);
    x += dir_x;
    y += dir_y;

    if (x > WIDTH - SIZE) dir_x *= -1;
    if (x < SIZE) dir_x *= -1;
    if (y > HEIGHT - SIZE) dir_y *= -1;
    if (y < SIZE) dir_y *= -1;

    circles[i].x = x;
    circles[i].y = y;
    circles[i].dir_x = dir_x;
    circles[i].dir_y = dir_y;
}

int main()
{
    unsigned int* pixels;
    int i, c;
    char status_line[200];
    unsigned long render_time = 0, render_times = 0, avg, fps;
    unsigned long tp1, tp2;
    FPSmanager fps_manager;

    srandom(time(0));

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    SDL_initFramerate(&fps_manager);
    SDL_setFramerate(&fps_manager, 200);

    int fin = 0;
    //    int mx = 0, my =0;

    for (i = 0; i < ITERATIONS; i++)
    {
        if (fin) break;

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                fin = 1;
                break;
            }
            if (event.type == SDL_KEYDOWN)
            {
                int kl = event.key.keysym.sym;
                switch (kl)
                {
                case 27:
                    fin = 1;
                    break;
                }
            }
            /*				if (event.type == SDL_MOUSEMOTION)
                            {
                                if (event.button.x > WIDTH) continue;
                                mx = event.button.x;
                                my = event.button.y;
                            }*/
        }

        tp1 = get_time_usec();
        clear_window();
        for (c = 0; c < i; c++) draw_circle(c);

        avg = i ? render_times / i : 0;
        fps = avg ? 1000000 / avg : 0;
        sprintf(status_line, "test SDL: %d/%d render time=%lu avg=%lu fps=%lu/%3d ", i, ITERATIONS, render_time, avg, fps, SDL_getFramerate(&fps_manager));
        write_text(status_line, 0, HEIGHT / 2);
        SDL_RenderPresent(main_window);
        SDL_framerateDelay(&fps_manager);

        tp2 = get_time_usec();
        render_time = tp2 - tp1;
        render_times += render_time;
    }
    fps = 1000000 / avg;
    printf("avg time=%lu fps=%lu\n", render_times / i, fps);
    SDL_Quit();

    return 0;
}
