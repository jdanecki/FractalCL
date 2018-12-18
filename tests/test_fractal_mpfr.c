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
#include <gmp.h>
#include <mpfr.h>
#include <pthread.h>

mpfr_t lx;
mpfr_t ty;
mpfr_t size;
unsigned int* pixels;

int max_iter = 1000;
int res = 256;
unsigned int m = 16;
#define THREADS 12

int hy = 32;
int h = HEIGHT / 2;

struct t_pars
{
    int tid;
    int y;
    int iter;
} t_par[THREADS];

struct cpu_args
{
    int max_iter;
    double max_iter_x, max_iter_y;
};

unsigned int get_iter(mpfr_t z_x, mpfr_t z_y, mpfr_t temp, mpfr_t d, mpfr_t j_x, mpfr_t j_y)
{
    unsigned int iter = 0;

    while (iter < max_iter)
    {
        mpfr_mul(j_x, z_x, z_x, MPFR_RNDD);
        mpfr_mul(temp, z_y, z_y, MPFR_RNDD);
        mpfr_sub(j_x, j_x, temp, MPFR_RNDD);
        mpfr_add_d(j_x, j_x, 0.15f, MPFR_RNDD);

        mpfr_mul(temp, z_x, z_y, MPFR_RNDD);
        mpfr_mul_d(j_y, temp, 2.0f, MPFR_RNDD);
        mpfr_sub_d(j_y, j_y, 0.6f, MPFR_RNDD);

        mpfr_mul(d, j_x, j_x, MPFR_RNDD);
        mpfr_mul(temp, j_y, j_y, MPFR_RNDD);
        mpfr_add(d, d, temp, MPFR_RNDD);

        if (mpfr_cmp_d(d, 2.0) > 0) break;

        mpfr_set(z_x, j_x, MPFR_RNDD);
        mpfr_set(z_y, j_y, MPFR_RNDD);
        iter++;
    }

    return iter;
}

void* execute_fractal(void* par)
{
    int x;
    mpfr_t z_x, z_y;
    mpfr_init2(z_x, res);
    mpfr_init2(z_y, res);

    mpfr_t temp;
    mpfr_init2(temp, res);

    mpfr_t j_x, j_y, l_z_y;
    mpfr_t d;

    mpfr_init2(j_x, res);
    mpfr_init2(j_y, res);
    mpfr_init2(d, res);
    mpfr_init2(l_z_y, res);

    int w = WIDTH / 2;

    struct t_pars* t = (struct t_pars*)par;

    mpfr_t stepy;
    mpfr_t stepx;

    mpfr_init_set_d(stepy, 1.0f, MPFR_RNDD);
    mpfr_init2(stepx, res);

    unsigned long int mult_y = hy * t->tid + t->y;
    mpfr_mul_ui(stepy, stepy, mult_y, MPFR_RNDD);
    mpfr_div_ui(stepy, stepy, h, MPFR_RNDD);

    mpfr_mul(temp, size, stepy, MPFR_RNDD);
    mpfr_sub(z_y, ty, temp, MPFR_RNDD);

    for (x = 0; x < w; x++)
    {
        unsigned int c = 0;

        mpfr_set_d(stepx, 1.0f, MPFR_RNDD);
        mpfr_mul_ui(stepx, stepx, x, MPFR_RNDD);
        mpfr_div_ui(stepx, stepx, w, MPFR_RNDD);

        mpfr_mul(temp, size, stepx, MPFR_RNDD);
        mpfr_add(z_x, lx, temp, MPFR_RNDD);

        mpfr_set(l_z_y, z_y, res);
        c = get_iter(z_x, l_z_y, temp, d, j_x, j_y);

        if (c < (max_iter - 1) && c > t->iter)
        {
            t->iter = c;
            //   args->max_iter_x = z_x;
            // args->max_iter_y = z_y;
        }
        pixels[(hy * t->tid + t->y) * WIDTH + x] = c * m;
    }
    t->y++;
    if (t->y == hy) t->y = 0;

    mpfr_clear(z_x);
    mpfr_clear(z_y);
    mpfr_clear(temp);

    mpfr_clear(j_x);
    mpfr_clear(j_y);
    mpfr_clear(l_z_y);
    mpfr_clear(d);
    mpfr_clear(stepx);
    mpfr_clear(stepy);

    return NULL;
}

