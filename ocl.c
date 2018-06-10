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

#include "fractal_ocl.h"

struct ocl_device intel;
struct ocl_fractal fractals[NR_FRACTALS];

int create_ocl_device(char *plat_name, cl_platform_id id) {
  int err;
  unsigned int num;
  struct ocl_device *dev;
  size_t size;
  char name[256];
  cl_context_properties prop[3];

  if (!strncmp(plat_name, "Intel", 5))
    dev = &intel;

  err = clGetDeviceIDs(id, CL_DEVICE_TYPE_GPU, 0, NULL, &num);
  if ((err != CL_SUCCESS) || (num != 1)) {
    printf("OCL GPU device not found err=%d\n", err);
    return 1;
  }

  err = clGetDeviceIDs(id, CL_DEVICE_TYPE_GPU, 1, &dev->gpu, NULL);
  err = clGetDeviceInfo(dev->gpu, CL_DEVICE_NAME, 0, NULL, &size);
  if (size > 255)
    return 1;
  err = clGetDeviceInfo(dev->gpu, CL_DEVICE_NAME, size, name, NULL);
  printf("GPU device: %s\n", name);
  memcpy(dev->name, name, size);

  err = clGetDeviceInfo(dev->gpu, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint),
                        &dev->eu, NULL);
  if (err != CL_SUCCESS) {
    printf("clGetDeviceInfo CL_DEVICE_MAX_COMPUTE_UNITS returned %d\n", err);
    return 1;
  }
  printf("MAX_COMPUTE_UNITS=%u\n", dev->eu);

  err = clGetDeviceInfo(dev->gpu, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t),
                        &dev->wgs, NULL);
  if (err != CL_SUCCESS) {
    printf("clGetDeviceInfo CL_DEVICE_MAX_WORK_GROUP_SIZE returned %d\n", err);
    return 1;
  }
  printf("MAX_WORKGROUP_SIZE=%lu\n", dev->wgs);

  prop[0] = CL_CONTEXT_PLATFORM;
  prop[1] = (cl_context_properties)id;
  prop[2] = 0;
  dev->ctx = clCreateContext(prop, 1, &dev->gpu, NULL, NULL, &err);
  if (err != CL_SUCCESS) {
    printf("clCreateContext returned %d\n", err);
    return 1;
  }
  dev->id = id;
  dev->found = 1;
  return 0;
}

int create_kernel(struct ocl_device *dev, enum fractals fractal) {
  int err;
  size_t param1;
  cl_ulong param2;
  char *name = fractals[fractal].name;

  dev->kernels[fractal] = clCreateKernel(dev->program, name, &err);
  if (err != CL_SUCCESS) {
    printf("%s: clCreateKernel %s returned %d\n", dev->name, name, err);
    return 1;
  }

  err = clGetKernelWorkGroupInfo(dev->kernels[fractal], dev->gpu,
                                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                 sizeof(size_t), &param1, NULL);
  if (err != CL_SUCCESS) {
    printf("%s: clGetKernelWorkGroupInfo %s returned %d\n", dev->name, name,
           err);
    return 1;
  }
  err = clGetKernelWorkGroupInfo(dev->kernels[fractal], dev->gpu,
                                 CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong),
                                 &param2, NULL);

  if (err != CL_SUCCESS) {
    printf("%s: clGetKernelWorkGroupInfo %s returned %d\n", dev->name, name,
           err);
    return 1;
  }

  printf("%s: PREFERRED_WORK_GROUP_SIZE_MULTIPLE=%lu max_threads=%lu "
         "PRIVATE_MEM=%lu\n",
         name, param1, param1 * 7 * dev->eu, param2);
  return 0;
}

int create_kernels(struct ocl_device *dev, char *options) {
  int err, i;
  size_t size;
  char *log;

  char *sources[NR_FRACTALS];
  char cl_options[1024];
  size_t filesizes[NR_FRACTALS];
  if (!dev->found)
    return 0;

  printf("prepare kernels for %s\n", dev->name);

  for (i = 0; i < NR_FRACTALS; i++) {
    sources[i] = fractals[i].source;
    filesizes[i] = fractals[i].filesize;
  }
  sprintf(cl_options, "%s -D HEIGHT_FL=%f -D HEIGHT=%d -D WIDTH_FL=%f -D "
                      "WIDTH=%d -D BPP=%d -D PITCH=%d",
          options ? options : "", HEIGHT_FL, HEIGHT, WIDTH_FL, WIDTH, BPP,
          PITCH);
  dev->program = clCreateProgramWithSource(
      dev->ctx, NR_FRACTALS, (const char **)sources, filesizes, &err);
  if (err != CL_SUCCESS) {
    printf("%s: clCreateProgramWithSource returned %d\n", dev->name, err);
    return 1;
  }
  printf("compiling kernels with %s\n", cl_options);
  err = clBuildProgram(dev->program, 1, &dev->gpu, cl_options, NULL, NULL);
  printf("%s: clBuildProgram returned %d\n", dev->name, err);

  printf("%s: ------ compilation log  -----------\n", dev->name);
  clGetProgramBuildInfo(dev->program, dev->gpu, CL_PROGRAM_BUILD_LOG, 0, NULL,
                        &size);
  log = calloc(1, size);
  clGetProgramBuildInfo(dev->program, dev->gpu, CL_PROGRAM_BUILD_LOG, size, log,
                        NULL);
  printf("%s\n", log);
  free(log);

  if (err != CL_SUCCESS) {
    return 1;
  }

  dev->queue = clCreateCommandQueue(dev->ctx, dev->gpu, 0, &err);
  if (err != CL_SUCCESS) {
    printf("%s: clCreateCommandQueue on GPU returned %d\n", dev->name, err);
    return 1;
  }

  if (create_kernel(dev, JULIA))
    return 1;
  if (create_kernel(dev, MANDELBROT))
    return 1;
  if (create_kernel(dev, JULIA_FULL))
    return 1;
  if (create_kernel(dev, DRAGON))
    return 1;
  if (create_kernel(dev, JULIA3))
    return 1;
  printf("------------------------------------------\n");
  return 0;
}

