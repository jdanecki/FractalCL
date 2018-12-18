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
#include <pthread.h>

double lx = 0.61983116712498886880;
double ty = 0.69204938421816364347;
double size = 1.0f;
unsigned int* pixels;

int max_iter = 1000;

struct cpu_args
{
    int max_iter;
    double max_iter_x, max_iter_y;
};

unsigned int get_iter(double z_x, double z_y)
{
    unsigned int iter = 0;
    double j_x, j_y;
    double er = 4.0f;
    double d;

    while (iter < max_iter)
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

void* execute_fractal(void* p)
{
    int x, y;
    struct cpu_args* args = (struct cpu_args*)p;
    //    args->max_iter = 0;
    for (y = 0; y < HEIGHT; y++)
    {
        double stepy = 1.0f * y / HEIGHT;
        for (x = 0; x < WIDTH; x++)
        {
            double z_x, z_y;
            unsigned int c = 0;
            double stepx = 1.0f * x / WIDTH;
            if (size > 0.0000000000001)
            {
                z_x = lx + size * stepx;
                z_y = ty - size * stepy;
                if (!y && x < 2) printf("[%d]: z_x=%0.40f z_y=%0.40f\n", x, z_x, z_y);
            }
            else
            {
                double mult = 2 * pow(10, 10);
                z_x = (mult * lx + size * mult * stepx) / mult;
                z_y = (mult * ty - size * mult * stepy) / mult;
                if (!y && x < 2) printf("[%d]:*z_x=%0.40f z_y=%0.40f\n", x, z_x, z_y);
            }

            c = get_iter(z_x, z_y);
            if (c < (max_iter - 1) && c > args->max_iter)
            {
                args->max_iter = c;
                args->max_iter_x = z_x;
                args->max_iter_y = z_y;
            }
            pixels[y * WIDTH + x] = c;
        }
    }
    return NULL;
}

int main()
{
    int i = 0, iter = 0;
    char status_line[200];
    unsigned long render_time = 0, render_times = 0;
    unsigned long exec_time = 0, exec_times = 0;
    unsigned long tp1, tp2, tp3, avg;
    int fin = 0;

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    double px = 0;
    double py = 0;
    double cx = lx + size / 2.0;
    double cy = ty - size / 2.0;

    int mx = 0, my = 0;
    int redraw = 1;
    int mouse_move;

    SDL_Rect window_rec;

    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    struct cpu_args args;

    memset(&args, 0, sizeof(args));

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

                redraw = 1;

                switch (kl)
                {
                case 27:
                    fin = 1;
                    break;
                case '=':
                    lx += size / 4.0f;
                    ty -= size / 4.0f;
                    size /= 2.0f;
                    break;
                case 'j':
                    lx += (args.max_iter_x - cx);
                    ty += (args.max_iter_y - cy);
                    //                    size /= 2.0f;
                    break;
                case '-':
                    lx -= size / 2.0f;
                    ty += size / 2.0f;
                    size *= 2.0f;
                    break;
                case 'a':
                    lx -= size / 4.0f;
                    break;
                case 'd':
                    lx += size / 4.0f;
                    break;
                case 'w':
                    ty += size / 4.0f;
                    break;
                case 's':
                    ty -= size / 4.0f;
                    break;
                case 'i':
                    max_iter += 10;
                    break;
                case 'o':
                    max_iter -= 10;
                    if (max_iter < 10) max_iter = 10;
                    break;
                }
                cx = lx + size / 2;
                cy = ty - size / 2;
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                mx = event.button.x;
                my = event.button.y;
                redraw = 1;
                mouse_move = 1;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                if (event.button.x > WIDTH) continue;

                px = lx + size * mx / WIDTH;
                py = ty - size * my / HEIGHT;

                if (event.button.button == 2)
                {
                    redraw = 1;
                    mouse_move = 1;
                }
                if (event.button.button == 3)
                {
                    redraw = 1;
                    mouse_move = 1;
                }
            }
        }
        if (redraw)
        {
            redraw = 0;

            tp1 = get_time_usec();

            execute_fractal(&args);

            tp2 = get_time_usec();

            SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
            SDL_RenderCopy(main_window, texture, NULL, &window_rec);

            int ry = 0;

            sprintf(status_line, "x:%1.20f ", px);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "y:%1.20f ", py);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "size:%f ", size);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            if (mouse_move)
            {
                px = lx + size * mx / WIDTH;
                py = ty - size * my / HEIGHT;
                iter = get_iter(px, py);
                sprintf(status_line, "iter=%d ", iter);
                write_text(status_line, WIDTH, ry);
                ry += FONT_SIZE;

                sprintf(status_line, "max:%d/%d ", args.max_iter, max_iter);
                write_text(status_line, WIDTH, ry);
                //                    printf(status_line);
                ry += FONT_SIZE;

                sprintf(status_line, "%1.20f ", args.max_iter_x);
                write_text(status_line, WIDTH, ry);
                //                    printf(status_line);
                ry += FONT_SIZE;

                sprintf(status_line, "%1.20f ", args.max_iter_y);
                write_text(status_line, WIDTH, ry);
                //                    printf("%s\n", status_line);
                ry += FONT_SIZE;
            }
            avg = i ? render_times / i : 0;

            sprintf(status_line, "exec=%lu ", exec_time);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "render=%lu avg=%lu ", render_time, avg);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "lx:%1.20f ", lx);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "rx:%1.20f ", lx + size);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "cx:%1.20f ", cx);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "ty:%1.20f ", ty);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "by:%1.20f ", ty - size);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "cy:%1.20f ", cy);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "size:%1.20f ", size);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "zoom:%1.20f ", size < 1 ? 1 / size : size);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            int fx = (args.max_iter_x - lx) * WIDTH / size;
            int fy = -(args.max_iter_y - ty) * HEIGHT / size;

            SDL_SetRenderDrawColor(main_window, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawLine(main_window, 0, 0, fx, fy);

            SDL_RenderPresent(main_window);

            tp3 = get_time_usec();
            exec_time = tp2 - tp1;
            exec_times += exec_time;

            render_time = tp3 - tp2;
            render_times += render_time;

            i++;
        }
    }
    SDL_Quit();

    return 0;
}
