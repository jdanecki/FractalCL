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
#include "window.h"
#include <unistd.h>

volatile int nr_devices;
struct ocl_device* ocl_devices;
int current_device;
struct ocl_fractal fractals[NR_FRACTALS];
struct ocl_fractal test_fractal;
extern int quiet;

int create_ocl_device(int di, char* plat_name, cl_platform_id id)
{
    int err;
    unsigned int num;
    struct ocl_device* dev = &ocl_devices[di];
    size_t size;
    cl_context_properties prop[3];

    if (!strncmp(plat_name, "Intel", 5)) dev->intel = 1;
    if (!strncmp(plat_name, "Portable", 8)) dev->pocl = 1;

    err = clGetDeviceIDs(id, CL_DEVICE_TYPE_ALL, 0, NULL, &num);
    if ((err != CL_SUCCESS) || (num != 1))
    {
        printf("OCL device not found err=%d\n", err);
        return 1;
    }

    err = clGetDeviceIDs(id, CL_DEVICE_TYPE_ALL, 1, &dev->device_id, NULL);

    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_NAME, 0, NULL, &size);
    if (size > 4095) return 1;
    dev->name = malloc(size + 1);

    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_NAME, size, dev->name, NULL);
    if (!quiet) printf("OCL device: %s\n", dev->name);

    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_VENDOR, 0, NULL, &size);
    if (size > 4095) return 1;
    dev->vendor = malloc(size + 1);
    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_VENDOR, size, dev->vendor, NULL);
    if (!quiet) printf("OCL device vendor: %s\n", dev->vendor);

    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_VERSION, 0, NULL, &size);
    if (size > 4095) return 1;
    dev->device_version = malloc(size + 1);
    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_VERSION, size, dev->device_version, NULL);
    if (!quiet) printf("OCL device version: %s\n", dev->device_version);

    err = clGetDeviceInfo(dev->device_id, CL_DRIVER_VERSION, 0, NULL, &size);
    if (size > 4095) return 1;
    dev->driver_version = malloc(size + 1);
    err = clGetDeviceInfo(dev->device_id, CL_DRIVER_VERSION, size, dev->driver_version, NULL);
    if (!quiet) printf("OCL driver version: %s\n", dev->driver_version);

    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &size);
    if (size > 4095) return 1;
    dev->ocl_version = malloc(size + 1);
    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_OPENCL_C_VERSION, size, dev->ocl_version, NULL);
    if (!quiet) printf("OCL C version: %s\n", dev->ocl_version);

    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_EXTENSIONS, 0, NULL, &size);
    if (size > 4095) return 1;
    dev->extensions = malloc(size + 1);
    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_EXTENSIONS, size, dev->extensions, NULL);
    if (!quiet) printf("OCL device extensions: %s\n", dev->extensions);
    if (strstr(dev->extensions, "cl_khr_fp64"))
    {
#ifdef FP_64_SUPPORT
        if (!quiet) printf("cl_khr_fp64 supported by device, and enabled in project configuration\n");
        dev->fp64 = 1;
#else
        if (!quiet)
        {
            printf("!!! cl_khr_fp64 supported by device, but disabled in project configuration !!!\n");
            printf("!!! enable it in configure script !!! \n");
        }
#endif
    }

    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_TYPE, 0, NULL, &size);
    if (size > 4095) return 1;
    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_TYPE, size, &dev->type, NULL);
    if (!quiet) printf("OCL device type: %ld\n", dev->type);

    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &dev->eu, NULL);
    if (err != CL_SUCCESS)
    {
        printf("clGetDeviceInfo CL_DEVICE_MAX_COMPUTE_UNITS returned %d\n", err);
        return 1;
    }
    if (!quiet) printf("MAX_COMPUTE_UNITS=%u\n", dev->eu);

    err = clGetDeviceInfo(dev->device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &dev->wgs, NULL);
    if (err != CL_SUCCESS)
    {
        printf("clGetDeviceInfo CL_DEVICE_MAX_WORK_GROUP_SIZE returned %d\n", err);
        return 1;
    }
    if (!quiet) printf("MAX_WORKGROUP_SIZE=%lu\n", dev->wgs);

    prop[0] = CL_CONTEXT_PLATFORM;
    prop[1] = (cl_context_properties)id;
    prop[2] = 0;
    dev->ctx = clCreateContext(prop, 1, &dev->device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("clCreateContext returned %d\n", err);
        return 1;
    }
    dev->platform_id = id;
    dev->initialized = 1;
    return 0;
}