void open_fractal(enum fractals fractal, char *name) {
  struct stat fileinfo;
  char filename[80];

  fractals[fractal].name = name;
  fractals[fractal].id = fractal;

  sprintf(filename, "%s.cl", name);
  stat(filename, &fileinfo);
  fractals[fractal].filesize = fileinfo.st_size;
  fractals[fractal].fd = open(filename, O_RDONLY);
  if (fractals[fractal].fd == -1)
    printf("open: %s\n", strerror(errno));

  fractals[fractal].source = mmap(NULL, fractals[fractal].filesize, PROT_READ,
                                  MAP_PRIVATE, fractals[fractal].fd, 0);
  if (fractals[fractal].source == MAP_FAILED)
    printf("mmap: %s\n", strerror(errno));
}

void close_fractal(enum fractals fractal) {
  munmap(fractals[fractal].source, fractals[fractal].filesize);
  close(fractals[fractal].fd);
}

int init_ocl() {
  int err, i;
  size_t size;
  unsigned int nr_platforms;
  cl_platform_id platforms_ids[2];
  char name[256];

  err = clGetPlatformIDs(0, NULL, &nr_platforms);
  if (err != CL_SUCCESS) {
    printf("clGetPlatformIDs returned %d\n", err);
    return 1;
  }
  printf("found %d platforms\n", nr_platforms);
  if (nr_platforms < 1) {
    printf("OCL platforms not found\n");
    return 1;
  }
  if (nr_platforms > 2) {
    printf("more than 2 platforms not supported\n");
    return 1;
  }
  err = clGetPlatformIDs(nr_platforms, platforms_ids, NULL);
  if (err != CL_SUCCESS) {
    printf("clGetPlatformIDs returned %d\n", err);
    return 1;
  }

  for (i = 0; i < nr_platforms; i++) {
    err = clGetPlatformInfo(platforms_ids[i], CL_PLATFORM_NAME, 0, NULL, &size);
    if (size > 255)
      return 1;
    err =
        clGetPlatformInfo(platforms_ids[i], CL_PLATFORM_NAME, size, name, NULL);
    printf("--- platform: %s\n", name);
    if (create_ocl_device(name, platforms_ids[i]))
      return 1;
  }
  nr_devices = intel.found;
  open_fractal(JULIA, "julia");
  open_fractal(MANDELBROT, "mandelbrot");
  open_fractal(JULIA_FULL, "julia_full");
  open_fractal(DRAGON, "dragon");
  open_fractal(JULIA3, "julia3");

  if (create_kernels(&intel, "-w"))
    return 1;

  return 0;
}

void close_device(struct ocl_device *dev) {
  int err, i;
  if (!dev->found)
    return;

  pthread_cond_signal(&dev->thread.cond);

  while (pthread_cond_destroy(&dev->thread.cond)) {
    pthread_cond_signal(&dev->thread.cond);
    sched_yield();
  }

  pthread_join(dev->thread.tid, NULL);
  pthread_mutex_destroy(&dev->thread.lock);

  clReleaseProgram(dev->program);

  for (i = 0; i < NR_FRACTALS; i++)
    clReleaseKernel(dev->kernels[i]);

  clReleaseMemObject(dev->cl_pixels);
  clReleaseMemObject(dev->cl_colors);

  err = clReleaseCommandQueue(dev->queue);
  if (err != CL_SUCCESS) {
    printf("%s: clReleaseCommandQueue gpu returned %d\n", dev->name, err);
    return;
  }

  err = clReleaseContext(dev->ctx);
  if (err != CL_SUCCESS) {
    printf("%s: clReleaseContext returned %d\n", dev->name, err);
  }
}

int close_ocl() {
  int i;
  close_device(&intel);

  for (i = 0; i < NR_FRACTALS; i++) {
    close_fractal(i);
  }
  return 0;
}
