/*
    Copyright (C) 2018-2019 Jacek Danecki <jacek.m.danecki@gmail.com>

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

#include <fractal_ocl.h>
#include <unistd.h>
#include "timer.h"

int finish_thread;
volatile int tasks_finished;
int quiet;

unsigned long execute_fractal(struct ocl_device* dev)
{
    char* name = test_fractal.name;
    cl_kernel kernel = dev->test_kernel;
    size_t gws[1];
    size_t ofs[1] = {0};
    int err;
    unsigned long exec_time;
    unsigned long tp1, tp2;

    gws[0] = 1;

    printf("execute fractal [%s] on %s\n", name, dev->name);
    tp1 = get_time_usec();
    err = clEnqueueNDRangeKernel(dev->queue, kernel, 1, ofs, gws, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("%s: clEnqueueNDRangeKernel %s returned %d\n", dev->name, name, err);
        return 0;
    }
    clFinish(dev->queue);
    tp2 = get_time_usec();
    exec_time = tp2 - tp1;

    return exec_time;
}

void* ocl_kernel(void* d)
{
    struct ocl_device* dev = (struct ocl_device*)d;
    struct ocl_thread* t = &dev->thread;
    unsigned long exec_time = 0;

    printf("[%lx]: start thread\n", dev->thread.tid);

    while (!finish_thread)
    {
        pthread_mutex_lock(&t->lock);
        while (!t->work)
        {
            if (finish_thread)
            {
                printf("thread exits for %s\n", dev->name);
                return NULL;
            }
            printf("[%lx]: wait thread\n", dev->thread.tid);
            pthread_cond_wait(&t->cond, &t->lock);
            printf("[%lx]: loop thread\n", dev->thread.tid);
        }
        t->work = 0;
        pthread_mutex_unlock(&t->lock);

        printf("[%lx]: ocl kernel for %s\n", dev->thread.tid, dev->name);

        exec_time = execute_fractal(dev);
        tasks_finished++;

        if (!exec_time)
        {
            printf("thread interrupted\n");
            dev->thread.finished = 1;
            nr_devices--;
            break;
        }
        else
        {
            printf("[%lx]: time=%lu\n", dev->thread.tid, exec_time);
        }
    }
    printf("[%lx]: thread done for %s\n", dev->thread.tid, dev->name);
    sleep(1);
    return NULL;
}

int signal_device(struct ocl_device* dev)
{
    if (!dev->initialized) return 1;
    if (dev->thread.finished) return 1;

    pthread_mutex_lock(&dev->thread.lock);
    dev->thread.work = 1;
    printf("[%lx]: signal\n", dev->thread.tid);
    pthread_cond_signal(&dev->thread.cond);
    pthread_mutex_unlock(&dev->thread.lock);
    return 0;
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

int main()
{
    int ret;
    int i;
    ret = init_ocl();
    if (ret)
    {
        printf("problem with OpenCL initialization\n");
        return 1;
    }
    for (i = 0; i < nr_devices; i++)
    {
        if (prepare_thread(&ocl_devices[i])) return 1;
    }

    for (i = 0; i < nr_devices; i++)
    {
        if (signal_device(&ocl_devices[i])) return 1;
        sleep(1);
    }

    printf("finishing test\n");
    finish_thread = 1;

    ret += close_ocl();

    printf("test result = %d\n", ret);
    return ret;
}
