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

#include "gui.h"
#ifdef OPENCL_SUPPORT
#include "fractal_ocl.h"
#else
#include "fractal.h"
#endif

#include "kernels/burning_ship.cl"
#include "kernels/common.cl"
#include "kernels/dragon.cl"
#include "kernels/generalized_celtic.cl"
#include "kernels/julia.cl"
#include "kernels/julia3.cl"
#include "kernels/julia_full.cl"
#include "kernels/mandelbrot.cl"
#include "kernels/tricorn.cl"

#include "parameters.h"

unsigned int* colors;
void* cpu_pixels;
int quiet;
int all_devices;
char status_line[200];

int draw_frames = 16;

#ifdef OPENCL_SUPPORT
extern pthread_cond_t cond_fin;
extern pthread_mutex_t lock_fin;
extern volatile int tasks_finished;
#endif

int performance_test;
int show_iterations;
unsigned long last_avg_result;
int console_mode;

char* fractals_names[NR_FRACTALS] = {"julia z^2", "mandelbrot", "julia full", "dragon", "julia z^3", "burning ship", "generalized celtic", "tricorn"};

enum app_modes
{
    APP_GUI,  // default GUI support
    APP_TEST, // performance test
    APP_DISC, // discovery mode, show devices
};

#define MAX(a, b) (a > b) ? a : b
#define MIN(a, b) (a < b) ? a : b

void rgb2hsv(int r, int g, int b, int* h, int* s, int* v)
{
    float r1, g1, b1, h1, s1, v1;
    float cmax, cmin;
    float t, delta;

    r1 = r / 255.0;
    g1 = g / 255.0;
    b1 = b / 255.0;

    t = MAX(r1, g1);
    if (t > b1)
        cmax = t;
    else
        cmax = b1;

    t = MIN(r1, g1);
    if (t < b1)
        cmin = t;
    else
        cmin = b1;

    v1 = cmax;

    delta = cmax - cmin;
    if (delta != 0)
    {
        s1 = delta / cmax;
    }
    else
        s1 = 0.0;
    if (s1 == 0)
    {
        h1 = 0.0;
    }
    else
    {
        if (r1 == cmax)
        {
            h1 = (g1 - b1) / delta;
        }
        else
        {
            if (g1 == cmax)
            {
                h1 = 2.0 + (b1 - r1) / delta;
            }
            else
            {
                if (b1 == cmax)
                {
                    h1 = 4.0 + (r1 - g1) / delta;
                }
            }
        }
    }
    *h = (int)roundf(60.0 * h1);
    *s = (int)roundf(100 * s1);
    *v = (int)roundf(100 * v1);
}

