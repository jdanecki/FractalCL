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
#include <time.h>

unsigned int* pixels;
float ofs = 0.01f;
int show_r = 1;
int show_g = 1;
int show_b = 1;
int show_palette, inc_palette;

float r1 = 0.25;
float r2 = 0.75;
float g1 = 0.5;
float b1 = 0.5;
float ofs_mult = 1.0;

void get_ab(float x1, float y1, float x2, float y2, float* a, float* b)
{
    *a = (y2 - y1) / (x2 - x1);
    *b = y1 - *a * x1;
}

float get_r(float c)
{
    float a, b;
    if (c < r1)
    {
        get_ab(0, 0.5, r1, 0, &a, &b);
        return c * a + b;
    }
    if (c < r2)
    {
        get_ab(r1, 0, r2, 1.0, &a, &b);
        return c * a + b;
    }
    get_ab(r2, 1.0, 1.0, 0.5, &a, &b);
    return c * a + b;
}

float get_g(float c)
{
    float a, b;

    if (c < g1)
    {
        get_ab(0, 1.0, g1, 0, &a, &b);
        return c * a + b;
    }
    get_ab(g1, 0, 1.0, 1.0, &a, &b);
    return c * a + b;
}

float get_b(float c)
{
    float a, b;
    if (c < b1)
    {
        get_ab(0, 0, b1, 1.0, &a, &b);
        return c * a + b;
    }
    get_ab(b1, 1.0, 1.0, 0, &a, &b);
    return c * a + b;
}

unsigned int get_color(float ci)
{
    unsigned int r = 0, g = 0, b = 0;

    if (ci < 0.0) ci *= -1.0;
    if (ci >= 1.0) ci -= truncf(ci);

    if (show_r) r = 255 * get_r(ci);
    if (show_g) g = 255 * get_g(ci);
    if (show_b) b = 255 * get_b(ci);

    return r << 16 | g << 8 | b;
}

float get_ofs(float d) { return d * (-100 + rand() % 200) / 100.0; }

float call_offset[400000];
int calls;

void set_point1(int x, int y, int w, int h, float c1, float c2, float c3, float c4)
{
    // c1   n1    c2
    // n4   n5    n2
    // c4   n3    c3
    if (w < 2 && h < 2)
    {
        pixels[x + y * WIDTH] = 0xff000000 | get_color((c1 + c2 + c3 + c4) / 4);
    }
    else
    {
        float n1, n2, n3, n4, n5, d, o;

        w /= 2;
        h /= 2;
        n1 = (c1 + c2) / 2.0;
        n2 = (c2 + c3) / 2.0;
        n3 = (c3 + c4) / 2.0;
        n4 = (c1 + c4) / 2.0;

        d = ofs_mult * (w + h) / (WIDTH + HEIGHT);

        n5 = (c1 + c2 + c3 + c4) / 4.0;
        if (!call_offset[calls])
        {
            o = get_ofs(d);
            call_offset[calls] = o;
        }
        n5 += call_offset[calls];
        calls++;

        set_point1(x, y, w, h, c1, n1, n5, n4);
        set_point1(x + w, y, w, h, n1, c2, n2, n5);
        set_point1(x + w, y + h, w, h, n5, n2, c3, n3);
        set_point1(x, y + h, w, h, n4, n5, n3, c4);
    }
}

void set_point2(int x, int y, int w, int h, float c1, float c2, float c3, float c4)
{
    // c1   n1    c2
    // n4   n5    n2
    // c4   n3    c3
    if (w < 2 && h < 2)
    {
        pixels[x + y * WIDTH] = 0xff000000 | get_color((c1 + c2 + c3 + c4) / 4);
    }
    else
    {
        float n1, n2, n3, n4, n5, d;

        w /= 2;
        h /= 2;

        d = ofs_mult * (w + h) / (WIDTH + HEIGHT);
        n5 = (c1 + c2 + c3 + c4) / 4.0 + get_ofs(d);

        n1 = (c1 + c2 + 2 * n5) / 4.0 + get_ofs(d);
        n2 = (c2 + c3 + 2 * n5) / 4.0 + get_ofs(d);
        n3 = (c3 + c4 + 2 * n5) / 4.0 + get_ofs(d);
        n4 = (c1 + c4 + 2 * n5) / 4.0 + get_ofs(d);

        set_point2(x, y, w, h, c1, n1, n5, n4);
        set_point2(x + w, y, w, h, n1, c2, n2, n5);
        set_point2(x + w, y + h, w, h, n5, n2, c3, n3);
        set_point2(x, y + h, w, h, n4, n5, n3, c4);
    }
}

void (*set_point)(int x, int y, int w, int h, float c1, float c2, float c3, float c4);

void initialize(float* c1, float* c2, float* c3, float* c4)
{
    *c1 = 0.01 * (rand() % 100);
    *c2 = 0.01 * (rand() % 100);
    *c3 = 0.01 * (rand() % 100);
    *c4 = 0.01 * (rand() % 100);
}

