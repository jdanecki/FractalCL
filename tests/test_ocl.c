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

int finish_thread;
volatile int tasks_finished;
enum fractals fractal = JULIA;

int execute_fractal(struct ocl_device* dev, enum fractals fractal)
{
    char* name = fractals[fractal].name;

    printf("execute fractal %s on %s\n", name, dev->name);
    return 0;
}

void* gpu_kernel(void* d)
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
                printf("thread exits\n");
                return NULL;
            }
            pthread_cond_wait(&t->cond, &t->lock);
        }
        t->work = 0;
        pthread_mutex_unlock(&t->lock);

        printf("gpu kernel for %s tid=%lx\n", dev->name, dev->thread.tid);

        err = execute_fractal(dev, fractal);

        if (err)
        {
            printf("thread interrupted\n");
            dev->thread.finished = 1;
            nr_devices--;
        }

        tasks_finished++;
    }
    printf("thread done\n");
    sleep(1);
    return NULL;
}

int signal_device(struct ocl_device* dev)
{
    if (!dev->initialized) return 0;
    if (dev->thread.finished) return 0;

    pthread_mutex_lock(&dev->thread.lock);
    dev->thread.work = 1;
    pthread_cond_signal(&dev->thread.cond);
    pthread_mutex_unlock(&dev->thread.lock);
    return 1;
}

int prepare_thread(struct ocl_device* dev)
{
    if (!dev->initialized) return 0;

    dev->thread.work = 0;
    if (pthread_mutex_init(&dev->thread.lock, NULL)) return 1;
    if (pthread_cond_init(&dev->thread.cond, NULL)) return 1;
    pthread_create(&dev->thread.tid, NULL, gpu_kernel, dev);
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
        if (signal_device(&ocl_devices[i])) return 1;
    }

    sleep(2);

    finish_thread = 1;

    ret += close_ocl();

    printf("test result = %d\n", ret);
    return ret;
}
