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
#include "fractal_complex.h"

SDL_Surface *main_window;

volatile int tasks_finished;
volatile int nr_devices;

pthread_cond_t cond_fin;
pthread_mutex_t lock_fin;

double zx = 1.0, zy = 1.0;   // zoom x, y
double dx, dy;               // shift left/right, down/up
double szx = 1.0, szy = 1.0; // scale x and y
double mx, my; // mouse coordinates between [ofs_lx..ofs_rx, ofs_ty..ofs_by]

int mm = 16;

#if 0
double ofs_lx = -0.7402f;//-1.5f; //0.094; //-1.5f;
double ofs_rx = -0.7164f; //-0.716403707741993;//1.5f; //0.096; //1.5f;
double ofs_ty = 0.2205f;//0.283962759342063; //1.5f; //0.504f; //1.5f;
double ofs_by = 0.1690f;//0.283962759341958; //-1.5f; //0.503f; //-1.5f;
#else
double ofs_lx = -1.5f;
double ofs_rx = 1.5f;
double ofs_ty = 1.5f;
double ofs_by = -1.5f;
#endif
double er = 4.0f;

int max_iter = 360;
int pal = 0; // 0=hsv 1=rgb
int show_z;
double c_x = 0.15f;
double c_y = -0.60f;
// double c_x = -0.7f;
// double c_y = 0.27015f;

int bpp;

enum fractals fractal = JULIA;

int gws_x = WIDTH / 4;
int gws_y = HEIGHT / 4;

int init_window() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    return 1;
  main_window = SDL_SetVideoMode(WIDTH, HEIGHT, 0, SDL_SWSURFACE);

  const SDL_VideoInfo *info = SDL_GetVideoInfo();
  bpp = info->vfmt->BitsPerPixel;
  printf("BitsPerPixel=%d\n", bpp);

  SDL_WM_SetCaption("FractalCL", "FractalCL");
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  return 0;
}

int prepare_pixels(struct ocl_device *dev) {
  int err;
  if (!dev->found)
    return 0;
  void *pixels;

  err = posix_memalign((void **)&pixels, 4096, IMAGE_SIZE);
  dev->cl_pixels =
      clCreateBuffer(dev->ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                     IMAGE_SIZE, pixels, &err);

  if (err != CL_SUCCESS) {
    printf("clCreateBuffer pixels returned %d\n", err);
    return 1;
  }
  printf("GPU buffer created with size=%d\n", IMAGE_SIZE);
  return 0;
}

int prepare_colors(struct ocl_device *dev) {
  // h [0..359]
  // s [1]
  // v [0, 1]

  int err, v, iter;
  unsigned int *colors;

  if (!dev->found)
    return 0;

  err = posix_memalign((void **)&colors, 4096, 4096);
  if (err)
    return 1;

  for (v = 0; v < 2; v++) {
    for (iter = 0; iter < 360; iter++) {
      float h1 = (iter % 360) / 60.0;
      float v1 = 1.0 * v;
      int r, g, b;
      float r1, g1, b1, i, f, p, q, t;

      i = floor(h1);
      f = h1 - i;
      p = 0;
      q = v1 * (1.0 - f);
      t = v1 * (1.0 - (1.0 - f));
      switch ((int)i) {
      case 0:
        r1 = v1;
        g1 = t;
        b1 = p;
        break;
      case 1:
        r1 = q;
        g1 = v1;
        b1 = p;
        break;
      case 2:
        r1 = p;
        g1 = v1;
        b1 = t;
        break;
      case 3:
        r1 = p;
        g1 = q;
        b1 = v1;
        break;
      case 4:
        r1 = t;
        g1 = p;
        b1 = v1;
        break;
      case 5:
        r1 = v1;
        g1 = p;
        b1 = q;
        break;
      }
      r = roundf(255.0 * r1);
      r &= 0xff;
      g = roundf(255.0 * g1);
      g &= 0xff;
      b = roundf(255.0 * b1);
      b &= 0xff;
      colors[iter + v * 360] = 0xff000000 | r << 16 | g << 8 | b;
    }
  }
  colors[0] = 0;
  colors[360] = 0;
  dev->cl_colors = clCreateBuffer(
      dev->ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 4096, colors, &err);
  if (err != CL_SUCCESS) {
    printf("%s clCreateBuffer colors returned %d\n", dev->name, err);
    return 1;
  }

  return 0;
}

int finish_thread;

int set_kernel_arg(cl_kernel kernel, char *name, int i, int size, void *arg) {
  int err;

  err = clSetKernelArg(kernel, i, size, arg);
  if (err != CL_SUCCESS) {
    printf("set_kernel_arg for: %s(i=%d size=%d) returned %d\n", name, i, size,
           err);
    return 1;
  }
  return 0;
}