void hsv2rgb(int h, int s, int v, int* r, int* g, int* b)
{
    // h [0..360]
    // s [0..100]
    // v [1..100]
    float r1, g1, b1, i, f, p, q, t, h1, s1, v1;
    s1 = s / 100.0;
    v1 = v / 100.0;

    if (!s && !h)
    {
        *r = 255 * v1;
        *g = 255 * v1;
        *b = 255 * v1;
        return;
    }
    if (h == 360) h1 = 0.0;
    h1 = h / 60.0;
    i = floor(h1);
    f = h1 - i;
    p = v1 * (1.0 - s1);
    q = v1 * (1.0 - s1 * f);
    t = v1 * (1.0 - (s1 * (1.0 - f)));
    switch ((int)i)
    {
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
    *r = roundf(255 * r1);
    *g = roundf(255 * g1);
    *b = roundf(255 * b1);
}

int initialize_colors()
{
    int err, v, iter;
#ifdef OPENCL_SUPPORT
    int d;
#endif
    // h [0..359]
    // s [1]
    // v [0, 1]
    err = posix_memalign((void**)&colors, 4096, 4096);
    if (err) return 1;

    for (v = 0; v < 2; v++)
    {
        for (iter = 0; iter < 360; iter++)
        {
            float h1 = iter / 60.0;
            float v1 = v ? 1.0 : 0.5; // 1.0 * v;
            int r, g, b;
            float r1, g1, b1, i, f, p, q, t;

            i = floor(h1);
            f = h1 - i;
            p = 0;
            q = v1 * (1.0 - f);
            t = v1 * (1.0 - (1.0 - f));
            switch ((int)i)
            {
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
            //			printf("v=%d i=%d r=%f g=%f b=%f\n", v, iter, r1, g1, b1);
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
#ifdef OPENCL_SUPPORT
    for (d = 0; d < nr_devices; d++)
    {
        if (prepare_colors(&ocl_devices[d])) return 1;
        if (prepare_pixels(&ocl_devices[d])) return 1;
        if (prepare_thread(&ocl_devices[d])) return 1;
    }
#endif
    return 0;
}

struct cpu_args
{
    int xs, xe, ys, ye;
};

#ifdef FP_64_SUPPORT
struct kernel_args64 cpu_kernel_args;
#else
struct kernel_args32 cpu_kernel_args;
#endif

void prepare_cpu_args()
{
    int c;

    cpu_kernel_args.ofs_lx = ofs_lx;
    cpu_kernel_args.ofs_rx = ofs_rx;
    cpu_kernel_args.ofs_ty = ofs_ty;
    cpu_kernel_args.ofs_by = ofs_by;

    FP_TYPE ofs_lx1 = (ofs_lx + dx) / szx;
    FP_TYPE ofs_rx1 = (ofs_rx + dx) / szx;
    FP_TYPE ofs_ty1 = (ofs_ty + dy) / szy;
    FP_TYPE ofs_by1 = (ofs_by + dy) / szy;

    lx = ofs_lx1;
    cpu_kernel_args.ofs_lx = ofs_lx1;
    rx = ofs_rx1;
    ty = ofs_ty1;
    cpu_kernel_args.ofs_ty = ofs_ty1;
    by = ofs_by1;

    cpu_kernel_args.step_x = (ofs_rx1 - ofs_lx1) / WIDTH_FL;
    cpu_kernel_args.step_y = (ofs_by1 - ofs_ty1) / HEIGHT_FL;

    cpu_kernel_args.rgb = rgb;
    cpu_kernel_args.mm = mm;
    cpu_kernel_args.er = er;
    cpu_kernel_args.max_iter = max_iter;
    cpu_kernel_args.mod1 = mod1;
    cpu_kernel_args.pal = pal;
    cpu_kernel_args.c_x = c_x;
    cpu_kernel_args.c_y = c_y;

    for (c = 0; c < 3; c++)
    {
        cpu_kernel_args.c1[c] = c1[c];
        cpu_kernel_args.c2[c] = c2[c];
        cpu_kernel_args.c3[c] = c3[c];
        cpu_kernel_args.c4[c] = c4[c];
    }
}

unsigned int calculate_one_pixel(int x, int y)
{
    switch (fractal)
    {
    case JULIA:
        return julia(cpu_kernel_args.ofs_x + x * 4, cpu_kernel_args.ofs_y + y * 4, cpu_pixels, colors, cpu_kernel_args);

    case JULIA3:
        return julia3(cpu_kernel_args.ofs_x + x * 4, cpu_kernel_args.ofs_y + y * 4, cpu_pixels, colors, cpu_kernel_args);

    case JULIA_FULL:
        return julia_full(x, y, cpu_pixels, colors, cpu_kernel_args);

    case MANDELBROT:
        return mandelbrot(cpu_kernel_args.ofs_x + x * 4, cpu_kernel_args.ofs_y + y * 4, cpu_pixels, colors, cpu_kernel_args);

    case BURNING_SHIP:
        return burning_ship(cpu_kernel_args.ofs_x + x * 4, cpu_kernel_args.ofs_y + y * 4, cpu_pixels, colors, cpu_kernel_args);

    case GENERALIZED_CELTIC:
        return generalized_celtic(cpu_kernel_args.ofs_x + x * 4, cpu_kernel_args.ofs_y + y * 4, cpu_pixels, colors, cpu_kernel_args);

    case TRICORN:
        return tricorn(cpu_kernel_args.ofs_x + x * 4, cpu_kernel_args.ofs_y + y * 4, cpu_pixels, colors, cpu_kernel_args);

    default:
        return 0;
    }
}

void* execute_fractal_cpu(void* c)
{
    int x, y;
    struct cpu_args* cpu = (struct cpu_args*)c;

    prepare_cpu_args();

    for (y = cpu->ys; y < cpu->ye; y++)
    {
        for (x = cpu->xs; x < cpu->xe; x++)
        {
            calculate_one_pixel(x, y);
        }
    }
    return NULL;
}

void start_cpu()
{
    unsigned long tp1, tp2;
    int t;

    cpu_kernel_args.ofs_x++;
    if (cpu_kernel_args.ofs_x == 4)
    {
        cpu_kernel_args.ofs_y++;
        cpu_kernel_args.ofs_x = 0;
    }
    if (cpu_kernel_args.ofs_y == 4)
    {
        cpu_kernel_args.ofs_x = 0;
        cpu_kernel_args.ofs_y = 0;
    }

    tp1 = get_time_usec();

    if (fractal == DRAGON)
    {
        memset(cpu_pixels, 0, IMAGE_SIZE);
        prepare_cpu_args();
        dragon(0, 0, cpu_pixels, colors, cpu_kernel_args);
    }
    else
    {
        struct cpu_args t_args[16] = {{0, gws_x / 4, 0, gws_y / 4},
                                      {gws_x / 4, gws_x / 2, 0, gws_y / 4},
                                      {gws_x / 2, gws_x * 3 / 4, 0, gws_y / 4},
                                      {gws_x * 3 / 4, gws_x, 0, gws_y / 4},

                                      {0, gws_x / 4, gws_y / 4, gws_y / 2},
                                      {gws_x / 4, gws_x / 2, gws_y / 4, gws_y / 2},
                                      {gws_x / 2, gws_x * 3 / 4, gws_y / 4, gws_y / 2},
                                      {gws_x * 3 / 4, gws_x, gws_y / 4, gws_y / 2},

                                      {0, gws_x / 4, gws_y / 2, gws_y * 3 / 4},
                                      {gws_x / 4, gws_x / 2, gws_y / 2, gws_y * 3 / 4},
                                      {gws_x / 2, gws_x * 3 / 4, gws_y / 2, gws_y * 3 / 4},
                                      {gws_x * 3 / 4, gws_x, gws_y / 2, gws_y * 3 / 4},

                                      {0, gws_x / 4, gws_y * 3 / 4, gws_y},
                                      {gws_x / 4, gws_x / 2, gws_y * 3 / 4, gws_y},
                                      {gws_x / 2, gws_x * 3 / 4, gws_y * 3 / 4, gws_y},
                                      {gws_x * 3 / 4, gws_x, gws_y * 3 / 4, gws_y}};

        pthread_t tid[16];
        for (t = 0; t < 16; t++)
        {
            pthread_create(&tid[t], NULL, execute_fractal_cpu, &t_args[t]);
        }
        for (t = 0; t < 16; t++)
        {
            pthread_join(tid[t], NULL);
        }
    }
    tp2 = get_time_usec();
    cpu_execution = tp2 - tp1;
}

unsigned long calculate_avg_time(unsigned long* exec_time)
{
    unsigned long avg;
#ifdef OPENCL_SUPPORT
    if (cur_dev)
    {
        *exec_time = ocl_devices[current_device].execution;
        avg = gpu_iter ? gpu_executions / gpu_iter : 0;
    }
    else
    {
        *exec_time = cpu_execution;
        avg = cpu_iter ? cpu_executions / cpu_iter : 0;
    }

#else
    *exec_time = cpu_execution;
    avg = cpu_iter ? cpu_executions / cpu_iter : 0;
#endif
    return avg;
}

void draw_right_panel(int column)
{
    int row = 0;
    unsigned long exec_time;
    unsigned long avg, r_avg1, r_avg2;

    draw_box(WIDTH, 0, RIGTH_PANEL_WIDTH, HEIGHT, 0, 0, 60);

    draw_string(row++, "===", " Main ====");
    draw_int(row++, "F1-F8 fractal (F9:mod1)", fractal);
#ifdef OPENCL_SUPPORT
    draw_int(row++, "v device", cur_dev);
#endif
    draw_double(row++, "lx", lx);
    draw_double(row++, "rx", rx);
    draw_double(row++, "ty", ty);
    draw_double(row++, "by", by);

    draw_string(row++, "==", " Parameters ===");
    draw_int(row++, "i/I iter", max_iter);
    draw_string(row++, "1", " show iter. histogram");
    draw_double(row++, "e/E er", er);
    draw_double(row++, "x/X c_x", c_x);
    draw_double(row++, "y/Y c_y", c_y);
    /*#ifdef OPENCL_SUPPORT
        draw_int(row++, "2/3 gws_x", gws_x);
        draw_int(row++, "2/3 gws_y", gws_y);
    #endif*/
    draw_double(row++, "zoom", zoom);
    draw_double(row++, "LB/RB zx", zx);
    draw_double(row++, "LB/RB zy", zy);

    draw_string(row++, "P ", " Colors ==");
    draw_int(row++, "p pal", pal);

    draw_hex(row++, "c/C channel", color_channel);
    draw_hex(row++, "+/= rgb +-1", rgb);
    draw_hex(row++, "_/- rgb +-16", rgb);
    draw_hex(row++, "m/M mm +-1", mm);

    draw_double(row++, "h/H c1", c1[color_channel]);
    draw_double(row++, "j/J c2", c2[color_channel]);
    draw_double(row++, "k/K c3", c3[color_channel]);
    draw_double(row++, "l/L c4", c4[color_channel]);

    draw_string(row++, "===", " Moves ====");
    draw_double(row++, "Left/Right szx", szx);
    draw_double(row++, "Down/Up szy", szy);
    draw_double(row++, "*// szx", szx);
    draw_double(row++, "*// szy", szy);
    draw_double(row++, "a/d dx", dx);
    draw_double(row++, "w/s dy", dy);

    draw_string(row++, "SPACE", " Benchmarking [us]");
    draw_long(row++, "frame", frames_time / draw_frames);
    avg = calculate_avg_time(&exec_time);

    draw_2long(row++, "exec", exec_time, "avg", avg);
    last_avg_result = avg;

    r_avg1 = flips ? prepare_times / flips : 0;
    draw_2long(row++, "prepare", prepare_time, "avg", r_avg1);

    r_avg2 = flips ? render_times / flips : 0;
    draw_2long(row++, "render", render_time, "avg", r_avg2);

    if (performance_test)
    {
        unsigned long res = 0;
        SDL_Rect dst;

        dst.w = 5;
        dst.x = column;
        dst.y = 0;

        if (r_avg1 + r_avg2)
        {
            res = (HEIGHT_FL - 40) * (frames_time / draw_frames) / (r_avg1 + r_avg2);
        }
        dst.h = res;

        SDL_SetRenderDrawColor(main_window, 255, 128, 128, 255);
        SDL_RenderFillRect(main_window, &dst);

        if (r_avg1 + r_avg2)
        {
            res = (HEIGHT_FL - 40) * avg / (r_avg1 + r_avg2);
        }
        dst.h = res;
        if (dst.x + 5 < WIDTH) dst.x += 5;

        SDL_SetRenderDrawColor(main_window, 128, 255, 128, 255);
        SDL_RenderFillRect(main_window, &dst);
    }
}

void show_iterations_window()
{
    int x, y, max_x, max_p = WIDTH * HEIGHT / 16;
    unsigned int* iter_map;
    unsigned int iter;

    unsigned char* pixels;
    int pitch;
    SDL_Rect window_rec;

    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    prepare_cpu_args();

    max_x = max_iter > WIDTH ? WIDTH : max_iter;
    iter_map = calloc(max_x, sizeof(iter));

    for (y = 0; y < HEIGHT / 4; y++)
    {
        for (x = 0; x < WIDTH / 4; x++)
        {
            iter = calculate_one_pixel(x, y);
            if (iter < max_x)
            {
                iter_map[iter]++;
            }
        }
    }

    SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

    for (x = 0; x < max_x; x++)
    {
        int ny;
        int ly;
        y = roundf(1.0f * (HEIGHT - 1) * iter_map[x] / max_p);
        for (ly = 0; ly < y; ly++)
        {
            ny = pitch * ly;
            pixels[ny + x * 4] = 255;
            pixels[ny + x * 4 + 1] = 255;
            pixels[ny + x * 4 + 2] = 255;
            pixels[ny + x * 4 + 3] = 255;
        }
    }
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(main_window, texture, NULL, &window_rec);
    free(iter_map);
}

void show_palette()
{
    if (pal)
    {
        unsigned char* pixels;
        int pitch, x, y;
        int nx, ny;

        SDL_Rect window_rec;
        SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

        window_rec.w = WIDTH;
        window_rec.h = HEIGHT;
        window_rec.x = 0;
        window_rec.y = 0;
        memset(pixels, 0, pitch * HEIGHT);
        for (y = -255; y < 256; y++)
        {
            for (x = -255; x < 256; x++)
            {
                int r = sqrt(x * x + y * y);
                if (r < 256)
                {
                    r = 255 - r;
                    nx = (256 + x) << 2;
                    ny = pitch * (256 + y);
                    pixels[ny + nx + 2] |= (r | (rgb & 255));
                    pixels[ny + nx + 3] = 255;

                    nx = (384 + x) << 2;
                    ny = pitch * (256 + y);
                    pixels[ny + nx + 1] |= (r | ((rgb & 0x00ff00) >> 8));
                    pixels[ny + nx + 3] = 255;

                    nx = (320 + x) << 2;
                    ny = pitch * (384 + y);
                    pixels[ny + nx] |= (r | ((rgb & 0xff0000) >> 16));
                    pixels[ny + nx + 3] = 255;
                }
            }
        }
        SDL_UnlockTexture(texture);
        SDL_RenderCopy(main_window, texture, NULL, &window_rec);
    }
    else
    {
        int x;
        SDL_Rect dst;

        dst.w = WIDTH;
        dst.x = 0;
        dst.y = 0;
        dst.h = HEIGHT;

        SDL_SetRenderDrawColor(main_window, 0, 0, 0, 255);
        SDL_RenderFillRect(main_window, &dst);

        for (x = 0; x < 720; x++)
        {
            unsigned int c = colors[x];
            int r = (c & 0xff0000) >> 16;
            int g = (c & 0x00ff00) >> 8;
            int b = c & 0xff;
            SDL_SetRenderDrawColor(main_window, r, g, b, 255);
            SDL_RenderDrawLine(main_window, x, 0, x, HEIGHT / 2);

            c |= rgb;
            r = (c & 0xff0000) >> 16;
            g = (c & 0x00ff00) >> 8;
            b = c & 0xff;
            SDL_SetRenderDrawColor(main_window, r, g, b, 255);
            SDL_RenderDrawLine(main_window, x, HEIGHT / 2, x, HEIGHT - 40);
        }
    }
    SDL_RenderPresent(main_window);
}

void prepare_frames()
{
    unsigned long tp1, tp2;
    int pixel;
    frames_time = 0;

    for (pixel = 0; pixel < draw_frames; pixel++)
    {
        tp1 = get_time_usec();
#ifdef OPENCL_SUPPORT
        if (cur_dev)
        {
            start_ocl();
            gpu_executions += ocl_devices[current_device].execution;
            gpu_iter++;
        }
        else
#endif
        {
            start_cpu();
            cpu_executions += cpu_execution;
            cpu_iter++;
        }
        tp2 = get_time_usec();
        frames_time += tp2 - tp1;
    }
}

void show_perf_result()
{
    puts("***********************************************************");
#ifdef OPENCL_SUPPORT
    if (cur_dev)
    {
        show_ocl_device(current_device);
    }
#endif
    printf("Kernel: ");
    fflush(stdout);
    system("uname -r");
    printf("--- CPU info ---\n");
    fflush(stdout);
    system("lscpu");
    printf("--- performance results --- \n");
    printf("avg exec time: %lu [us]\n", last_avg_result);
    puts("***********************************************************");
}

int draw = 1;
int flip_window;
int stop_animation = 1;
int animate;
int column;
double m1x, m1y;
int key;

void draw_palettes()
{
    draw = 0;
    flip_window = 0;
    stop_animation = 0;
    performance_test = 0;
    draw_right_panel(column);
    show_palette();
}

void outside()
{
    dx = 0;
    dy = 0;
    draw = 0;
    animate = 0;
    draw_frames = 16;
    stop_animation = 1;
}

void draw_fractals()
{
    float m2x, m2y;
    unsigned long tp1, tp2, tp3;
    SDL_Rect window_rec;

    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    flip_window = 0;

    tp1 = get_time_usec();
    prepare_frames();
    tp2 = get_time_usec();

#ifdef OPENCL_SUPPORT
    if (cur_dev)
    {
        update_gpu_texture();
    }
    else
#endif
    {
        void* pixels;
        int pitch;
        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        memcpy(pixels, cpu_pixels, pitch * HEIGHT);
        SDL_UnlockTexture(texture);
    }
    SDL_RenderCopy(main_window, texture, NULL, &window_rec);

    m2x = equation(m1x, 0.0f, lx, WIDTH_FL, rx);
    m2y = equation(m1y, 0.0f, ty, HEIGHT_FL, by);

    draw_right_panel(column);
    if (performance_test)
    {
        column++;
        column %= WIDTH;
    }

    prepare_cpu_args();
    sprintf(status_line, "[%2.20f,%2.20f] %s: %s iter=%d mod1=%d", m2x, m2y, cur_dev ? "OCL" : "CPU", fractals_names[fractal],
            calculate_one_pixel(m1x / 4, m1y / 4), mod1);
    write_text(status_line, 0, HEIGHT - FONT_SIZE);
#ifdef OPENCL_SUPPORT
    if (cur_dev)
    {
        sprintf(status_line, "OCL[%d/%d]: %s %s ", current_device + 1, nr_devices, ocl_devices[current_device].name,
                ocl_devices[current_device].fp64 ? "fp64" : "fp32");
        write_text(status_line, 0, HEIGHT - 2 * FONT_SIZE);
    }
#endif
    if (show_iterations)
    {
        show_iterations_window();
    }

    SDL_RenderPresent(main_window);
    tp3 = get_time_usec();

    prepare_time = tp2 - tp1;
    prepare_times += prepare_time;

    render_time = tp3 - tp2;
    render_times += render_time;
    flips++;
    // printf("prepare time=%lu\n", tp2 - tp1);
    // printf("render time=%lu\n", tp3 - tp2);
}

int keyboard_event(SDL_Event* event)
{
    int kl = event->key.keysym.sym;
    //                printf("mod=%d\n", event->key.keysym.mod);
    switch (kl)
    {
    case SDLK_ESCAPE:
        return 1;
    case SDLK_F1:
    case SDLK_F2:
    case SDLK_F3:
    case SDLK_F4:
    case SDLK_F5:
    case SDLK_F6:
    case SDLK_F7:
    case SDLK_F8:
        select_fractals(kl);
        break;
    case SDLK_F9:
        mod1 ^= 1;
        break;
    case 'i':
    case 'e':
        //              case '2':
        //              case '3':
        change_fractal_params(kl, event->key.keysym.mod);
        break;
    case 'c':
    case '=':
    case '-':
    case 'm':
    case 'p':
    case 'h':
    case 'j':
    case 'k':
    case 'l':
        change_fractal_colors(kl, event->key.keysym.mod);
        break;
    case SDLK_LEFT:
    case SDLK_RIGHT:
    case SDLK_DOWN:
    case SDLK_UP:
    case 'a':
    case 'd':
    case 's':
    case 'w':
    case '8':
    case '/':
    case 'x':
    case 'y':
        key = move_fractal(kl, event->key.keysym.mod);
        break;
    case '1':
        show_iterations ^= 1;
        break;
#ifdef OPENCL_SUPPORT
    case 'v':

        cur_dev++;
        if (cur_dev > nr_devices)
        {
            cur_dev = 0; // switch to CPU
            iter_limit = 43000000000000LL;
        }
        if (cur_dev)
        { // for OCL devices
            current_device = cur_dev - 1;
            iter_limit = ocl_devices[current_device].fp64 ? 43000000000000LL : 300000;
        }
        clear_counters();
        break;
#endif
    case SDLK_SPACE:
        performance_test ^= 1;
        if (performance_test)
        {
            clear_counters();
            column = 0;
        }
        else
        {
            show_perf_result();
        }
        break;
    }
    draw = 1;
    draw_frames = 16;

    return 0;
}

void mouse_event(SDL_Event* event)
{
    static int button = 0;

    if (event->type == SDL_MOUSEMOTION)
    {
        if (event->button.x > WIDTH) return;
        m1x = event->button.x;
        m1y = event->button.y;
        if (!performance_test)
        {
            flip_window = 1;
            draw_frames = 1;
        }
    }

    if (event->type == SDL_MOUSEBUTTONDOWN)
    {
        if (event->button.x > WIDTH) return;
        if (event->button.button == 2)
        {
            zx = 1.0;
            zy = 1.0;

            draw = 0;
            animate = 0;
            draw_frames = 16;
            stop_animation = 1;
            return;
        }

        mx = equation(event->button.x, 0, ofs_lx, WIDTH, ofs_rx);
        my = equation(event->button.y % (HEIGHT), 0, ofs_ty, HEIGHT, ofs_by);

        if (event->button.button == 3)
        {
            if (button != event->button.button)
            {
                zx = 1.0;
                zy = 1.0;
                stop_animation = 1;
                draw_frames = 16;
            }
            else
            {
                zx += 0.001;
                zy += 0.001;
            }
            animate = 1;
            draw_frames = 1;
        }

        if (event->button.button == 1)
        {
            if (button != event->button.button)
            {
                zx = 1.0;
                zy = 1.0;
                stop_animation = 1;
                draw_frames = 16;
            }
            else
            {
                zx -= 0.001;
                zy -= 0.001;
            }
            animate = 1;
            draw_frames = 1;
        }
        button = event->button.button;

        draw = 1;
    }
    if (event->type == SDL_MOUSEBUTTONUP)
    {
        //   animate = 0;
    }
}

int handle_events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        //			printf("event = %d\n", event.type);
        if (event.type == SDL_WINDOWEVENT)
        {
            draw = 1;
            draw_frames = 16;
        }

        if (event.type == SDL_QUIT) return 1;
        if (event.type == SDL_KEYDOWN)
        {
            if (keyboard_event(&event)) return 1;
        }
        if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN) mouse_event(&event);
    }
    return 0;
}

void gui_loop()
{
    /*    unsigned long tp1=0, tp2=0;
        unsigned long tms;*/
    init_window();

    while (1)
    {

        if (palette) draw_palettes();
        if (animate || key)
        {
            if (calculate_offsets()) outside();
        }
        if (fractal == DRAGON || fractal == JULIA_FULL) draw_frames = 1;
        if (draw || stop_animation)
        {
            stop_animation = 0;
            flip_window = 1;
        }
        else
        {
            SDL_Delay(1);
        }

        if (flip_window || performance_test || show_iterations)
        {
            //            tp1 = get_time_usec();
            draw_fractals();
            /*          tp2 = get_time_usec();
                      tms = (tp2 - tp1);
                      printf("time = %lu fps=%lu\n", tms, tms ? 1000000/tms : 0);*/
        }
        if (!animate) draw = 0;
        if (key)
        {
            draw = 0;
            key = 0;
        }

        if (handle_events()) break;
    }
}

void run_test()
{
    unsigned long exec_time;
#ifdef OPENCL_SUPPORT
    if (cur_dev)
    {
        printf("starting performance test with %u iterations for device: %d\n", draw_frames, current_device);
    }
    else
#endif
    {
        printf("starting performance test with %u iterations on CPU\n", draw_frames);
    }
    prepare_frames();
    last_avg_result = calculate_avg_time(&exec_time);
    show_perf_result();
    clear_counters();
}

void perf_test()
{
#ifdef OPENCL_SUPPORT
    int d;

    if (all_devices)
    {
        for (d = 0; d < nr_devices; d++)
        {
            current_device = d;
            run_test();
        }
    }
    else
#endif
    {
        run_test();
    }
}

void run_program(enum app_modes app_mode, int device)
{
#ifdef OPENCL_SUPPORT
    if (init_ocl())
    {
        printf("OpenCL device not found, using CPU only\n");
    }
    else
    {
        if (app_mode == APP_DISC)
        {
            show_ocl_devices();
            return;
        }
        if (app_mode == APP_TEST)
        {
            if (device == -2)
            {
                cur_dev = 0;
                current_device = 0;
            }
            else
            {
                cur_dev = 1; // use first OCL device
                if (device >= 0 && device < nr_devices) current_device = device;
            }
        }
        else
        {
            cur_dev = 1; // use first OCL device
            if (device >= 0 && device < nr_devices) current_device = device;
        }
        iter_limit = ocl_devices[current_device].fp64 ? 43000000000000LL : 300000;
        if (pthread_mutex_init(&lock_fin, NULL)) return;
        if (pthread_cond_init(&cond_fin, NULL)) return;
    }
#endif
    if (initialize_colors()) return;
    if (posix_memalign((void**)&cpu_pixels, 4096, IMAGE_SIZE)) return;

    if (!console_mode)
    {
        gui_loop();
    }
    else
    {
        perf_test();
    }
#ifdef OPENCL_SUPPORT
    finish_thread = 1;
    if (nr_devices)
    {
        pthread_mutex_destroy(&lock_fin);
        pthread_cond_destroy(&cond_fin);
    }
    close_ocl();
#endif

    if (!console_mode) SDL_Quit();
}

void help()
{
#ifdef OPENCL_SUPPORT
    puts("-dn - select n OpenCL device");
    puts("-c  - run performance test on CPU");
    puts("-l  - list OpenCL devices");
    puts("-a  - test all OpenCL devices");
#endif
    puts("-t  - run performance test on GPU/CPU");
    puts("-i  - number of iterations in performance test");
    puts("-q  - quiet mode - disable logs");
    puts("-h  - show help");
    puts("-v  - show version");
    puts("-fn - select n fractal type");
    puts("where n:");
    puts("      0 - julia");
    puts("      1 - mandelbrot");
    puts("      2 - julia (full)");
    puts("      3 - dragon");
    puts("      4 - julia 3");
    puts("      5 - burning ship");
    puts("      6 - generalized celtic");
    exit(0);
}

int main(int argc, char* argv[])
{
    int opt;
    int device = -1;
    enum app_modes app_mode = APP_GUI;
    int f;
    int iter = 32000;
#ifdef OPENCL_SUPPORT
    while ((opt = getopt(argc, argv, "d:tlhi:qaf:vc")) != -1)
#else
    while ((opt = getopt(argc, argv, "thi:qf:v")) != -1)
#endif
    {
        switch (opt)
        {
#ifdef OPENCL_SUPPORT
        case 'd':
            if (device == -2)
            {
                puts("can't use -d and -c together");
                return 1;
            }
            device = strtoul(optarg, NULL, 0);
            printf("selected device: %d\n", device);
            break;
        case 'l':
            app_mode = APP_DISC;
            break;
        case 'a':
            all_devices = 1;
            break;
        case 'c':
            app_mode = APP_TEST;
            performance_test = 1;
            console_mode = 1;
            if (device != -1)
            {
                puts("can't use -d and -c together");
                return 1;
            }
            device = -2;
            break;
#endif
        case 't':
            app_mode = APP_TEST;
            performance_test = 1;
            console_mode = 1;
            break;
        case 'i':
            iter = strtoul(optarg, NULL, 0);
            if (iter < 16) draw_frames = 16;
            break;
        case 'h':
            help();
            break;
        case 'q':
            quiet = 1;
            break;
        case 'f':
            f = strtoul(optarg, NULL, 0);
            if (f < 0) f = 0;
            if (f >= NR_FRACTALS) f = 0;
            select_fractal(f);
            break;
        case 'v':
            printf("FractalCL version: %s\n", STRING_MACRO(VERSION));
            return 0;
        }
    }
    if (console_mode && app_mode == APP_TEST)
    {
        draw_frames = iter;
    }
    srandom(time(0));

    run_program(app_mode, device);
    return 0;
}
