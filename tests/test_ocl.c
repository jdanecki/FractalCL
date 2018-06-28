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

#include <fractal_ocl.h>
#include <unistd.h>

int finish_thread;
volatile int tasks_finished;
int quiet;

int execute_fractal(struct ocl_device* dev)
{
    char* name = test_fractal.name;
    cl_kernel kernel = dev->test_kernel;
    size_t gws[2];
    size_t ofs[2] = {0, 0};
    int err;

    gws[0] = 1;
    gws[1] = 1;

    printf("execute fractal [%s] on %s\n", name, dev->name);
    err = clEnqueueNDRangeKernel(dev->queue, kernel, 2, ofs, gws, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("%s: clEnqueueNDRangeKernel %s returned %d\n", dev->name, name, err);
        return 1;
    }
    clFinish(dev->queue);

    return 0;
}

void* ocl_kernel(void* d)
{
    struct ocl_device* dev = (struct ocl_device*)d;
    struct ocl_thread* t = &dev->thread;
    int err = 0;

    while (!finish_thread && !err)
    {
        pthread_mutex_lock(&t->lock);
        while (!t->work)
        {
            if (finish_thread)
            {
                printf("thread exits for %s\n", dev->name);
                return NULL;
            }
            pthread_cond_wait(&t->cond, &t->lock);
        }
        t->work = 0;
        pthread_mutex_unlock(&t->lock);

        printf("ocl kernel for %s tid=%lx\n", dev->name, dev->thread.tid);

        err = execute_fractal(dev);

        if (err)
        {
            printf("thread interrupted\n");
            dev->thread.finished = 1;
            nr_devices--;
        }

        tasks_finished++;
    }
    printf("thread done for %s\n", dev->name);
    sleep(1);
    return NULL;
}

int signal_device(struct ocl_device* dev)
{
    if (!dev->initialized) return 1;
    if (dev->thread.finished) return 1;

    pthread_mutex_lock(&dev->thread.lock);
    dev->thread.work = 1;
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
