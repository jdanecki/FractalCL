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

#include "parameters.h"
#include "window.h"
#include <SDL.h>

FP_TYPE zx = 1.0, zy = 1.0; // zoom x, y
FP_TYPE zoom = 1.0f;
FP_TYPE dx, dy;               // shift left/right, down/up
FP_TYPE szx = 1.0, szy = 1.0; // scale x and y
FP_TYPE mx, my;               // mouse coordinates between [ofs_lx..ofs_rx, ofs_ty..ofs_by]

unsigned int rgb;
unsigned int mm = 1;
int postprocess;

int gws_x = WIDTH / 4;
int gws_y = HEIGHT / 4;

#if 0
FP_TYPE ofs_lx = -0.7402f;//-1.5f; //0.094; //-1.5f;
FP_TYPE ofs_rx = -0.7164f; //-0.716403707741993;//1.5f; //0.096; //1.5f;
FP_TYPE ofs_ty = 0.2205f;//0.283962759342063; //1.5f; //0.504f; //1.5f;
FP_TYPE ofs_by = 0.1690f;//0.283962759341958; //-1.5f; //0.503f; //-1.5f;
#else

FP_TYPE ofs_lx = OFS_LX;
FP_TYPE ofs_rx = OFS_RX;
FP_TYPE ofs_ty = 1.5f;
FP_TYPE ofs_by = -1.5f;
FP_TYPE lx;
FP_TYPE rx;
FP_TYPE ty;
FP_TYPE by;
#endif
FP_TYPE er = 4.0f;
float c1[3] = {0.5f, 0.5f, 0.5f};
float c2[3] = {0.5f, 0.5f, 0.5f};
float c3[3] = {1.0f, 1.0f, 1.0f};
float c4[3] = {0.0f, 0.33f, 0.66f};
int mod1;
unsigned int max_iter = 360;
int pal;     // 0=hsv 1,...=rgb
int palette; // 1 - show palette
FP_TYPE c_x = 0.15f;
FP_TYPE c_y = -0.60f;
// FP_TYPE c_x = -0.7f;
// FP_TYPE c_y = 0.27015f;
int cur_dev;
enum fractals fractal = JULIA;
unsigned long long iter_limit = 43000000000000LL;

unsigned long render_time;
unsigned long render_times;
unsigned long flips;
unsigned long cpu_execution;
unsigned long cpu_executions, gpu_executions;
unsigned long cpu_iter, gpu_iter;
int color_channel; // r, g, b
extern int quiet;
extern void draw_box(int x, int y, int w, int h, int r, int g, int b);

#ifdef OPENCL_SUPPORT
extern void clear_pixels_ocl();
#endif

int calculate_offsets()
{
    FP_TYPE d;

    ofs_lx = (ofs_lx - mx) * zx + mx;
    ofs_rx = (ofs_rx - mx) * zx + mx;

    ofs_ty = (ofs_ty - my) * zy + my;
    ofs_by = (ofs_by - my) * zy + my;

    d = (ofs_rx - ofs_lx);
    if (d)
    {
        zoom = (OFS_RX - OFS_LX) / d;
    }

    if (ofs_lx < -10 || ofs_rx > 10 || ofs_ty > 10 || ofs_by < -10 || zoom > iter_limit)
    {
        return 1;
    }
    else
        return 0;
}

void clear_counters()
{
    cpu_iter = 0;
    cpu_executions = 0;
    gpu_iter = 0;
    gpu_executions = 0;
    render_times = 0;
    flips = 0;
}

void init_parameters()
{
    ofs_lx = OFS_LX;
    ofs_rx = OFS_RX;
    ofs_ty = 1.5f;
    ofs_by = -1.5f;
    zoom = 1.0f;
    dx = 0;
    dy = 0;
    zx = 1.0f;
    zy = 1.0f;
    szx = 1.0f;
    szy = 1.0f;
    er = 4.0f;
    max_iter = 360;
    mod1 = 0;
}