int main()
{
    int i = 0; //, iter = 0;
    char status_line[200];
    unsigned long render_time = 0, render_times = 0;
    unsigned long exec_time = 0, exec_times = 0;
    unsigned long tp1, tp2, tp3, avg;
    int fin = 0;

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    //    double px = 0;
    //    double py = 0;
    //    double cx = lx + size / 2.0;
    //  double cy = ty - size / 2.0;

    //    int mx = 0, my = 0;
    int redraw = 1;
    //    int mouse_move;

    SDL_Rect window_rec;

    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    struct cpu_args args;

    memset(&args, 0, sizeof(args));
    mpfr_set_default_prec(res);

    mpfr_init_set_d(lx, -0.123661097601143424, MPFR_RNDD);
    mpfr_init_set_d(ty, 0.058635712827822206, MPFR_RNDD);
    mpfr_init_set_d(size, 0.00000000000022737, MPFR_RNDD);

    mpfr_t temp;
    mpfr_init2(temp, res);

    pthread_t tid[THREADS];

    for (int t = 0; t < THREADS; t++)
    {
        t_par[t].tid = t;
    }

    int loops = hy;

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
                loops = hy;

                switch (kl)
                {
                case 27:
                    fin = 1;
                    break;
                case '=':
                    mpfr_div_d(temp, size, 4.0f, MPFR_RNDD);
                    mpfr_add(lx, lx, temp, MPFR_RNDD);
                    mpfr_sub(ty, ty, temp, MPFR_RNDD);
                    mpfr_div_d(size, size, 2.0f, MPFR_RNDD);
                    break;
                case 'j':
                    /*                    lx += (args.max_iter_x - cx);
                                        ty += (args.max_iter_y - cy);*/
                    //                    size /= 2.0f;
                    break;
                case '-':
                    mpfr_div_d(temp, size, 2.0f, MPFR_RNDD);
                    mpfr_sub(lx, lx, temp, MPFR_RNDD);
                    mpfr_sub(ty, ty, temp, MPFR_RNDD);
                    mpfr_mul_d(size, size, 2.0f, MPFR_RNDD);
                    break;
                case 'a':
                    mpfr_div_d(temp, size, 4.0f, MPFR_RNDD);
                    mpfr_sub(lx, lx, temp, MPFR_RNDD);
                    break;
                case 'd':
                    mpfr_div_d(temp, size, 4.0f, MPFR_RNDD);
                    mpfr_add(lx, lx, temp, MPFR_RNDD);
                    break;
                case 'w':
                    mpfr_div_d(temp, size, 4.0f, MPFR_RNDD);
                    mpfr_add(ty, ty, temp, MPFR_RNDD);
                    break;
                case 's':
                    mpfr_div_d(temp, size, 4.0f, MPFR_RNDD);
                    mpfr_sub(ty, ty, temp, MPFR_RNDD);
                    break;
                case 'i':
                    max_iter += 10;
                    break;
                case 'o':
                    max_iter -= 10;
                    if (max_iter < 10) max_iter = 10;
                    break;
                case 'm':
                    m++;
                    break;
                case 'n':
                    m--;
                    if (m < 1) m = 1;
                    break;
                }
                // cx = lx + size / 2;
                // cy = ty - size / 2;
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                // mx = event.button.x;
                // my = event.button.y;
                redraw = 1;
                loops = hy;
                // mouse_move = 1;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                if (event.button.x > WIDTH) continue;

                //                px = lx + size * mx / WIDTH;
                //              py = ty - size * my / HEIGHT;

                if (event.button.button == 2)
                {
                    redraw = 1;
                    loops = hy;
                    // mouse_move = 1;
                }
                if (event.button.button == 3)
                {
                    redraw = 1;
                    loops = hy;
                    //      mouse_move = 1;
                }
            }
        }
        if (redraw)
        {
            loops--;
            if (!loops)
            {
                redraw = 0;
                loops = hy;
            }

            tp1 = get_time_usec();

            for (int t = 0; t < THREADS; t++)
            {
                t_par[t].iter = 0;
                pthread_create(&tid[t], NULL, execute_fractal, &t_par[t]);
            }
            for (int t = 0; t < THREADS; t++)
            {
                pthread_join(tid[t], NULL);
            }

            tp2 = get_time_usec();

            SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
            SDL_RenderCopy(main_window, texture, NULL, &window_rec);

            int ry = 0;

            /*         sprintf(status_line, "x:%1.20f ", px);
                     write_text(status_line, WIDTH, ry);
                     ry += FONT_SIZE;

                     sprintf(status_line, "y:%1.20f ", py);
                     write_text(status_line, WIDTH, ry);
                     ry += FONT_SIZE;
         */
            double s = mpfr_get_d(size, MPFR_RNDD);
            sprintf(status_line, "size:%0.17f ", s);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;
            /*
                        if (mouse_move)
                        {
                            px = lx + size * mx / WIDTH;
                            py = ty - size * my / HEIGHT;
                            iter = get_iter(px, py);
                            sprintf(status_line, "iter=%d ", iter);
                            write_text(status_line, WIDTH, ry);
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
            */
            int t_max_iter = 0;
            for (int t = 0; t < THREADS; t++)
            {
                if (t_par[t].iter > t_max_iter) t_max_iter = t_par[t].iter;
            }

            sprintf(status_line, "max:%d/%d ", t_max_iter, max_iter);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            avg = i ? render_times / i : 0;

            sprintf(status_line, "exec=%lu ", exec_time);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "render=%lu avg=%lu ", render_time, avg);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "loops=%d ", loops);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            double l = mpfr_get_d(lx, MPFR_RNDD);

            sprintf(status_line, "lx:%1.18f ", l);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            /*            sprintf(status_line, "rx:%1.20f ", lx + size);
                        write_text(status_line, WIDTH, ry);
                        ry += FONT_SIZE;

                        sprintf(status_line, "cx:%1.20f ", cx);
                        write_text(status_line, WIDTH, ry);
                        ry += FONT_SIZE;
            */
            double t = mpfr_get_d(ty, MPFR_RNDD);
            sprintf(status_line, "ty:%1.18f ", t);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            sprintf(status_line, "m:%d ", m);
            write_text(status_line, WIDTH, ry);
            ry += FONT_SIZE;

            /*            sprintf(status_line, "by:%1.20f ", ty - size);
                        write_text(status_line, WIDTH, ry);
                        ry += FONT_SIZE;

                        sprintf(status_line, "cy:%1.20f ", cy);
                        write_text(status_line, WIDTH, ry);
                        ry += FONT_SIZE;

                        sprintf(status_line, "zoom:%1.20f ", size < 1 ? 1 / size : size);
                        write_text(status_line, WIDTH, ry);
                        ry += FONT_SIZE;

                        int fx = (args.max_iter_x - lx) * WIDTH / size;
                        int fy = -(args.max_iter_y - ty) * HEIGHT / size;

                        SDL_SetRenderDrawColor(main_window, 255, 255, 255, SDL_ALPHA_OPAQUE);
                        SDL_RenderDrawLine(main_window, 0, 0, fx, fy);
            */
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
    mpfr_clear(lx);
    mpfr_clear(ty);
    mpfr_clear(size);
    mpfr_clear(temp);

    return 0;
}
