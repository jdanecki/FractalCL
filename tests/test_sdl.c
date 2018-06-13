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

#define ITERATIONS 3000

int main()
{
    unsigned int* pixels;
    int x, y, i;
    char status_line[200];
    unsigned long render_time = 0, render_times = 0, avg, fps;
    unsigned long tp1, tp2;

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    for (i = 0; i < ITERATIONS; i++)
    {
        tp1 = get_time_usec();
        for (y = 0; y < HEIGHT; y++)
        {
            for (x = 0; x < WIDTH; x++)
            {
                pixels[y * WIDTH + x] = 0xff000000 | (y * WIDTH + x);
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
        SDL_RenderCopy(main_window, texture, NULL, NULL);

        avg = i ? render_times / i : 0;
        fps = avg ? 1000000 / avg : 0;
        sprintf(status_line, "test SDL: %d/%d render time=%lu avg=%lu fps=%lu", i, ITERATIONS, render_time, avg, fps);
        write_text(status_line, 0, HEIGHT / 2);
        SDL_RenderPresent(main_window);
        tp2 = get_time_usec();
        render_time = tp2 - tp1;
        render_times += render_time;
    }
    fps = 1000000 / avg;
    printf("avg time=%lu fps=%lu\n", render_times / i, fps);
    SDL_Quit();

    return 0;
}
