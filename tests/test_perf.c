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

#include "gui.h"
#include "timer.h"
#include <fractal_ocl.h>

int finish_thread;
volatile int tasks_finished;
int quiet;
unsigned int* pixels;

struct job
{
    struct ocl_device* dev;
    unsigned long exec_time[WIDTH];
    int i;
    int gws;
} * jobs;

unsigned long execute_fractal(struct ocl_device* dev, int ws_x, int ws_y)
{
    char* name = perf_kernel.name;
    cl_kernel kernel = dev->perf_kernel;
    size_t gws[1];
    size_t ofs[1] = {0};
    int err;
    unsigned long tp1, tp2;

    gws[0] = ws_x;
    //    gws[1] = ws_y;

    // printf("execute fractal [%s] on %s\n", name, dev->name);
    tp1 = get_time_usec();
    err = clEnqueueNDRangeKernel(dev->queue, kernel, 1, ofs, gws, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("%s: clEnqueueNDRangeKernel %s returned %d\n", dev->name, name, err);
        return 0;
    }
    clFinish(dev->queue);
    tp2 = get_time_usec();
    return tp2 - tp1;
}

void* ocl_kernel(void* d)
{
    struct job* j = (struct job*)d;
    struct ocl_device* dev = j->dev;
    cl_kernel kernel = dev->perf_kernel;
    int gws_y = 1;
    int err;

    j->gws = 1;

    printf("[%d]: start thread on %s\n", j->i, dev->name);

    dev->cl_pixels = clCreateBuffer(dev->ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, IMAGE_SIZE, pixels + WIDTH * j->i * 100, &err);
    if (err != CL_SUCCESS)
    {
        printf("clCreateBuffer pixels returned %d\n", err);
        return NULL;
    }

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &dev->cl_pixels);
    if (err != CL_SUCCESS)
    {
        printf("%s: clSetKernelArg returned %d\n", dev->name, err);
        return NULL;
    }
    err = clSetKernelArg(kernel, 1, sizeof(int), &j->i);
    if (err != CL_SUCCESS)
    {
        printf("%s: clSetKernelArg returned %d\n", dev->name, err);
        return NULL;
    }

    while (!finish_thread)
    {
        j->exec_time[j->gws - 1] = execute_fractal(dev, j->gws, gws_y);

        if (!j->exec_time)
        {
            printf("thread interrupted\n");
            dev->thread.finished = 1;
            nr_devices--;
            break;
        }
        //        else { printf("[%d]: time=%lu gws=[%d, %d]\n", j->i, j->exec_time[j->gws-1], j->gws, gws_y); }
        j->gws++;
        if (j->gws > WIDTH)
        {
            j->gws = 1;
            //			gws_y++;
        }
        /*		if (gws_y > HEIGHT) {
                    gws_y = 1;
                    gws_x = 1;
                }*/
    }
    printf("[%lx]: thread done for %s\n", dev->thread.tid, dev->name);
    return NULL;
}

int prepare_jobs(int t)
{
    struct ocl_device* dev = &ocl_devices[t];
    jobs[t].dev = dev;
    jobs[t].i = t;
    memset(jobs[t].exec_time, 0, WIDTH * 8);
    if (!dev->initialized) return 0;

    //    if (pthread_mutex_init(&dev->thread.lock, NULL)) return 1;
    if (pthread_cond_init(&dev->thread.cond, NULL)) return 1;
    pthread_create(&dev->thread.tid, NULL, ocl_kernel, &jobs[t]);
    return 0;
}

int main()
{
    int x, y, i;
    char status_line[200];
    int fin = 0, ret;

    srandom(time(0));
    ret = init_ocl();
    if (ret)
    {
        printf("problem with OpenCL initialization\n");
        return 1;
    }

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    for (y = 0; y < HEIGHT; y++)
    {
        for (x = 0; x < WIDTH; x++)
        {
            pixels[y * WIDTH + x] = 20 << 16 | 20 << 8 | 20;
        }
    }
    int mx = 0, my = 0;

    SDL_Rect window_rec;

    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    jobs = malloc(nr_devices * sizeof(struct job));

    for (i = 0; i < nr_devices; i++)
    {
        if (prepare_jobs(i)) return 1;
    }

    while (!fin)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                fin = 1;
                break;
            }
            if (event.type == SDL_KEYDOWN)
            {
                int kl = event.key.keysym.sym;
                switch (kl)
                {
                case 27:
                    fin = 1;
                    break;
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                mx = event.button.x;
                my = event.button.y;
            }
        }

        for (i = 0; i < nr_devices; i++)
        {
            unsigned int color;
            for (x = 0; x < WIDTH; x++)
            {
                y = jobs[i].exec_time[x] / 4;
                if (y > HEIGHT) y = HEIGHT - 1;
                if (i)
                    color = 250 << 16;
                else
                    color = 250 << 8 | 250;
                pixels[(HEIGHT - y) * WIDTH + x] = color;
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
        SDL_RenderCopy(main_window, texture, NULL, &window_rec);

        sprintf(status_line, "[%d,%d]=%ld / %ld ", mx, HEIGHT - my, jobs[0].exec_time[mx], jobs[1].exec_time[mx]);
        write_text(status_line, 0, HEIGHT / 2);
        SDL_RenderPresent(main_window);
        memset(pixels, 0, IMAGE_SIZE);
    }
    finish_thread = 1;
    ret += close_ocl();
    free(jobs);
    SDL_Quit();

    return ret;
}
