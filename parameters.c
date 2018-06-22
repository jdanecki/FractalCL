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

FP_TYPE zx = 1.0, zy = 1.0; // zoom x, y
FP_TYPE zoom = 1.0f;
FP_TYPE dx, dy;               // shift left/right, down/up
FP_TYPE szx = 1.0, szy = 1.0; // scale x and y
FP_TYPE mx, my;               // mouse coordinates between [ofs_lx..ofs_rx, ofs_ty..ofs_by]

unsigned int mm = 16;
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

unsigned int max_iter = 360;
int pal;     // 0=hsv 1=rgb
int palette; // 1 - show palette
int show_z;
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
unsigned long frames_time;
unsigned long cpu_execution;
unsigned long cpu_executions, gpu_executions;
unsigned long cpu_iter, gpu_iter;
int color_channel; // r, g, b

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

void set_fractal(enum fractals f, int d)
{
    fractal = f;
    gws_x = WIDTH / d;
    gws_y = HEIGHT / d;
    max_iter = 360;
    er = 4.0f;
    ofs_lx = -1.5f;
    ofs_ty = 1.5f;
    clear_counters();
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

void dec_float(FP_TYPE* v, FP_TYPE by, FP_TYPE min, int clear)
{
    *v -= by;
    if (*v < min) *v = min;
    if (clear) clear_counters();
}

void inc_float(FP_TYPE* v, FP_TYPE by, int clear)
{
    *v += by;
    if (clear) clear_counters();
}
