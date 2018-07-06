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

#ifndef __FRACTAL_TYPES__
#define __FRACTAL_TYPES__

struct kernel_args64
{
    unsigned int rgb;
    unsigned int mm;
    double ofs_lx;
    double ofs_rx;
    double ofs_ty;
    double ofs_by;
    double step_x;
    double step_y;
    double er;
    unsigned int max_iter;
    int pal;
    double c_x, c_y;
    int ofs_x, ofs_y;
    float c1[3], c2[3], c3[3], c4[3];
};

struct kernel_args32
{
    unsigned int rgb;
    unsigned int mm;
    float ofs_lx;
    float ofs_rx;
    float ofs_ty;
    float ofs_by;
    float step_x;
    float step_y;
    float er;
    unsigned int max_iter;
    int pal;
    float c_x, c_y;
    int ofs_x, ofs_y;
    float c1[3], c2[3], c3[3], c4[3];
};

#ifdef FP_64_SUPPORT
#define FP_TYPE double
#define KERNEL_ARGS kernel_args64
#else
#define FP_TYPE float
#define KERNEL_ARGS kernel_args32
#endif

#ifdef HOST_APP
#define __global
#endif
unsigned int set_color(struct KERNEL_ARGS args, unsigned int i, __global unsigned int* colors);

#endif
