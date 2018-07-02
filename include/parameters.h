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
#include "fractal.h"

#define OFS_LX -1.5f
#define OFS_RX 1.5f

extern FP_TYPE ofs_lx;
extern FP_TYPE ofs_rx;
extern FP_TYPE ofs_ty;
extern FP_TYPE ofs_by;
extern FP_TYPE lx;
extern FP_TYPE rx;
extern FP_TYPE ty;
extern FP_TYPE by;
extern int gws_x;
extern int gws_y;
extern FP_TYPE zx, zy;
extern FP_TYPE zoom;
extern FP_TYPE dx, dy;
extern FP_TYPE szx, szy;
extern FP_TYPE mx, my;
extern unsigned int rgb, mm;
extern FP_TYPE er;
extern unsigned int max_iter;
extern int pal;
extern int palette;
extern int show_z;
extern FP_TYPE c_x;
extern FP_TYPE c_y;
extern enum fractals fractal;
extern int cur_dev; // 0 - CPU, 1,..nr_devices - OCL devices
extern unsigned long long iter_limit;
extern unsigned long render_time;
extern unsigned long render_times;
extern unsigned long flips;
extern unsigned long frames_time;
extern unsigned long cpu_execution;
extern unsigned long cpu_executions, gpu_executions;
extern unsigned long cpu_iter, gpu_iter;
extern int color_channel;

int calculate_offsets();
void select_fractal(int f);
void select_fractals(int k);
void clear_counters();
void dec_int(unsigned int* v, unsigned int by, unsigned int min, int clear);
void inc_int(unsigned int* v, unsigned int by, int clear);
void dec_float(FP_TYPE* v, FP_TYPE by, FP_TYPE min, int clear);
void inc_float(FP_TYPE* v, FP_TYPE by, int clear);
void change_fractal_params(int kl, int mod_kl);
void change_fractal_colors(int kl, int mod_kl);
int move_fractal(int kl, int mod_kl);