int execute_fractal(struct ocl_device *dev, enum fractals fractal) {
  size_t gws[2];
  size_t ofs[2] = {0, 0};
  cl_kernel kernel = dev->kernels[fractal];
  char *name = fractals[fractal].name;
  struct kernel_args *args = &dev->args[fractal];
  int err;

  args->ofs_lx = ofs_lx;
  args->ofs_rx = ofs_rx;
  args->ofs_ty = ofs_ty;
  args->ofs_by = ofs_by;

  double ofs_lx1 = (args->ofs_lx + dx) / szx;
  double ofs_rx1 = (args->ofs_rx + dx) / szx;
  double ofs_ty1 = (args->ofs_ty + dy) / szy;
  double ofs_by1 = (args->ofs_by + dy) / szy;
  args->mm = mm;
  args->er = er;
  args->max_iter = max_iter;
  args->pal = pal;
  args->show_z = show_z;
  args->c_x = c_x;
  args->c_y = c_y;
  args->ofs_x++;
  if (args->ofs_x == 4) {
    args->ofs_y++;
    args->ofs_x = 0;
  }
  if (args->ofs_y == 4) {
    args->ofs_x = 0;
    args->ofs_y = 0;
  }

  unsigned int ticks;

  gws[0] = gws_x;
  gws[1] = gws_y;

  // if (fractal == JULIA4)
  // {
  // 	gws[0] /= 4;
  //}
  if (set_kernel_arg(kernel, name, 0, sizeof(cl_mem), &dev->cl_pixels))
    return 1;
  if (set_kernel_arg(kernel, name, 1, sizeof(cl_mem), &dev->cl_colors))
    return 1;
  if (set_kernel_arg(kernel, name, 2, sizeof(cl_int), &mm))
    return 1;
  if (set_kernel_arg(kernel, name, 3, sizeof(double), &ofs_lx1))
    return 1;
  double step_x = (ofs_rx1 - ofs_lx1) / WIDTH_FL;
  if (set_kernel_arg(kernel, name, 4, sizeof(double), &step_x))
    return 1;
  //	if (set_kernel_arg(kernel, name, 4, sizeof(double), &ofs_rx1)) return 1;

  if (set_kernel_arg(kernel, name, 5, sizeof(double), &ofs_ty1))
    return 1;
  double step_y = (ofs_by1 - ofs_ty1) / HEIGHT_FL;
  if (set_kernel_arg(kernel, name, 6, sizeof(double), &step_y))
    return 1;
  //	if (set_kernel_arg(kernel, name, 6, sizeof(double), &ofs_by1)) return 1;

  if (set_kernel_arg(kernel, name, 7, sizeof(double), &args->er))
    return 1;
  if (set_kernel_arg(kernel, name, 8, sizeof(int), &args->max_iter))
    return 1;
  if (set_kernel_arg(kernel, name, 9, sizeof(int), &args->pal))
    return 1;
  if (set_kernel_arg(kernel, name, 10, sizeof(int), &args->show_z))
    return 1;
  if (fractal == JULIA || fractal == JULIA4 || fractal == DRAGON) {
    if (set_kernel_arg(kernel, name, 11, sizeof(double), &args->c_x))
      return 1;
    if (set_kernel_arg(kernel, name, 12, sizeof(double), &args->c_y))
      return 1;

    if (fractal == JULIA || fractal == DRAGON) {
      if (set_kernel_arg(kernel, name, 13, sizeof(int), &args->ofs_x))
        return 1;
      if (set_kernel_arg(kernel, name, 14, sizeof(int), &args->ofs_y))
        return 1;
    }
  }
  if (fractal == MANDELBROT) {
    if (set_kernel_arg(kernel, name, 11, sizeof(int), &args->ofs_x))
      return 1;
    if (set_kernel_arg(kernel, name, 12, sizeof(int), &args->ofs_y))
      return 1;
  }

  ticks = SDL_GetTicks();
  // err = clEnqueueNDRangeKernel(dev->queue, kernel, 2, ofs, gws, NULL, 0,
  // NULL, &dev->event);
  err = clEnqueueNDRangeKernel(dev->queue, kernel, 2, ofs, gws, NULL, 0, NULL,
                               NULL);
  if (err != CL_SUCCESS) {
    printf("%s: clEnqueueNDRangeKernel %s gpu main returned %d\n", dev->name,
           name, err);
    return 1;
  }
  // clFlush(dev->queue);
  // clWaitForEvents(1, &dev->event);
  clFinish(dev->queue);
  dev->execution = SDL_GetTicks() - ticks;
  //  printf("%s: Execution: %u\n", dev->name, dev->execution);
  clReleaseEvent(dev->event);

  // clFinish(cl[thread].queue_gpu);
  return 0;
}