void set_fractal(enum fractals f, int d)
{
    if (!quiet) printf("select fractal: %d\n", f);
    fractal = f;
    gws_x = WIDTH / d;
    gws_y = HEIGHT / d;
    init_parameters();
    clear_counters();
}

void select_fractal(int f)
{
    switch (f)
    {
    case JULIA:
        set_fractal(JULIA, 4);
        break;
    case MANDELBROT:
        set_fractal(MANDELBROT, 4);
        break;
    case JULIA_FULL:
        set_fractal(JULIA_FULL, 1);
        break;
    case DRAGON:
        set_fractal(DRAGON, 1);
        max_iter = 10000;
        er = 0.9f;
        ofs_lx = 0.0f;
        ofs_ty = 0.0f;
        clear_counters();
        break;
    case JULIA3:
        set_fractal(JULIA3, 4);
        break;
    case BURNING_SHIP:
        set_fractal(BURNING_SHIP, 4);
        break;
    case GENERALIZED_CELTIC:
        set_fractal(GENERALIZED_CELTIC, 4);
        break;
    case TRICORN:
        set_fractal(TRICORN, 4);
        break;
    }
}

void select_fractals(int k)
{
    switch (k)
    {
    case SDLK_F1:
        select_fractal(JULIA);
        break;
    case SDLK_F2:
        select_fractal(MANDELBROT);
        break;
    case SDLK_F3:
        select_fractal(JULIA_FULL);
        break;
    case SDLK_F4:
        select_fractal(DRAGON);
#ifdef OPENCL_SUPPORT
        clear_pixels_ocl();
#endif
        break;
    case SDLK_F5:
        select_fractal(JULIA3);
        break;
    case SDLK_F6:
        select_fractal(BURNING_SHIP);
        break;
    case SDLK_F7:
        select_fractal(GENERALIZED_CELTIC);
        break;
    case SDLK_F8:
        select_fractal(TRICORN);
        break;
    }
}

void dec_int(unsigned int* v, unsigned int by, unsigned int min, int clear)
{
    *v -= by;
    if (*v < min) *v = min;
    if (clear) clear_counters();
}

void inc_int(unsigned int* v, unsigned int by, int clear)
{
    *v += by;
    if (clear) clear_counters();
}

void dec_fp_type(FP_TYPE* v, FP_TYPE by, FP_TYPE min, int clear)
{
    *v -= by;
    if (*v < min) *v = min;
    if (clear) clear_counters();
}

void inc_fp_type(FP_TYPE* v, FP_TYPE by, int clear)
{
    *v += by;
    if (clear) clear_counters();
}

void dec_float(float* v, float by, float min, int clear)
{
    *v -= by;
    if (*v < min) *v = min;
    if (clear) clear_counters();
}

void inc_float(float* v, float by, int clear)
{
    *v += by;
    if (clear) clear_counters();
}

void change_fractal_params(int kl, int mod_kl)
{
    int mod = 0;
    if (mod_kl == 1 || mod_kl == 2) mod = 1;
    switch (kl)
    {
    case 'i':
        if (mod)
            dec_int(&max_iter, 1, 1, 1);
        else
            inc_int(&max_iter, 1, 1);
        break;
    case 'e':
        if (mod)
            dec_fp_type(&er, 0.1, 0.1, 1);
        else
            inc_fp_type(&er, 0.1, 1);
        break;
        /*    case '2':
                gws_x *= 2;
                if (gws_x > WIDTH) gws_x = WIDTH;
                gws_y *= 2;
                if (gws_y > HEIGHT) gws_y = HEIGHT;
                printf("gws: x=%d y=%d\n", gws_x, gws_y);
                clear_counters();
                break;
            case '3':
                gws_x /= 2;
                if (gws_x < 8) gws_x = 8;
                gws_y /= 2;
                if (gws_y < 8) gws_y = 8;
                printf("gws: x=%d y=%d\n", gws_x, gws_y);
                clear_counters();
                break;*/
    }
}

