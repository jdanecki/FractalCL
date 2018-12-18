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

#define ITERATIONS 3000

int main()
{
    unsigned int* pixels;
    int x, y, i = 0, r, g, b;
    char status_line[200];
    unsigned long render_time = 0, render_times = 0, avg, fps;
    unsigned long tp1, tp2;
    int fin = 0;

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    for (y = 0; y < HEIGHT; y++)
    {
        for (x = 0; x < WIDTH; x++)
        {
            r = 0x80 + 0x7f * sin(6.28 * x / WIDTH);
            g = 0x80 + 0x7f * sin(6.28 * y / HEIGHT);
            b = 0x80 + 0x7f * cos(6.28 * (x + y) / (WIDTH + HEIGHT));

            pixels[y * WIDTH + x] = r << 16 | g << 8 | b;
        }
    }
    int mx = 0, my = 0;

    SDL_Rect window_rec;

    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    while (!fin)
    {
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
            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                mx = event.button.x;
                my = event.button.y;
            }
        }
        tp1 = get_time_usec();
        /*
                for (y = 0 ; y < 20; y++)  {
                    for (x = 0 ; x < 20; x++) {
                        unsigned int ofs = (my - 10 + y) * WIDTH + (mx - 10 + x);
                        if (ofs > 0 && ofs < WIDTH *HEIGHT)
                            pixels[ofs]++;
                    }
                }
        */
        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
        //        SDL_RenderCopy(main_window, texture, NULL, NULL);
        SDL_RenderCopy(main_window, texture, NULL, &window_rec);

        avg = i ? render_times / i : 0;
        fps = avg ? 1000000 / avg : 0;
        sprintf(status_line, "test SDL[%d,%d]=%x %d render time=%lu avg=%lu fps=%lu", mx, my, pixels[my * WIDTH + mx], i, render_time, avg, fps);
        write_text(status_line, 0, HEIGHT / 2);
        SDL_RenderPresent(main_window);
        tp2 = get_time_usec();
        render_time = tp2 - tp1;
        render_times += render_time;
        i++;
    }
    fps = 1000000 / avg;
    printf("avg time=%lu fps=%lu\n", render_times / i, fps);
    SDL_Quit();

    return 0;
}
