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
#include "gui.h"
#include "parameters.h"

int finish_thread;
pthread_cond_t cond_fin;
pthread_mutex_t lock_fin;
volatile int tasks_finished;

extern unsigned int* colors;
int prepare_pixels(struct ocl_device* dev)
{
    int err;
    if (!dev->initialized) return 0;
    void* pixels;

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;
    dev->cl_pixels = clCreateBuffer(dev->ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, IMAGE_SIZE, pixels, &err);

    if (err != CL_SUCCESS)
    {
        printf("clCreateBuffer pixels returned %d\n", err);
        return 1;
    }
    printf("OCL buffer created with size=%d\n", IMAGE_SIZE);
    return 0;
}

int prepare_colors(struct ocl_device* dev)
{
    int err;

    if (!dev->initialized) return 0;

    dev->cl_colors = clCreateBuffer(dev->ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 4096, colors, &err);
    if (err != CL_SUCCESS)
    {
        printf("%s clCreateBuffer colors returned %d\n", dev->name, err);
        return 1;
    }

    return 0;
}

int set_kernel_arg(cl_kernel kernel, char* name, int i, int size, void* arg)
{
    int err;

    err = clSetKernelArg(kernel, i, size, arg);
    if (err != CL_SUCCESS)
    {
        printf("set_kernel_arg for: %s(i=%d size=%d) returned %d\n", name, i, size, err);
        return 1;
    }
    return 0;
}

int execute_fractal(struct ocl_device* dev, enum fractals fractal)
{
    size_t gws[2];
    size_t ofs[2] = {0, 0};
    cl_kernel kernel = dev->kernels[fractal];
    char* name = fractals[fractal].name;
    struct kernel_args* args = &dev->args[fractal];
    int err;
    unsigned long tp1, tp2;
    int fp_size = dev->fp64 ? sizeof(double) : sizeof(float);

    args->ofs_lx = ofs_lx;
    args->ofs_rx = ofs_rx;
    args->ofs_ty = ofs_ty;
    args->ofs_by = ofs_by;

    FP_TYPE ofs_lx1 = (args->ofs_lx + dx) / szx;
    FP_TYPE ofs_rx1 = (args->ofs_rx + dx) / szx;
    FP_TYPE ofs_ty1 = (args->ofs_ty + dy) / szy;
    FP_TYPE ofs_by1 = (args->ofs_by + dy) / szy;

    lx = ofs_lx1;
    rx = ofs_rx1;
    ty = ofs_ty1;
    by = ofs_by1;

    FP_TYPE step_x = (ofs_rx1 - ofs_lx1) / WIDTH_FL;
    FP_TYPE step_y = (ofs_by1 - ofs_ty1) / HEIGHT_FL;

    args->mm = mm;
    args->er = er;
    args->max_iter = max_iter;
    args->pal = pal;
    args->show_z = show_z;
    args->c_x = c_x;
    args->c_y = c_y;
    args->ofs_x++;
    if (args->ofs_x == 4)
    {
        args->ofs_y++;
        args->ofs_x = 0;
    }
    if (args->ofs_y == 4)
    {
        args->ofs_x = 0;
        args->ofs_y = 0;
    }

    gws[0] = gws_x;
    gws[1] = gws_y;

    if (set_kernel_arg(kernel, name, 0, sizeof(cl_mem), &dev->cl_pixels)) return 1;
    if (set_kernel_arg(kernel, name, 1, sizeof(cl_mem), &dev->cl_colors)) return 1;
    if (set_kernel_arg(kernel, name, 2, sizeof(cl_int), &mm)) return 1;
    if (dev->fp64)
    {
        if (set_kernel_arg(kernel, name, 3, fp_size, &ofs_lx1)) return 1;
        if (set_kernel_arg(kernel, name, 4, fp_size, &step_x)) return 1;
        if (set_kernel_arg(kernel, name, 5, fp_size, &ofs_ty1)) return 1;
        if (set_kernel_arg(kernel, name, 6, fp_size, &step_y)) return 1;
        if (set_kernel_arg(kernel, name, 7, fp_size, &args->er)) return 1;
    }
    else
    {
        float fofs_lx1 = (float)ofs_lx1;
        float fofs_ty1 = (float)ofs_ty1;

        float fstep_x = (float)step_x;
        float fstep_y = (float)step_y;

        float fer = (float)args->er;

        if (set_kernel_arg(kernel, name, 3, fp_size, &fofs_lx1)) return 1;
        if (set_kernel_arg(kernel, name, 4, fp_size, &fstep_x)) return 1;
        if (set_kernel_arg(kernel, name, 5, fp_size, &fofs_ty1)) return 1;
        if (set_kernel_arg(kernel, name, 6, fp_size, &fstep_y)) return 1;
        if (set_kernel_arg(kernel, name, 7, fp_size, &fer)) return 1;
    }
    if (set_kernel_arg(kernel, name, 8, sizeof(int), &args->max_iter)) return 1;
    if (set_kernel_arg(kernel, name, 9, sizeof(int), &args->pal)) return 1;
    if (set_kernel_arg(kernel, name, 10, sizeof(int), &args->show_z)) return 1;
    if (fractal == JULIA || fractal == JULIA_FULL || fractal == DRAGON || fractal == JULIA3)
    {
        if (dev->fp64)
        {
            if (set_kernel_arg(kernel, name, 11, fp_size, &args->c_x)) return 1;
            if (set_kernel_arg(kernel, name, 12, fp_size, &args->c_y)) return 1;
        }
        else
        {
            float fc_x = (float)args->c_x;
            float fc_y = (float)args->c_y;

            if (set_kernel_arg(kernel, name, 11, fp_size, &fc_x)) return 1;
            if (set_kernel_arg(kernel, name, 12, fp_size, &fc_y)) return 1;
        }

        if (fractal == JULIA || fractal == JULIA3)
        {
            if (set_kernel_arg(kernel, name, 13, sizeof(int), &args->ofs_x)) return 1;
            if (set_kernel_arg(kernel, name, 14, sizeof(int), &args->ofs_y)) return 1;
        }
    }
    if (fractal == MANDELBROT || fractal == BURNING_SHIP || fractal == GENERALIZED_CELTIC)
    {
        if (set_kernel_arg(kernel, name, 11, sizeof(int), &args->ofs_x)) return 1;
        if (set_kernel_arg(kernel, name, 12, sizeof(int), &args->ofs_y)) return 1;
    }

    // err = clEnqueueNDRangeKernel(dev->queue, kernel, 2, ofs, gws, NULL, 0,
    // NULL, &dev->event);
    //
    //    printf("%s: clEnqueueNDRangeKernel %s\n", dev->name, name);

    tp1 = get_time_usec();

    err = clEnqueueNDRangeKernel(dev->queue, kernel, 2, ofs, gws, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("%s: clEnqueueNDRangeKernel %s returned %d\n", dev->name, name, err);
        return 1;
    }
    // clFlush(dev->queue);
    // clWaitForEvents(1, &dev->event);
    clFinish(dev->queue);
    tp2 = get_time_usec();
    dev->execution = tp2 - tp1;

    //  clReleaseEvent(dev->event);

    // clFinish(cl[thread].queue_gpu);
    return 0;
}