int create_kernel(struct ocl_device* dev, struct ocl_fractal* fractal, cl_kernel* kernel)
{
    int err;
    size_t param1;
    cl_ulong param2;
    char* name = fractal->name;

    *kernel = clCreateKernel(dev->program, name, &err);
    if (err != CL_SUCCESS)
    {
        printf("%s: clCreateKernel [%s] returned %d\n", dev->name, name, err);
        return 1;
    }

    err = clGetKernelWorkGroupInfo(*kernel, dev->device_id, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &param1, NULL);
    if (err != CL_SUCCESS)
    {
        printf("%s: clGetKernelWorkGroupInfo %s returned %d\n", dev->name, name, err);
    }
    else
    {
        if (!quiet)
        {
            printf("%s: PREFERRED_WORK_GROUP_SIZE_MULTIPLE=%lu\n", name, param1);
            if (dev->intel) printf("threads=%lu\n", param1 * 7 * dev->eu);
        }
    }
    if (!dev->pocl)
    {
        err = clGetKernelWorkGroupInfo(*kernel, dev->device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &param2, NULL);
        if (err != CL_SUCCESS)
        {
            printf("%s: clGetKernelWorkGroupInfo %s returned %d\n", dev->name, name, err);
        }
        else
        {
            if (!quiet) printf("%s: PRIVATE_MEM=%lu\n", name, param2);
        }
    }
    return 0;
}

int create_kernels(struct ocl_device* dev, char* options)
{
    int err, i;
    size_t size;
    char* log;

    char* sources[NR_FRACTALS + 1]; // one more for test_kernel
    char cl_options[1024];
    size_t filesizes[NR_FRACTALS + 1];
    if (!dev->initialized) return 0;

    if (!quiet) printf("prepare kernels for %s\n", dev->name);

    for (i = 0; i < NR_FRACTALS; i++)
    {
        sources[i] = fractals[i].source;
        filesizes[i] = fractals[i].filesize;
        if (!quiet) printf("preparing kernel%d]: %s\n", i, fractals[i].name);
    }
    sources[i] = test_fractal.source;
    filesizes[i] = test_fractal.filesize;

    sprintf(cl_options, "%s -D HEIGHT_FL=%f -D HEIGHT=%d -D WIDTH_FL=%f -D "
                        "WIDTH=%d -D BPP=%d -D PITCH=%d %s",
            options ? options : "", HEIGHT_FL, HEIGHT, WIDTH_FL, WIDTH, BPP, PITCH, dev->fp64 ? "-DFP_64_SUPPORT=1" : "");
    dev->program = clCreateProgramWithSource(dev->ctx, NR_FRACTALS + 1, (const char**)sources, filesizes, &err);
    if (err != CL_SUCCESS)
    {
        printf("%s: clCreateProgramWithSource returned %d\n", dev->name, err);
        return 1;
    }
    if (!quiet) printf("compiling kernels with %s\n", cl_options);
    err = clBuildProgram(dev->program, 1, &dev->device_id, cl_options, NULL, NULL);
    if (!quiet) printf("%s: clBuildProgram returned %d\n", dev->name, err);

    if (!quiet) printf("%s: ------ compilation log  -----------\n", dev->name);
    clGetProgramBuildInfo(dev->program, dev->device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &size);
    log = calloc(1, size);
    clGetProgramBuildInfo(dev->program, dev->device_id, CL_PROGRAM_BUILD_LOG, size, log, NULL);
    if (!quiet) printf("%s\n", log);
    free(log);

    if (err != CL_SUCCESS)
    {
        return 1;
    }

    dev->queue = clCreateCommandQueue(dev->ctx, dev->device_id, 0, &err);
    if (err != CL_SUCCESS)
    {
        printf("%s: clCreateCommandQueue on GPU returned %d\n", dev->name, err);
        return 1;
    }

    if (create_kernel(dev, &fractals[JULIA], &dev->kernels[JULIA])) return 1;
    if (create_kernel(dev, &fractals[MANDELBROT], &dev->kernels[MANDELBROT])) return 1;
    if (create_kernel(dev, &fractals[JULIA_FULL], &dev->kernels[JULIA_FULL])) return 1;
    if (create_kernel(dev, &fractals[DRAGON], &dev->kernels[DRAGON])) return 1;
    if (create_kernel(dev, &fractals[JULIA3], &dev->kernels[JULIA3])) return 1;
    if (create_kernel(dev, &fractals[BURNING_SHIP], &dev->kernels[BURNING_SHIP])) return 1;
    if (create_kernel(dev, &fractals[GENERALIZED_CELTIC], &dev->kernels[GENERALIZED_CELTIC])) return 1;

    if (create_kernel(dev, &test_fractal, &dev->test_kernel)) return 1;

    if (!quiet) printf("------------------------------------------\n");
    return 0;
}

void open_fractal(struct ocl_fractal* fractal, char* name)
{
    struct stat fileinfo;
    char filename[256];

    fractal->name = name;

    sprintf(filename, "%s/kernels/%s.cl", STRING_MACRO(DATA_PATH), name);
    stat(filename, &fileinfo);
    fractal->filesize = fileinfo.st_size;
    fractal->fd = open(filename, O_RDONLY);
    if (fractal->fd == -1) printf("Can't open %s: %s\n", filename, strerror(errno));

    fractal->source = mmap(NULL, fractal->filesize, PROT_READ, MAP_PRIVATE, fractal->fd, 0);
    if (fractal->source == MAP_FAILED) printf("mmap: %s\n", strerror(errno));
}

