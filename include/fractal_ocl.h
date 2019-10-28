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
#include "fractal_types.h"
#include <CL/cl.h>

struct ocl_fractal
{
    char* name;
    char* source;
    int fd;
    size_t filesize;
};

extern struct ocl_fractal fractals[NR_FRACTALS];
extern struct ocl_fractal test_fractal;
extern struct ocl_fractal perf_kernel;

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
    cl_kernel perf_kernel;
#ifdef FP_64_SUPPORT
    struct kernel_args64 args64[NR_FRACTALS];
#endif
    struct kernel_args32 args32[NR_FRACTALS];
    cl_event event;
    char* name;
    char* vendor;
    char* device_version;
    char* driver_version;
    char* ocl_version;
    char* extensions;
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
extern int finish_thread;

int init_ocl();
int close_ocl();
int prepare_colors(struct ocl_device* dev);
int prepare_pixels(struct ocl_device* dev);
int prepare_thread(struct ocl_device* dev);
void start_ocl();
void clear_pixels_ocl();
void update_gpu_texture();
void show_ocl_devices();
void show_ocl_device(int d);