void* ocl_kernel(void* d)
{
    struct ocl_device* dev = (struct ocl_device*)d;
    struct ocl_thread* t = &dev->thread;
    int err = 0;

    while (!finish_thread && !err)
    {
        //        printf("%s: waiting for work\n", dev->name);
        pthread_mutex_lock(&t->lock);
        while (!t->work)
        {
            if (finish_thread)
            {
                printf("%s: thread exits\n", dev->name);
                return NULL;
            }
            pthread_cond_wait(&t->cond, &t->lock);
        }
        t->work = 0;
        pthread_mutex_unlock(&t->lock);

        //        printf("ocl kernel for %s tid=%lx\n", dev->name,
        //        dev->thread.tid);

        err = execute_fractal(dev, fractal);

        if (err)
        {
            printf("thread interrupted\n");
            dev->thread.finished = 1;
            nr_devices--;
        }

        pthread_mutex_lock(&lock_fin);
        tasks_finished++;
        pthread_cond_signal(&cond_fin);
        pthread_cond_broadcast(&cond_fin);
        pthread_mutex_unlock(&lock_fin);
    }
    printf("%s: thread exits\n", dev->name);
    sleep(1);
    return NULL;
}

int signal_device(struct ocl_device* dev)
{
    //	printf("signal current device: %s\n", dev->name);
    if (!dev->initialized) return 1;
    if (dev->thread.finished)
    {
        printf("%s: thread already finished\n", dev->name);
        return 1;
    }

    pthread_mutex_lock(&dev->thread.lock);
    dev->thread.work = 1;
    pthread_cond_signal(&dev->thread.cond);
    pthread_mutex_unlock(&dev->thread.lock);
    return 0;
}

void start_ocl()
{
    if (!nr_devices) return;

    if (signal_device(&ocl_devices[current_device]))
    {
        printf("can't signal device\n");
        return;
    }

    pthread_mutex_lock(&lock_fin);
    //    printf("waiting for finished tasks\n");
    while (!tasks_finished)
    {
        pthread_cond_wait(&cond_fin, &lock_fin);
    }
    //    printf("tasks finished\n");
    tasks_finished = 0;
    pthread_mutex_unlock(&lock_fin);
}

void update_gpu_texture()
{
    int err;
    if (ocl_devices[current_device].initialized)
    {
        void* px1;

        px1 = clEnqueueMapBuffer(ocl_devices[current_device].queue, ocl_devices[current_device].cl_pixels, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, IMAGE_SIZE,
                                 0, NULL, NULL, &err);
        if (err != CL_SUCCESS)
        {
            printf("clEnqueueMapBuffer error %d\n", err);
        }
        else
        {
            SDL_UpdateTexture(texture, NULL, px1, WIDTH * 4);
            if (fractal == DRAGON) memset(px1, 0, IMAGE_SIZE);
            clEnqueueUnmapMemObject(ocl_devices[current_device].queue, ocl_devices[current_device].cl_pixels, px1, 0, NULL, NULL);
        }
    }
}

int prepare_thread(struct ocl_device* dev)
{
    if (!dev->initialized) return 0;

    dev->thread.work = 0;
    if (pthread_mutex_init(&dev->thread.lock, NULL)) return 1;
    if (pthread_cond_init(&dev->thread.cond, NULL)) return 1;
    pthread_create(&dev->thread.tid, NULL, ocl_kernel, dev);
    return 0;
}

void clear_pixels_ocl()
{
    if (ocl_devices && ocl_devices[current_device].initialized)
    {
        void* px1;
        int err;

        px1 = clEnqueueMapBuffer(ocl_devices[current_device].queue, ocl_devices[current_device].cl_pixels, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, IMAGE_SIZE,
                                 0, NULL, NULL, &err);
        if (err != CL_SUCCESS)
        {
            printf("clEnqueueMapBuffer "
                   "error %d\n",
                   err);
        }
        else
        {
            memset(px1, 0, IMAGE_SIZE);
            clEnqueueUnmapMemObject(ocl_devices[current_device].queue, ocl_devices[current_device].cl_pixels, px1, 0, NULL, NULL);
        }
    }
}
