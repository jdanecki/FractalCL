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

#include "kernels/dragon.cl"
#include "kernels/julia.cl"
#include "kernels/julia3.cl"
#include "kernels/julia_full.cl"
#include "kernels/mandelbrot.cl"

unsigned int* colors;
void* cpu_pixels;

FP_TYPE zx = 1.0, zy = 1.0; // zoom x, y
FP_TYPE zoom;
FP_TYPE dx, dy;               // shift left/right, down/up
FP_TYPE szx = 1.0, szy = 1.0; // scale x and y
FP_TYPE mx, my;               // mouse coordinates between [ofs_lx..ofs_rx, ofs_ty..ofs_by]

int mm = 16;
int gws_x = WIDTH / 4;
int gws_y = HEIGHT / 4;

#if 0
FP_TYPE ofs_lx = -0.7402f;//-1.5f; //0.094; //-1.5f;
FP_TYPE ofs_rx = -0.7164f; //-0.716403707741993;//1.5f; //0.096; //1.5f;
FP_TYPE ofs_ty = 0.2205f;//0.283962759342063; //1.5f; //0.504f; //1.5f;
FP_TYPE ofs_by = 0.1690f;//0.283962759341958; //-1.5f; //0.503f; //-1.5f;
#else
#define OFS_LX -1.5f
#define OFS_RX 1.5f

FP_TYPE ofs_lx = OFS_LX;
FP_TYPE ofs_rx = OFS_RX;
FP_TYPE ofs_ty = 1.5f;
FP_TYPE ofs_by = -1.5f;
#endif
FP_TYPE er = 4.0f;

int max_iter = 360;
int pal = 0; // 0=hsv 1=rgb
int show_z;
FP_TYPE c_x = 0.15f;
FP_TYPE c_y = -0.60f;
// FP_TYPE c_x = -0.7f;
// FP_TYPE c_y = 0.27015f;

char status_line[200];

int cur_dev; // 0 - CPU, 1,..nr_devices - OCL devices

enum fractals fractal = JULIA;

unsigned long render_time;
unsigned long frame_time;
int draw_frames = 16;

#ifdef OPENCL_SUPPORT
extern pthread_cond_t cond_fin;
extern pthread_mutex_t lock_fin;
extern volatile int tasks_finished;
#endif

int initialize_colors()
{
    int err, v, iter;
#ifdef OPENCL_SUPPORT
    int i;
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
            float h1 = (iter % 360) / 60.0;
            float v1 = 1.0 * v;
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
    for (i = 0; i < nr_devices; i++)
    {
        if (prepare_colors(&ocl_devices[i])) return 1;
        if (prepare_pixels(&ocl_devices[i])) return 1;
        if (prepare_thread(&ocl_devices[i])) return 1;
    }
#endif
    return 0;
}

struct cpu_args
{
    int xs, xe, ys, ye, ofs_x, ofs_y;
};

unsigned long cpu_execution;

void* execute_fractal_cpu(void* c)
{
    int x, y;
    struct cpu_args* cpu = (struct cpu_args*)c;

    FP_TYPE ofs_lx1 = (ofs_lx + dx) / szx;
    FP_TYPE ofs_rx1 = (ofs_rx + dx) / szx;
    FP_TYPE ofs_ty1 = (ofs_ty + dy) / szy;
    FP_TYPE ofs_by1 = (ofs_by + dy) / szy;

    FP_TYPE step_x = (ofs_rx1 - ofs_lx1) / WIDTH_FL;
    FP_TYPE step_y = (ofs_by1 - ofs_ty1) / HEIGHT_FL;

    for (y = cpu->ys; y < cpu->ye; y++)
    {
        for (x = cpu->xs; x < cpu->xe; x++)
        {
            switch (fractal)
            {
            case JULIA:
                julia(cpu->ofs_x + x * 4, cpu->ofs_y + y * 4, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z, c_x,
                      c_y);
                break;
            case JULIA3:
                julia3(cpu->ofs_x + x * 4, cpu->ofs_y + y * 4, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z, c_x,
                       c_y);
                break;
            case JULIA_FULL:
                julia_full(x, y, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z, c_x, c_y);
                break;
            case MANDELBROT:
                mandelbrot(cpu->ofs_x + x * 4, cpu->ofs_y + y * 4, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z);
                break;
            default:
                return NULL;
            }
        }
    }
    return NULL;
}

