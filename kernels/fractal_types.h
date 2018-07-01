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

struct kernel_args
{
    unsigned int mm;
    FP_TYPE ofs_lx;
    FP_TYPE ofs_rx;
    FP_TYPE ofs_ty;
    FP_TYPE ofs_by;
    FP_TYPE step_x;
    FP_TYPE step_y;
    FP_TYPE er;
    unsigned int max_iter;
    int pal;
    int show_z;
    FP_TYPE c_x, c_y;
    int ofs_x, ofs_y;
};
#endif
