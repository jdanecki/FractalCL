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
double lx = -1.0f;
double ty = 1.0f;
double size_x = 2.0f;
double size_y = 2.0f;

unsigned int get_iter(double z_x, double z_y)
{
    unsigned int iter = 0;
    double j_x, j_y;
    double er = 4;
    double d;

    while (iter < 300)
    {
        j_x = z_x * z_x - z_y * z_y + 0.15f;
        j_y = 2.0f * z_x * z_y - 0.6f;

        d = (j_x * j_x + j_y * j_y);
        if (d > er) break;

        z_x = j_x;
        z_y = j_y;
        iter++;
    }
    return iter;
}

int main()
{
    unsigned int* pixels;
    int x, y, i, iter = 0;
    char status_line[200];
    unsigned long render_time = 0, render_times = 0, avg, fps;
    unsigned long tp1, tp2;
    int fin = 0;

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    double px = 0;
    double py = 0;

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
                case '=':
                    lx /= 2;
                    size_x /= 2.0f;
                    size_y /= 2.0f;
                    break;
                case '-':
                    lx *= 2.0f;
                    size_y *= 2.0f;
                    break;
                case 'a':
                    lx -= size_x / 3.0f;
                    break;
                case 'd':
                    lx += size_x / 3.0f;
                    break;
                case 'w':
                    ty -= size_y / 3.0f;
                    break;
                case 's':
                    ty += size_y / 3.0f;
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

        for (y = 0; y < HEIGHT; y++)
        {
            for (x = 0; x < WIDTH; x++)
            {
                double z_x, z_y;
                unsigned int c = 0;

                z_x = lx + size_x * x / WIDTH;
                z_y = ty - size_y * y / HEIGHT;

                c = get_iter(z_x, z_y);
                pixels[y * WIDTH + x] = c;
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
        //        SDL_RenderCopy(main_window, texture, NULL, NULL);
        SDL_RenderCopy(main_window, texture, NULL, &window_rec);

        avg = i ? render_times / i : 0;
        fps = avg ? 1000000 / avg : 0;

        sprintf(status_line, "x:%1.20f", px);
        write_text(status_line, WIDTH, 0);

        sprintf(status_line, "y:%1.20f", py);
        write_text(status_line, WIDTH, 20);

        sprintf(status_line, "%1.20f", size_x);
        write_text(status_line, WIDTH, 40);

        sprintf(status_line, "%1.20f", size_y);
        write_text(status_line, WIDTH, 60);

        px = lx + size_x * mx / WIDTH;
        py = ty - size_y * my / HEIGHT;
        iter = get_iter(px, py);

        sprintf(status_line, "iter=%d ", iter);
        write_text(status_line, WIDTH, 80);

        //		sprintf(status_line, "render=%lu avg=%lu ", render_time, avg);
        //      write_text(status_line, WIDTH, 20);

        //		sprintf(status_line, "fps=%lu ", fps);
        //      write_text(status_line, WIDTH, 40);

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