int main()
{
    int fin = 0;
    int x, y;
    char status_line[200];
    int mx = 0, my = 0;
    SDL_Rect window_rec;
    srandom(time(0));
    int animate = 0;
    int sc;
    float ind = 0.0;

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    float c1, c2, c3, c4;

    initialize(&c1, &c2, &c3, &c4);
    set_point = set_point1;

    int redraw = 1;

    while (!fin)
    {
        SDL_Event event;

        if (show_palette)
        {
            if (inc_palette)
                ind += ofs;
            else
                ind = 0.0;

            for (y = 0; y < HEIGHT; y++)
            {
                for (x = 0; x < WIDTH; x++)
                {
                    pixels[x + y * WIDTH] = 0xff000000 | get_color(1.0 * (x + ind) / WIDTH);
                }
            }
        }
        else
        {
            if (redraw)
            {
                calls = 0;
                set_point(0, 0, 512, 512, c1, c2, c3, c4);
                redraw--;
            }
            else
            {
                if (inc_palette)
                {
                    int rv = rand() % 20;
                    float o = (0.1 * rv) * 0.004;

                    for (y = 0; y < 512; y++)
                    {
                        for (x = 0; x < 512; x++)
                        {
                            float r, g, b;
                            int rc, gc, bc;
                            r = ((pixels[x + y * WIDTH] & 0xff0000) >> 16) / 255.0;
                            g = ((pixels[x + y * WIDTH] & 0xff00) >> 8) / 255.0;
                            b = (pixels[x + y * WIDTH] & 0xff) / 255.0;
                            r += o;
                            g += o;
                            b += o;
                            rc = r * 255;
                            rc &= 0xff;
                            gc = g * 255;
                            gc &= 0xff;
                            bc = b * 255;
                            bc &= 0xff;
                            pixels[x + y * WIDTH] = 0xff000000 | rc << 16 | gc << 8 | bc;
                        }
                    }
                }
            }
        }
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
                case 'p':
                    ofs -= 0.01;
                    break;
                case 'o':
                    ofs += 0.01;
                    break;

                case SDLK_F1:
                    show_r ^= 1;
                    break;
                case SDLK_F2:
                    show_g ^= 1;
                    break;
                case SDLK_F3:
                    show_b ^= 1;
                    break;
                case SDLK_F4:
                    show_palette ^= 1;
                    break;
                case SDLK_F5:
                    inc_palette ^= 1;
                    break;

                case SDLK_F6:
                    initialize(&c1, &c2, &c3, &c4);
                    break;

                case SDLK_F7:
                    c1 = c2 = c3 = c4 = 1.0;
                    break;
                case SDLK_F8:
                    c1 = c2 = c3 = c4 = 0.0;
                    break;

                case SDLK_F9:
                    set_point = set_point1;
                    break;
                case SDLK_F10:
                    set_point = set_point2;
                    break;
                case SDLK_F11:
                    animate ^= 1;
                    break;
                case 'r':
                    r1 -= 0.05f;
                    if (r1 < 0.01) r1 = 0.01;
                    break;
                case 't':
                    r1 += 0.05f;
                    if (r1 > 0.99) r1 = 0.99;
                    break;

                case 'y':
                    r2 -= 0.05f;
                    if (r2 < 0.01) r2 = 0.01;
                    break;
                case 'u':
                    r2 += 0.05f;
                    if (r2 > 0.99) r2 = 0.99;
                    break;

                case 'g':
                    g1 -= 0.05f;
                    if (g1 < 0.01) g1 = 0.01;
                    break;
                case 'h':
                    g1 += 0.05f;
                    if (g1 > 0.99) g1 = 0.99;
                    break;

                case 'b':
                    b1 -= 0.05f;
                    if (b1 < 0.01) b1 = 0.01;
                    break;
                case 'n':
                    b1 += 0.05f;
                    if (b1 > 0.99) b1 = 0.99;
                    break;

                case ',':
                    ofs_mult -= 0.1;
                    if (ofs_mult < 0.1) ofs_mult = 0.1;
                    break;
                case '.':
                    ofs_mult += 0.1f;
                    break;
                case '1':
                    c1 += ofs;
                    break;
                case '2':
                    c2 += ofs;
                    break;
                case '3':
                    c3 += ofs;
                    break;
                case '4':
                    c4 += ofs;
                    break;

                case '5':
                    c1 -= ofs;
                    break;
                case '6':
                    c2 -= ofs;
                    break;
                case '7':
                    c3 -= ofs;
                    break;
                case '8':
                    c4 -= ofs;
                    break;
                case SDLK_SPACE:
                    memset(call_offset, 0, sizeof(call_offset));
                    break;
                }
                printf("ofs = %f r=%d g=%d b=%d r1=%f r2=%f g1=%f b1=%f ofs_mult=%f\n", ofs, show_r, show_g, show_b, r1, r2, g1, b1, ofs_mult);
                printf("c1=%f c2=%f c3=%f c4=%f\n", c1, c2, c3, c4);
                redraw = 1;
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                mx = event.button.x;
                my = event.button.y;
                sprintf(status_line, "[%3d,%3d]=%-8x %2d ", mx, my, pixels[my * WIDTH + mx] & 0xffffff, pixels[my * WIDTH + mx] & 0xff);
                write_text(status_line, WIDTH, 0);
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
        SDL_RenderCopy(main_window, texture, NULL, &window_rec);
        SDL_RenderPresent(main_window);

        if (animate)
        {
            sc = rand() % 4;
            switch (sc)
            {
            case 0:
                c1 += ofs * (rand() % 2);
                break;
            case 1:
                c2 += ofs * (rand() % 2);
                break;
            case 2:
                c3 += ofs * (rand() % 2);
                break;
            case 3:
                c4 += ofs * (rand() % 2);
                break;
            }
            redraw = 1;
        }
        //		printf("c1=%f c2=%f c3=%f c4=%f\n", c1, c2, c3, c4);
    }
    SDL_Quit();

    return 0;
}