//#define CHECK_TIME 1

void *gpu_kernel(void *d) {
  struct ocl_device *dev = (struct ocl_device *)d;
  struct ocl_thread *t = &dev->thread;
  int err = 0;

  while (!finish_thread && !err) {
    pthread_mutex_lock(&t->lock);
    while (!t->work) {
      if (finish_thread) {
        //		  printf("thread exits\n");
        return NULL;
      }
      pthread_cond_wait(&t->cond, &t->lock);
    }
    t->work = 0;
    pthread_mutex_unlock(&t->lock);

    //        printf("gpu kernel for %s tid=%lx\n", dev->name, dev->thread.tid);

    err = execute_fractal(dev, fractal);

    if (err) {
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
  // printf("thread exits\n");
  sleep(1);
  return NULL;
}

int signal_device(struct ocl_device *dev) {
  if (!dev->found)
    return 0;
  if (dev->thread.finished)
    return 0;

  pthread_mutex_lock(&dev->thread.lock);
  dev->thread.work = 1;
  pthread_cond_signal(&dev->thread.cond);
  pthread_mutex_unlock(&dev->thread.lock);
  return 1;
}

void start_gpu() {
  if (!nr_devices)
    return;
  if (signal_device(&intel) == 0)
    return;

  pthread_mutex_lock(&lock_fin);
  //  printf("waiting\n");
  while (tasks_finished != nr_devices) {
    pthread_cond_wait(&cond_fin, &lock_fin);
  }
  //  printf("tasks finished\n");
  tasks_finished = 0;
  pthread_mutex_unlock(&lock_fin);
}

int prepare_thread(struct ocl_device *dev) {
  if (!dev->found)
    return 0;

  dev->thread.work = 0;
  if (pthread_mutex_init(&dev->thread.lock, NULL))
    return 1;
  if (pthread_cond_init(&dev->thread.cond, NULL))
    return 1;
  pthread_create(&dev->thread.tid, NULL, gpu_kernel, dev);
  return 0;
}

void run_program() {
  SDL_Event event;
  int button;
  int click = 0;
  int key = 0;
#ifdef CHECK_TIME
//	SDL_Rect r;
//	int counter = 0;
#endif

  int err;
  // int i;
  int draw = 1;
  /*r.w = WIDTH;
  r.h = HEIGHT;
  r.x = 0;
  r.y = 0;
*/
  if (init_ocl())
    return;
  init_window();

  if (prepare_colors(&intel))
    return;
  if (prepare_pixels(&intel))
    return;

  if (pthread_mutex_init(&lock_fin, NULL))
    return;
  if (pthread_cond_init(&cond_fin, NULL))
    return;

  if (prepare_thread(&intel))
    return;

  while (1) {
#ifdef CHECK_TIME
    struct timespec tp1, tp2;
#endif
    // SDL_FillRect(main_window, &r, 0);

    if (click || key) {
      ofs_lx = (ofs_lx - mx) * zx + mx;
      ofs_rx = (ofs_rx - mx) * zx + mx;

      ofs_ty = (ofs_ty - my) * zy + my;
      ofs_by = (ofs_by - my) * zy + my;
    }

    draw = 1; // for tests only
    if (draw) {
#ifdef CHECK_TIME
      unsigned int ticks;
      clock_gettime(CLOCK_MONOTONIC_RAW, &tp1);
      ticks = SDL_GetTicks();

#endif
      start_gpu();
      if (intel.found) {
        void *px1;

        px1 = clEnqueueMapBuffer(intel.queue, intel.cl_pixels, CL_TRUE,
                                 CL_MAP_READ | CL_MAP_WRITE, 0, IMAGE_SIZE, 0,
                                 NULL, NULL, &err);
        if (err != CL_SUCCESS) {
          printf("clEnqueueMapBuffer error %d\n", err);
        } else {
          // memcpy(main_window->pixels + IMAGE_SIZE/2, px1, IMAGE_SIZE);
          memcpy(main_window->pixels, px1, IMAGE_SIZE);
          //					memset(px1, 0, IMAGE_SIZE);
          clEnqueueUnmapMemObject(intel.queue, intel.cl_pixels, px1, 0, NULL,
                                  NULL);
        }
      }

      SDL_Flip(main_window);

    } else
      SDL_Delay(1);

    if (!click)
      draw = 0;
    if (key) {
      draw = 0;
      key = 0;
    }

    while (SDL_PollEvent(&event)) {
      //		SDL_WaitEvent(&event); {

      if (event.type == SDL_QUIT)
        goto finish;
      if (event.type == SDL_KEYDOWN) {
        int kl = event.key.keysym.sym;

        switch (kl) {
        case 27:
          goto finish;
        case 'u':
          max_iter -= 10;
          if (max_iter < 10)
            max_iter = 10;
          break;
        case 'i':
          max_iter += 10;
          break;
        case 'o':
          mm++;
          break;
        case 'p':
          mm--;
          if (!mm)
            mm = 1;
          break;
        case 'k':
          mm += 100;
          break;
        case 'l':
          mm -= 100;
          if (mm < 1)
            mm = 1;
          break;
        case 'n':
          mm += 1000;
          break;
        case 'm':
          mm -= 1000;
          if (mm < 1)
            mm = 1;
          break;

        case SDLK_LEFT:
          szx -= 0.01;
          if (szx < 0.1)
            szx = 0.1;
          key = 1;
          break;
        case SDLK_RIGHT:
          szx += 0.01;
          key = 1;
          break;
        case SDLK_DOWN:
          szy -= 0.01;
          if (szy < 0.1)
            szy = 0.1;
          key = 1;
          break;
        case SDLK_UP:
          szy += 0.01;
          key = 1;
          break;
        case 'a':
          dx -= 0.1 / zx;
          key = 1;
          break;
        case 'd':
          dx += 0.1 / zx;
          key = 1;
          break;
        case 's':
          dy -= 0.1 / zy;
          key = 1;
          break;
        case 'w':
          dy += 0.1 / zy;
          key = 1;
          break;
        case 'z':
          er -= 0.1;
          if (er < 0.1)
            er = 0.1;
          break;
        case 'x':
          er += 0.1;
          break;
        case SDLK_COMMA:
          szx += 1.0;
          szy += 1.0;
          key = 1;
          break;
        case SDLK_PERIOD:
          szx -= 1.0;
          szy -= 1.0;
          if (szy < 0.1)
            szy = 0.1;
          if (szx < 0.1)
            szx = 0.1;
          key = 1;
          break;
        case 'h':
          pal ^= 1;
          if (!pal)
            printf("HSV\n");
          else
            printf("RGB\n");
          break;
        case '[':
          c_x -= 0.001;
          break;
        case ']':
          c_x += 0.001;
          break;
        case '1':
          show_z ^= 1;
          break;
        case '2':
          gws_x *= 2;
          if (gws_x > WIDTH)
            gws_x = WIDTH;
          gws_y *= 2;
          if (gws_y > HEIGHT)
            gws_y = HEIGHT;
          printf("gws: x=%d y=%d\n", gws_x, gws_y);
          break;
        case '3':
          gws_x /= 2;
          if (gws_x < 8)
            gws_x = 8;
          gws_y /= 2;
          if (gws_y < 8)
            gws_y = 8;
          printf("gws: x=%d y=%d\n", gws_x, gws_y);
          break;
        case '-':
          c_y -= 0.001;
          break;
        case '=':
          c_y += 0.001;
          break;
        case SDLK_F1:
          fractal = JULIA;
          break;
        case SDLK_F2:
          fractal = MANDELBROT;
          break;
        case SDLK_F3:
          fractal = JULIA4;
          break;
        case SDLK_F4:
          fractal = DRAGON;
          break;
        }
        draw = 1;
      }

      if (event.type == SDL_MOUSEMOTION) {
      }

      if (event.type == SDL_MOUSEBUTTONDOWN) {

        if (event.button.button == 2) {
          dx = 0;
          dy = 0;

          draw = 0;
          click = 0;
          continue;
        }

        mx = equation(event.button.x, 0, ofs_lx, WIDTH, ofs_rx);
        my = equation(event.button.y % (HEIGHT), 0, ofs_ty, HEIGHT, ofs_by);

        if (event.button.button == 3) {
          if (button != event.button.button) {
            zx = 1.0;
            zy = 1.0;
          } else {
            zx += 0.01;
            zy += 0.01;
          }
          click = 1;
        }

        if (event.button.button == 1) {
          if (button != event.button.button) {
            zx = 1.0;
            zy = 1.0;
          } else {
            zx -= 0.01;
            zy -= 0.01;
          }
          click = 1;
        }
        button = event.button.button;

        draw = 1;
      }
      if (event.type == SDL_MOUSEBUTTONUP) {
        //   click = 0;
      }
    }
  }
finish:
  finish_thread = 1;
  pthread_mutex_destroy(&lock_fin);
  pthread_cond_destroy(&cond_fin);

  close_ocl();
  SDL_Quit();
}

int main() {
  srandom(time(0));

  run_program();
  return 0;
}