void close_fractal(struct ocl_fractal* fractal)
{
    munmap(fractal->source, fractal->filesize);
    close(fractal->fd);
}

int init_ocl()
{
    int err = 0, i;
    size_t size;
    unsigned int nr_platforms;
    cl_platform_id* platforms_ids;
    char name[256];

    err = clGetPlatformIDs(0, NULL, &nr_platforms);
    if (err != CL_SUCCESS)
    {
        printf("clGetPlatformIDs returned %d\n", err);
        return 1;
    }
    if (!quiet) printf("found %d platforms\n", nr_platforms);
    if (nr_platforms < 1)
    {
        printf("OCL platforms not found\n");
        return 1;
    }
    platforms_ids = malloc(nr_platforms * sizeof(cl_platform_id));
    err = clGetPlatformIDs(nr_platforms, platforms_ids, NULL);
    if (err != CL_SUCCESS)
    {
        printf("clGetPlatformIDs returned %d\n", err);
        err = 1;
        goto deallocate_return;
    }

    ocl_devices = calloc(nr_platforms, sizeof(struct ocl_device));
    for (i = 0; i < nr_platforms; i++)
    {
        err = clGetPlatformInfo(platforms_ids[i], CL_PLATFORM_NAME, 0, NULL, &size);
        if (size > 99)
        {
            err = 1;
            goto deallocate_return;
        }
        err = clGetPlatformInfo(platforms_ids[i], CL_PLATFORM_NAME, size, name, NULL);
        if (!quiet) printf("--- platform: %s\n", name);
        if (create_ocl_device(i, name, platforms_ids[i]))
        {
            err = 1;
            goto deallocate_return;
        }
        nr_devices++;
    }
    current_device = 0;
    open_fractal(&fractals[JULIA], "julia");
    open_fractal(&fractals[MANDELBROT], "mandelbrot");
    open_fractal(&fractals[JULIA_FULL], "julia_full");
    open_fractal(&fractals[DRAGON], "dragon");
    open_fractal(&fractals[JULIA3], "julia3");
    open_fractal(&fractals[BURNING_SHIP], "burning_ship");
    open_fractal(&fractals[GENERALIZED_CELTIC], "generalized_celtic");

    open_fractal(&test_fractal, "test_kernel");

    for (i = 0; i < nr_devices; i++) err |= create_kernels(&ocl_devices[i], "-w");

deallocate_return:
    free(platforms_ids);
    return err;
}

void close_device(struct ocl_device* dev)
{
    int err, i;
    if (!dev->initialized) return;

    pthread_cond_signal(&dev->thread.cond);

    while (pthread_cond_destroy(&dev->thread.cond))
    {
        pthread_cond_signal(&dev->thread.cond);
        sched_yield();
    }

    pthread_join(dev->thread.tid, NULL);
    pthread_mutex_destroy(&dev->thread.lock);

    clReleaseProgram(dev->program);

    for (i = 0; i < NR_FRACTALS; i++) clReleaseKernel(dev->kernels[i]);

    clReleaseMemObject(dev->cl_pixels);
    clReleaseMemObject(dev->cl_colors);

    err = clReleaseCommandQueue(dev->queue);
    if (err != CL_SUCCESS)
    {
        printf("%s: clReleaseCommandQueue returned %d\n", dev->name, err);
        return;
    }

    err = clReleaseContext(dev->ctx);
    if (err != CL_SUCCESS)
    {
        printf("%s: clReleaseContext returned %d\n", dev->name, err);
    }
}

int close_ocl()
{
    int i;
    for (i = 0; i < nr_devices; i++) close_device(&ocl_devices[i]);
    free(ocl_devices);
    for (i = 0; i < NR_FRACTALS; i++)
    {
        close_fractal(&fractals[i]);
    }
    close_fractal(&test_fractal);
    return 0;
}

void show_ocl_device(int d)
{
    struct ocl_device* dev = &ocl_devices[d];
    printf("name:           %s\n", dev->name);
    printf("vendor:         %s\n", dev->vendor);
    printf("device version: %s\n", dev->device_version);
    printf("driver version: %s\n", dev->driver_version);
    printf("ocl C version:  %s\n", dev->ocl_version);
    printf("type:           %s\n", dev->type == 4 ? "GPU" : dev->type == 2 ? "CPU" : "other");
    printf("extensions:     %s\n", dev->extensions);
}

void show_ocl_devices()
{
    int d;
    for (d = 0; d < nr_devices; d++)
    {
        printf("%d:\n", d);
        show_ocl_device(d);
        puts("---------------------------------------------------------");
    }
}
