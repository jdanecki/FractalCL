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

double lx = -1.0f;
double ty = 1.0f;
double size = 2.0f;
unsigned int* pixels;

int max_iter = 255;

#define THREADS 4
#define USE_PTHREADS

struct cpu_args
{
    int xs, xe, ys, ye, max_iter;
    double max_iter_x, max_iter_y;
};

unsigned int get_iter(double z_x, double z_y)
{
    unsigned int iter = 0;
    double j_x, j_y;
    double er = 4;
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
    args->max_iter = 0;
    for (y = args->ys; y < args->ye; y++)
    {
        for (x = args->xs; x < args->xe; x++)
        {
            double z_x, z_y;
            unsigned int c = 0;

            z_x = lx + size * x / WIDTH;
            z_y = ty - size * y / HEIGHT;

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

    struct cpu_args args[THREADS];

    memset(args, 0, sizeof(args));

    args[0].xe = WIDTH / 2;
    args[0].ye = HEIGHT / 2;

    args[1].xs = WIDTH / 2;
    args[1].xe = WIDTH;
    args[1].ye = HEIGHT / 2;

    args[2].xe = WIDTH / 2;
    args[2].ys = HEIGHT / 2;
    args[2].ye = HEIGHT;

    args[3].xs = WIDTH / 2;
    args[3].xe = WIDTH;
    args[3].ys = HEIGHT / 2;
    args[3].ye = HEIGHT;

    pthread_t tid[THREADS];
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
                    lx = (lx - cx) / 2.0 + cx;
                    ty = (ty - cy) / 2.0 + cy;
                    size /= 2.0f;
                    break;
                case '-':
                    lx = (lx - cx) * 2.0 + cx;
                    ty = (ty - cy) * 2.0 + cy;
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

#ifdef USE_PTHREADS
            for (int t = 0; t < THREADS; t++)
            {
                pthread_create(&tid[t], NULL, execute_fractal, &args[t]);
            }
            for (int t = 0; t < THREADS; t++)
            {
                pthread_join(tid[t], NULL);
            }
#else
            for (int t = 0; t < THREADS; t++) execute_fractal(&args[t]);
#endif

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

                for (int t = 0; t < THREADS; t++)
                {
                    sprintf(status_line, "max[%d]:%d/%d ", t, args[t].max_iter, max_iter);
                    write_text(status_line, WIDTH, ry);
                    printf(status_line);
                    ry += FONT_SIZE;

                    sprintf(status_line, "%1.20f ", args[t].max_iter_x);
                    write_text(status_line, WIDTH, ry);
                    printf(status_line);
                    ry += FONT_SIZE;

                    sprintf(status_line, "%1.20f ", args[t].max_iter_y);
                    write_text(status_line, WIDTH, ry);
                    printf("%s\n", status_line);
                    ry += FONT_SIZE;
                }
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

            SDL_SetRenderDrawColor(main_window, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawLine(main_window, 0, HEIGHT / 2, WIDTH, HEIGHT / 2);
            SDL_RenderDrawLine(main_window, WIDTH / 2, 0, WIDTH / 2, HEIGHT);

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
