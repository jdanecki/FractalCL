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
#include <SDL.h>
#include <SDL_ttf.h>
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
extern const char *font_file;
extern TTF_Font *font;

#define FONT_SIZE 20

#define WIDTH 1024
#define HEIGHT 768
#define WIDTH_FL (WIDTH * 1.0f)
#define HEIGHT_FL (HEIGHT * 1.0f)

#define BPP 4
#define PITCH (WIDTH * BPP)
#define IMAGE_SIZE (WIDTH * HEIGHT * BPP)

#define STRING(VALUE) #VALUE
#define STRING_MACRO(MACRO) STRING(MACRO)

int init_ocl();
int close_ocl();

enum fractals { JULIA, MANDELBROT, JULIA4, DRAGON, NR_FRACTALS };

struct kernel_args {
  void *pixels;
  unsigned int *colors;
  int mm;
  double ofs_lx;
  double ofs_rx;
  double ofs_ty;
  double ofs_by;
  double er;
  int max_iter;
  int pal;
  int show_z;
  double c_x, c_y;
  int ofs_x, ofs_y;
};

struct ocl_fractal {
  char *name;
  enum fractals id;
  char *source;
  int fd;
  size_t filesize;
};

extern struct ocl_fractal fractals[NR_FRACTALS];

struct ocl_thread {
  pthread_t tid;
  int work;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  int finished;
};

struct ocl_device {
  cl_device_id gpu;
  cl_command_queue queue;
  cl_platform_id id;
  cl_context ctx;
  int found;
  cl_program program;
  cl_kernel kernels[NR_FRACTALS];
  struct kernel_args args[NR_FRACTALS];
  cl_event event;
  char name[256];
  unsigned int eu;
  size_t wgs;
  struct ocl_thread thread;
  cl_mem cl_colors;
  cl_mem cl_pixels;
  unsigned int execution;
};

extern struct ocl_device intel;
extern volatile int nr_devices;