void change_fractal_colors(int kl, int mod_kl)
{
    int mod = 0;
    if (mod_kl == 1 || mod_kl == 2) mod = 1;

    switch (kl)
    {
    case 'c':
        if (mod)
        {
            color_channel--;
            if (color_channel < 0) color_channel = 0;
        }
        else
        {
            color_channel++;
            if (color_channel > 2) color_channel = 2;
        }
        break;
    case '=':
        if (mod)
            rgb += 1 << (color_channel * 8);
        else
            rgb -= 1 << (color_channel * 8);
        break;
    case '-':
        if (mod)
            rgb += 16 << (color_channel * 8);
        else
            rgb -= 16 << (color_channel * 8);
        break;
    case 'm':
        if (mod)
            dec_int(&mm, 1, 1, 1);
        else
            inc_int(&mm, 1, 1);
        break;
    case 'p':
        if (mod)
        {
            palette ^= 1;
            if (palette) rgb = 0;
        }
        else
        {
            pal++;
            if (pal == 3) pal = 0;
            draw_box(WIDTH, 0, RIGTH_PANEL_WIDTH, HEIGHT, 0, 0, 60);
        }
        break;
    case 'h':
        if (mod)
            dec_float(&c1[color_channel], 0.1, 0.0, 0);
        else
            inc_float(&c1[color_channel], 0.1, 0);
        break;
    case 'j':
        if (mod)
            dec_float(&c2[color_channel], 0.1, 0.0, 0);
        else
            inc_float(&c2[color_channel], 0.1, 0);
        break;
    case 'k':
        if (mod)
            dec_float(&c3[color_channel], 0.1, 0.0, 0);
        else
            inc_float(&c3[color_channel], 0.1, 0);
        break;
    case 'l':
        if (mod)
            dec_float(&c4[color_channel], 0.1, 0.0, 0);
        else
            inc_float(&c4[color_channel], 0.1, 0);
        break;
    }
}

int move_fractal(int kl, int mod_kl)
{
    int mod = 0;
    if (mod_kl == 1 || mod_kl == 2) mod = 1;
    int key = 0;

    switch (kl)
    {
    case SDLK_LEFT:
        dec_fp_type(&szx, 0.01 / zoom, 0.1, 1);
        key = 1;
        break;
    case SDLK_RIGHT:
        inc_fp_type(&szx, 0.01 / zoom, 1);
        key = 1;
        break;
    case SDLK_DOWN:
        dec_fp_type(&szy, 0.01 / zoom, 0.1, 1);
        key = 1;
        break;
    case SDLK_UP:
        inc_fp_type(&szy, 0.01 / zoom, 1);
        key = 1;
        break;
    case 'a':
        inc_fp_type(&dx, -0.1 / zoom, 0);
        key = 1;
        break;
    case 'd':
        inc_fp_type(&dx, 0.1 / zoom, 0);
        key = 1;
        break;
    case 's':
        inc_fp_type(&dy, -0.1 / zoom, 0);
        key = 1;
        break;
    case 'w':
        inc_fp_type(&dy, 0.1 / zoom, 0);
        key = 1;
        break;
    case '8':
        if (mod)
        {
            inc_fp_type(&szx, 1.0, 0);
            inc_fp_type(&szy, 1.0, 1);
            key = 1;
        }
        break;
    case '/':
        dec_fp_type(&szx, 1.0, 1.0, 0);
        dec_fp_type(&szy, 1.0, 1.0, 0);
        key = 1;
        break;
    case 'x':
        if (mod)
            c_x -= 0.001;
        else
            c_x += 0.001;
        break;
    case 'y':
        if (mod)
            c_y -= 0.001;
        else
            c_y += 0.001;
        break;
    }
    return key;
}