void start_cpu()
{
    unsigned long tp1, tp2;
    static int ofs_x = 0, ofs_y = 0;
    int t;

    ofs_x++;
    if (ofs_x == 4)
    {
        ofs_y++;
        ofs_x = 0;
    }
    if (ofs_y == 4)
    {
        ofs_x = 0;
        ofs_y = 0;
    }

    tp1 = get_time_usec();

    if (fractal == DRAGON)
    {
        memset(cpu_pixels, 0, IMAGE_SIZE);
        dragon(0, 0, cpu_pixels, colors, mm, ofs_lx, 0, ofs_ty, 0, er, max_iter, pal, show_z, c_x, c_y);
    }
    else
    {
        struct cpu_args t_args[16] = {{0, gws_x / 4, 0, gws_y / 4, ofs_x, ofs_y},
                                      {gws_x / 4, gws_x / 2, 0, gws_y / 4, ofs_x, ofs_y},
                                      {gws_x / 2, gws_x * 3 / 4, 0, gws_y / 4, ofs_x, ofs_y},
                                      {gws_x * 3 / 4, gws_x, 0, gws_y / 4, ofs_x, ofs_y},

                                      {0, gws_x / 4, gws_y / 4, gws_y / 2, ofs_x, ofs_y},
                                      {gws_x / 4, gws_x / 2, gws_y / 4, gws_y / 2, ofs_x, ofs_y},
                                      {gws_x / 2, gws_x * 3 / 4, gws_y / 4, gws_y / 2, ofs_x, ofs_y},
                                      {gws_x * 3 / 4, gws_x, gws_y / 4, gws_y / 2, ofs_x, ofs_y},

                                      {0, gws_x / 4, gws_y / 2, gws_y * 3 / 4, ofs_x, ofs_y},
                                      {gws_x / 4, gws_x / 2, gws_y / 2, gws_y * 3 / 4, ofs_x, ofs_y},
                                      {gws_x / 2, gws_x * 3 / 4, gws_y / 2, gws_y * 3 / 4, ofs_x, ofs_y},
                                      {gws_x * 3 / 4, gws_x, gws_y / 2, gws_y * 3 / 4, ofs_x, ofs_y},

                                      {0, gws_x / 4, gws_y * 3 / 4, gws_y, ofs_x, ofs_y},
                                      {gws_x / 4, gws_x / 2, gws_y * 3 / 4, gws_y, ofs_x, ofs_y},
                                      {gws_x / 2, gws_x * 3 / 4, gws_y * 3 / 4, gws_y, ofs_x, ofs_y},
                                      {gws_x * 3 / 4, gws_x, gws_y * 3 / 4, gws_y, ofs_x, ofs_y}};

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
    SDL_UpdateTexture(texture, NULL, cpu_pixels, WIDTH * 4);
}

void draw_right_panel()
{
    int raw = 0;
    unsigned long exec_time;

    draw_string(raw++, "===", " Main ====");
    draw_int(raw++, "F1-F5 fractal", fractal);
#ifdef OPENCL_SUPPORT
    draw_int(raw++, "v device", cur_dev);
#endif
    draw_int(raw++, "1 show_z", show_z);
    draw_double(raw++, "ofs_lx", ofs_lx);
    draw_double(raw++, "ofs_rx", ofs_rx);
    draw_double(raw++, "ofs_ty", ofs_ty);
    draw_double(raw++, "ofs_by", ofs_by);

    raw++;
    draw_string(raw++, "==", " Parameters ===");
    draw_int(raw++, "u/i iter", max_iter);
    draw_double(raw++, "z/x er", er);
    draw_double(raw++, "[/] c_x", c_x);
    draw_double(raw++, "-/= c_y", c_y);
#ifdef OPENCL_SUPPORT
    draw_int(raw++, "2/3 gws_x", gws_x);
    draw_int(raw++, "2/3 gws_y", gws_y);
#endif
    draw_double(raw++, "zoom", zoom);
    draw_double(raw++, "LB/RB zx", zx);
    draw_double(raw++, "LB/RB zy", zy);

    raw++;
    draw_string(raw++, "===", " Colors ====");
    draw_string(raw++, "h pal", (pal) ? "RGB" : "HSV");
    draw_hex(raw++, "o/p mm", mm);
    draw_hex(raw++, "k/l mm", mm);
    draw_hex(raw++, "n/m mm", mm);

    raw++;
    draw_string(raw++, "===", " Moves ====");
    draw_double(raw++, "Left/Right szx", szx);
    draw_double(raw++, "Down/Up szy", szy);
    draw_double(raw++, ",/. szx", szx);
    draw_double(raw++, ",/. szy", szy);
    draw_double(raw++, "a/d dx", dx);
    draw_double(raw++, "w/s dy", dy);

    raw++;
    draw_string(raw++, "=", " Benchmarking ==");
    draw_long(raw++, "time", frame_time / draw_frames);
#ifdef OPENCL_SUPPORT
    exec_time = cur_dev ? ocl_devices[current_device].execution : cpu_execution;
#else
    exec_time = cpu_execution;
#endif
    draw_long(raw++, "exec", exec_time);
    draw_long(raw++, "render", render_time);
}

unsigned long draw_one_frame()
{
    unsigned long tp1, tp2;

    tp1 = get_time_usec();
#ifdef OPENCL_SUPPORT
    if (cur_dev)
    {
        start_ocl();
    }
    else
#endif
    {
        start_cpu();
    }
    tp2 = get_time_usec();
    return tp2 - tp1;
}

void run_program()
{
    SDL_Event event;
    int button;
    int animate = 0;
    int key = 0;
    int stop_animation = 1;
    int flip_window = 0;
    double m1x, m1y;
    int draw = 1;
    SDL_Rect window_rec;
    unsigned long iter_limit = 43000000000000;

#ifdef OPENCL_SUPPORT
    if (init_ocl())
    {
        printf("OpenCL device not found, using CPU only\n");
    }
    else
    {
        cur_dev = 1;
        iter_limit = ocl_devices[current_device].fp64 ? 43000000000000 : 300000;
        if (pthread_mutex_init(&lock_fin, NULL)) return;
        if (pthread_cond_init(&cond_fin, NULL)) return;
    }
#endif
    init_window();

    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    if (initialize_colors()) return;

    if (posix_memalign((void**)&cpu_pixels, 4096, IMAGE_SIZE)) return;
    while (1)
    {
        if (animate || key)
        {
            ofs_lx = (ofs_lx - mx) * zx + mx;
            ofs_rx = (ofs_rx - mx) * zx + mx;

            ofs_ty = (ofs_ty - my) * zy + my;
            ofs_by = (ofs_by - my) * zy + my;

            zoom = (OFS_RX - OFS_LX) / (ofs_rx - ofs_lx);

            if (ofs_lx < -10 || ofs_rx > 10 || ofs_ty > 10 || ofs_by < -10 || ((OFS_RX - OFS_LX) / (ofs_rx - ofs_lx) > iter_limit))
            {
                dx = 0;
                dy = 0;

                draw = 0;
                animate = 0;
                draw_frames = 16;
                stop_animation = 1;
            }
        }
        if (fractal == DRAGON) draw_frames = 1;

        if (draw || stop_animation)
        {
            stop_animation = 0;
            flip_window = 1;
        }
        else
        {
            SDL_Delay(1);
        }

        if (flip_window)
        {
            float m2x, m2y;
            int pixel;
            unsigned long tp1, tp2;

            flip_window = 0;
            frame_time = 0;

            tp1 = get_time_usec();

            for (pixel = 0; pixel < draw_frames; pixel++)
            {
                frame_time += draw_one_frame();
            }
            SDL_RenderCopy(main_window, texture, NULL, &window_rec);

            m2x = equation(m1x, 0.0f, ofs_lx, WIDTH_FL, ofs_rx);
            m2y = equation(m1y, 0.0f, ofs_ty, HEIGHT_FL, ofs_by);

            draw_right_panel();

            sprintf(status_line, "[%2.20f,%2.20f] %s", m2x, m2y, cur_dev ? "OCL" : "CPU");
            write_text(status_line, 0, HEIGHT - FONT_SIZE);
#ifdef OPENCL_SUPPORT
            if (cur_dev)
            {
                sprintf(status_line, "OCL[%d/%d]: %s %s ", current_device + 1, nr_devices, ocl_devices[current_device].name,
                        ocl_devices[current_device].fp64 ? "fp64" : "fp32");
                write_text(status_line, 0, HEIGHT - 2 * FONT_SIZE);
            }
#endif
            SDL_RenderPresent(main_window);
            tp2 = get_time_usec();
            render_time = tp2 - tp1;
            //		    printf("render time=%lu\n", render_time);
        }
        if (!animate) draw = 0;
        if (key)
        {
            draw = 0;
            key = 0;
        }

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) goto finish;
            if (event.type == SDL_KEYDOWN)
            {
                int kl = event.key.keysym.sym;
                switch (kl)
                {
                case 27:
                    goto finish;
                case 'u':
                    max_iter -= 10;
                    if (max_iter < 10) max_iter = 10;
                    break;
                case 'i':
                    max_iter += 10;
                    break;
                case 'o':
                    mm++;
                    break;
                case 'p':
                    mm--;
                    if (!mm) mm = 1;
                    break;
                case 'k':
                    mm += 100;
                    break;
                case 'l':
                    mm -= 100;
                    if (mm < 1) mm = 1;
                    break;
                case 'n':
                    mm += 1000;
                    break;
                case 'm':
                    mm -= 1000;
                    if (mm < 1) mm = 1;
                    break;

                case SDLK_LEFT:
                    szx -= 0.01 / zoom;
                    if (szx < 0.1) szx = 0.1;
                    key = 1;
                    break;
                case SDLK_RIGHT:
                    szx += 0.01 / zoom;
                    key = 1;
                    break;
                case SDLK_DOWN:
                    szy -= 0.01 / zoom;
                    if (szy < 0.1) szy = 0.1;
                    key = 1;
                    break;
                case SDLK_UP:
                    szy += 0.01 / zoom;
                    key = 1;
                    break;
                case 'a':
                    dx -= 0.1 / zoom;
                    key = 1;
                    break;
                case 'd':
                    dx += 0.1 / zoom;
                    key = 1;
                    break;
                case 's':
                    dy -= 0.1 / zoom;
                    key = 1;
                    break;
                case 'w':
                    dy += 0.1 / zoom;
                    key = 1;
                    break;
                case 'z':
                    er -= 0.1;
                    if (er < 0.0f) er = 0.0f;
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
                    if (szy < 0.1) szy = 0.1;
                    if (szx < 0.1) szx = 0.1;
                    key = 1;
                    break;
                case 'h':
                    pal ^= 1;
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
                    if (gws_x > WIDTH) gws_x = WIDTH;
                    gws_y *= 2;
                    if (gws_y > HEIGHT) gws_y = HEIGHT;
                    printf("gws: x=%d y=%d\n", gws_x, gws_y);
                    break;
                case '3':
                    gws_x /= 2;
                    if (gws_x < 8) gws_x = 8;
                    gws_y /= 2;
                    if (gws_y < 8) gws_y = 8;
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
                    gws_x = WIDTH / 4;
                    gws_y = HEIGHT / 4;
                    max_iter = 360;
                    er = 4.0f;
                    ofs_lx = -1.5f;
                    ofs_ty = 1.5f;
                    break;
                case SDLK_F2:
                    fractal = MANDELBROT;
                    gws_x = WIDTH / 4;
                    gws_y = HEIGHT / 4;
                    max_iter = 360;
                    er = 4.0f;
                    ofs_lx = -1.5f;
                    ofs_ty = 1.5f;
                    break;
                case SDLK_F3:
                    fractal = JULIA_FULL;
                    gws_x = WIDTH;
                    gws_y = HEIGHT;
                    max_iter = 360;
                    er = 4.0f;
                    ofs_lx = -1.5f;
                    ofs_ty = 1.5f;
                    break;
                case SDLK_F4:
                    fractal = DRAGON;
                    gws_x = WIDTH;
                    gws_y = HEIGHT;
                    max_iter = 10000;
                    er = 0.9f;
                    ofs_lx = 0.0f;
                    ofs_ty = 0.0f;
#ifdef OPENCL_SUPPORT
                    clear_pixels_ocl();
#endif
                    break;
                case SDLK_F5:
                    fractal = JULIA3;
                    gws_x = WIDTH / 4;
                    gws_y = HEIGHT / 4;
                    max_iter = 360;
                    er = 4.0f;
                    ofs_lx = -1.5f;
                    ofs_ty = 1.5f;
                    break;
#ifdef OPENCL_SUPPORT
                case 'v':
                    cur_dev++;
                    if (cur_dev > nr_devices) cur_dev = 0;
                    if (cur_dev)
                    {
                        current_device = cur_dev - 1;
                    }
                    break;
#endif
                }
                draw = 1;
                draw_frames = 16;
            }

            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                m1x = event.button.x;
                m1y = event.button.y;
                flip_window = 1;
                draw_frames = 1;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                if (event.button.x > WIDTH) continue;
                if (event.button.button == 2)
                {
                    dx = 0;
                    dy = 0;

                    draw = 0;
                    animate = 0;
                    draw_frames = 16;
                    stop_animation = 1;
                    continue;
                }

                mx = equation(event.button.x, 0, ofs_lx, WIDTH, ofs_rx);
                my = equation(event.button.y % (HEIGHT), 0, ofs_ty, HEIGHT, ofs_by);

                if (event.button.button == 3)
                {
                    if (button != event.button.button)
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

                if (event.button.button == 1)
                {
                    if (button != event.button.button)
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
                button = event.button.button;

                draw = 1;
            }
            if (event.type == SDL_MOUSEBUTTONUP)
            {
                //   animate = 0;
            }
        }
    }
finish:
#ifdef OPENCL_SUPPORT
    finish_thread = 1;
    if (nr_devices)
    {
        pthread_mutex_destroy(&lock_fin);
        pthread_cond_destroy(&cond_fin);
    }
    close_ocl();
#endif
    SDL_Quit();
}

int main()
{
    srandom(time(0));

    run_program();
    return 0;
}
