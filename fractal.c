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
#include "kernels/dragon.cl"
#include "kernels/generalized_celtic.cl"
#include "kernels/julia.cl"
#include "kernels/julia3.cl"
#include "kernels/julia_full.cl"
#include "kernels/mandelbrot.cl"

#include "parameters.h"

unsigned int* colors;
void* cpu_pixels;

char status_line[200];

int draw_frames = 16;

#ifdef OPENCL_SUPPORT
extern pthread_cond_t cond_fin;
extern pthread_mutex_t lock_fin;
extern volatile int tasks_finished;
#endif

int performance_test;
unsigned long last_avg_result;

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
            float h1 = iter / 60.0;
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

void* execute_fractal_cpu(void* c)
{
    int x, y;
    struct cpu_args* cpu = (struct cpu_args*)c;

    FP_TYPE ofs_lx1 = (ofs_lx + dx) / szx;
    FP_TYPE ofs_rx1 = (ofs_rx + dx) / szx;
    FP_TYPE ofs_ty1 = (ofs_ty + dy) / szy;
    FP_TYPE ofs_by1 = (ofs_by + dy) / szy;

    lx = ofs_lx1;
    rx = ofs_rx1;
    ty = ofs_ty1;
    by = ofs_by1;

    FP_TYPE step_x = (ofs_rx1 - ofs_lx1) / WIDTH_FL;
    FP_TYPE step_y = (ofs_by1 - ofs_ty1) / HEIGHT_FL;

    for (y = cpu->ys; y < cpu->ye; y++)
    {
        for (x = cpu->xs; x < cpu->xe; x++)
        {
            switch (fractal)
            {
            case JULIA:
                julia(cpu->ofs_x + x * 4, cpu->ofs_y + y * 4, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z, c_x, c_y);
                break;
            case JULIA3:
                julia3(cpu->ofs_x + x * 4, cpu->ofs_y + y * 4, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z, c_x, c_y);
                break;
            case JULIA_FULL:
                julia_full(x, y, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z, c_x, c_y);
                break;
            case MANDELBROT:
                mandelbrot(cpu->ofs_x + x * 4, cpu->ofs_y + y * 4, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z);
                break;
            case BURNING_SHIP:
                burning_ship(cpu->ofs_x + x * 4, cpu->ofs_y + y * 4, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z);
                break;
            case GENERALIZED_CELTIC:
                generalized_celtic(cpu->ofs_x + x * 4, cpu->ofs_y + y * 4, cpu_pixels, colors, mm, ofs_lx, step_x, ofs_ty, step_y, er, max_iter, pal, show_z);
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
}

void draw_right_panel(int column)
{
    int raw = 0;
    unsigned long exec_time;
    unsigned long avg, r_avg;

    draw_string(raw++, "===", " Main ====");
    draw_int(raw++, "F1-F7 fractal", fractal);
#ifdef OPENCL_SUPPORT
    draw_int(raw++, "v device", cur_dev);
#endif
    draw_int(raw++, "1 show_z", show_z);
    draw_double(raw++, "lx", lx);
    draw_double(raw++, "rx", rx);
    draw_double(raw++, "ty", ty);
    draw_double(raw++, "by", by);

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
    draw_string(raw++, "h pal", (pal) ? "HSV" : "RGB");
    if (!pal)
    {
        draw_hex(raw++, "o/p channel", color_channel);
        draw_hex(raw++, "k/l +-1", mm);
        draw_hex(raw++, "n/m +-16", mm);
    }

    raw++;
    draw_string(raw++, "===", " Moves ====");
    draw_double(raw++, "Left/Right szx", szx);
    draw_double(raw++, "Down/Up szy", szy);
    draw_double(raw++, ",/. szx", szx);
    draw_double(raw++, ",/. szy", szy);
    draw_double(raw++, "a/d dx", dx);
    draw_double(raw++, "w/s dy", dy);

    raw++;
    draw_string(raw++, "SPACE", " Benchmarking [us]");
    draw_long(raw++, "time", frames_time / draw_frames);
#ifdef OPENCL_SUPPORT
    if (cur_dev)
    {
        exec_time = ocl_devices[current_device].execution;
        avg = gpu_iter ? gpu_executions / gpu_iter : 0;
    }
    else
    {
        exec_time = cpu_execution;
        avg = cpu_iter ? cpu_executions / cpu_iter : 0;
    }

#else
    exec_time = cpu_execution;
    avg = cpu_iter ? cpu_executions / cpu_iter : 0;
#endif
    draw_2long(raw++, "exec", exec_time, "avg", avg);
    last_avg_result = avg;
    r_avg = flips ? render_times / flips : 0;
    draw_2long(raw++, "render", render_time, "avg", r_avg);

    if (performance_test)
    {
        unsigned long res = 0;
        SDL_Rect dst;

        dst.w = 5;
        dst.x = column;
        dst.y = 0;

        if (r_avg)
        {
            res = (HEIGHT_FL - 40) * (frames_time / draw_frames) / r_avg;
        }
        dst.h = res;

        SDL_SetRenderDrawColor(main_window, 255, 128, 128, 255);
        SDL_RenderFillRect(main_window, &dst);

        if (r_avg)
        {
            res = (HEIGHT_FL - 40) * avg / r_avg;
        }
        dst.h = res;
        if (dst.x + 5 < WIDTH) dst.x += 5;

        SDL_SetRenderDrawColor(main_window, 128, 255, 128, 255);
        SDL_RenderFillRect(main_window, &dst);
    }
}

void show_pallete()
{
    int x;
    for (x = 0; x < 720; x++)
    {
        int r = (colors[x] & 0xff0000) >> 16;
        int g = (colors[x] & 0x00ff00) >> 8;
        int b = colors[x] & 0xff;
        SDL_SetRenderDrawColor(main_window, r, g, b, 255);
        SDL_RenderDrawLine(main_window, x, 0, x, HEIGHT - 40);
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
    int column = 0;

#ifdef OPENCL_SUPPORT
    if (init_ocl())
    {
        printf("OpenCL device not found, using CPU only\n");
    }
    else
    {
        cur_dev = 1;
        iter_limit = ocl_devices[current_device].fp64 ? 43000000000000LL : 300000;
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
            if (calculate_offsets())
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
        if (pal == 2)
        {
            draw = 0;
            flip_window = 0;
            stop_animation = 0;
            performance_test = 0;
            show_pallete();
        }

        if (draw || stop_animation)
        {
            stop_animation = 0;
            flip_window = 1;
        }
        else
        {
            SDL_Delay(1);
        }

        if (flip_window || performance_test)
        {
            float m2x, m2y;
            unsigned long tp1, tp2;

            flip_window = 0;
            tp1 = get_time_usec();

            prepare_frames();
#ifdef OPENCL_SUPPORT
            if (cur_dev)
            {
                update_gpu_texture();
            }
            else
#endif
            {
                SDL_UpdateTexture(texture, NULL, cpu_pixels, WIDTH * 4);
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
            render_times += render_time;
            flips++;
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
                    dec_int(&max_iter, 10, 10, 1);
                    break;
                case 'i':
                    inc_int(&max_iter, 1, 1);
                    break;
                case 'p':
                    color_channel++;
                    if (color_channel > 2) color_channel = 2;
                    break;
                case 'o':
                    color_channel--;
                    if (color_channel < 0) color_channel = 0;
                    break;
                case 'l':
                    mm += 1 << (color_channel * 8);
                    break;
                case 'k':
                    mm -= 1 << (color_channel * 8);
                    break;
                case 'm':
                    mm += 16 << (color_channel * 8);
                    break;
                case 'n':
                    mm -= 16 << (color_channel * 8);
                    break;

                case SDLK_LEFT:
                    dec_float(&szx, 0.01 / zoom, 0.1, 1);
                    key = 1;
                    break;
                case SDLK_RIGHT:
                    inc_float(&szx, 0.01 / zoom, 1);
                    key = 1;
                    break;
                case SDLK_DOWN:
                    dec_float(&szy, 0.01 / zoom, 0.1, 1);
                    key = 1;
                    break;
                case SDLK_UP:
                    inc_float(&szy, 0.01 / zoom, 1);
                    key = 1;
                    break;
                case 'a':
                    inc_float(&dx, -0.1 / zoom, 0);
                    key = 1;
                    break;
                case 'd':
                    inc_float(&dx, 0.1 / zoom, 0);
                    key = 1;
                    break;
                case 's':
                    inc_float(&dy, -0.1 / zoom, 0);
                    key = 1;
                    break;
                case 'w':
                    inc_float(&dy, 0.1 / zoom, 0);
                    key = 1;
                    break;
                case 'z':
                    dec_float(&er, 0.1, 0.1, 1);
                    break;
                case 'x':
                    inc_float(&er, 0.1, 1);
                    break;
                case SDLK_COMMA:
                    inc_float(&szx, 1.0, 0);
                    inc_float(&szy, 1.0, 1);
                    key = 1;
                    break;
                case SDLK_PERIOD:
                    dec_float(&szx, 1.0, 1.0, 0);
                    dec_float(&szy, 1.0, 1.0, 0);
                    key = 1;
                    break;
                case 'h':
                    pal -= 1;
                    draw_box(WIDTH, 0, RIGTH_PANEL_WIDTH, HEIGHT, 0, 0, 60);
                    if (pal < 0) pal = 2;
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
                    clear_counters();
                    break;
                case '3':
                    gws_x /= 2;
                    if (gws_x < 8) gws_x = 8;
                    gws_y /= 2;
                    if (gws_y < 8) gws_y = 8;
                    printf("gws: x=%d y=%d\n", gws_x, gws_y);
                    clear_counters();
                    break;
                case '-':
                    c_y -= 0.001;
                    break;
                case '=':
                    c_y += 0.001;
                    break;
                case SDLK_F1:
                    set_fractal(JULIA, 4);
                    break;
                case SDLK_F2:
                    set_fractal(MANDELBROT, 4);
                    break;
                case SDLK_F3:
                    set_fractal(JULIA_FULL, 1);
                    break;
                case SDLK_F4:
                    set_fractal(DRAGON, 1);
                    max_iter = 10000;
                    er = 0.9f;
                    ofs_lx = 0.0f;
                    ofs_ty = 0.0f;
#ifdef OPENCL_SUPPORT
                    clear_pixels_ocl();
#endif
                    clear_counters();
                    break;
                case SDLK_F5:
                    set_fractal(JULIA3, 4);
                    break;
                case SDLK_F6:
                    set_fractal(BURNING_SHIP, 4);
                    break;
                case SDLK_F7:
                    set_fractal(GENERALIZED_CELTIC, 4);
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
                        printf("performance result for %s = %lu\n",
#ifdef OPENCL_SUPPORT
                               cur_dev ? ocl_devices[current_device].name : "CPU",
#else
                               "CPU",
#endif
                               last_avg_result);
                    }
                    break;
                }
                draw = 1;
                draw_frames = 16;
            }

            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                m1x = event.button.x;
                m1y = event.button.y;
                if (!performance_test)
                {
                    flip_window = 1;
                    draw_frames = 1;
                }
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
