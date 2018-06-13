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

#include <CL/cl.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define WIDTH 1024
#define HEIGHT 768
#define WIDTH_FL (WIDTH * 1.0f)
#define HEIGHT_FL (HEIGHT * 1.0f)

#define BPP 4
#define PITCH (WIDTH * BPP)
#define IMAGE_SIZE (WIDTH * HEIGHT * BPP)

#define STRING(VALUE) #VALUE
#define STRING_MACRO(MACRO) STRING(MACRO)

#ifdef FP_64_SUPPORT
#define FP_TYPE double
#else
#define FP_TYPE float
#endif

int init_ocl();
int close_ocl();

enum fractals
{
    JULIA,
    MANDELBROT,
    JULIA_FULL,
    DRAGON,
    JULIA3,
    NR_FRACTALS
};

struct kernel_args
{
    void* pixels;
    unsigned int* colors;
    int mm;
    FP_TYPE ofs_lx;
    FP_TYPE ofs_rx;
    FP_TYPE ofs_ty;
    FP_TYPE ofs_by;
    FP_TYPE er;
    int max_iter;
    int pal;
    int show_z;
    FP_TYPE c_x, c_y;
    int ofs_x, ofs_y;
};

struct ocl_fractal
{
    char* name;
    char* source;
    int fd;
    size_t filesize;
};

extern struct ocl_fractal fractals[NR_FRACTALS];
extern struct ocl_fractal test_fractal;

struct ocl_thread
{
    pthread_t tid;
    int work;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int finished;
};

struct ocl_device
{
    cl_device_id device_id;
    cl_device_type type;
    cl_command_queue queue;
    cl_platform_id platform_id;
    cl_context ctx;
    int initialized;
    cl_program program;
    cl_kernel kernels[NR_FRACTALS];
    cl_kernel test_kernel;
    struct kernel_args args[NR_FRACTALS];
    cl_event event;
    char name[100];
    unsigned int eu;
    size_t wgs;
    struct ocl_thread thread;
    cl_mem cl_colors;
    cl_mem cl_pixels;
    unsigned long execution;
    int intel;
    int fp64;
    int pocl;
};

extern struct ocl_device* ocl_devices;
extern int current_device;
extern volatile int nr_devices;
