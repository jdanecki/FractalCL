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
#include <math.h>
#include <stdlib.h>

unsigned int* colors;

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
    int err, iter;
#ifdef OPENCL_SUPPORT
    int d;
#endif
    // h [0..359]
    // s [1]
    // v [0, 1]
    err = posix_memalign((void**)&colors, 4096, 4096);
    if (err) return 1;

    for (iter = 0; iter < 360; iter++)
    {
        float h1 = iter / 60.0;
        float v1 = 1.0;
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

        unsigned int c = 0xff000000 | r << 16 | g << 8 | b;
        colors[iter] = c;
        colors[359 * 2 - iter] = c;
    }
    colors[0] = 0;
    colors[360] = (colors[359] + colors[361]) / 2;
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

unsigned int get_color(int c) { return colors[c % 360]; }

void show_hsv_palette()
{
    int x;
    SDL_Rect dst;

    dst.w = WIDTH;
    dst.x = 0;
    dst.y = 0;
    dst.h = HEIGHT;

    SDL_SetRenderDrawColor(main_window, 0, 0, 0, 255);
    SDL_RenderFillRect(main_window, &dst);

    for (x = 0; x < WIDTH; x++)
    {
        unsigned int c = get_color(x);
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

void show_rgb_palette1()
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
                r = (255 - r) * mm;
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

void show_rgb_palette2()
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
            unsigned int r = sqrt(x * x + y * y);
            if (r < 256)
            {
                float cf;
                r = 255 - r;
                cf = 1.0 * r / 255.0;
                r = 255 * (c1[2] + c2[2] * cos(6.2830 * (c3[2] * cf + c4[2])));
                nx = (256 + x) << 2;
                ny = pitch * (256 + y);
                pixels[ny + nx + 2] |= r;
                pixels[ny + nx + 3] = 255;

                r = 255 * (c1[1] + c2[1] * cos(6.2830 * (c3[1] * cf + c4[1])));
                nx = (384 + x) << 2;
                ny = pitch * (256 + y);
                pixels[ny + nx + 1] |= r;
                pixels[ny + nx + 3] = 255;

                r = 255 * (c1[0] + c2[0] * cos(6.2830 * (c3[0] * cf + c4[0])));
                nx = (320 + x) << 2;
                ny = pitch * (384 + y);
                pixels[ny + nx] |= r;
                pixels[ny + nx + 3] = 255;
            }
        }
    }
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(main_window, texture, NULL, &window_rec);
}

void show_palette()
{
    switch (pal)
    {
    case 0: // HSV
        show_hsv_palette();
        break;
    case 1: // RGB
        show_rgb_palette1();
        break;
    case 2: // RGB
        show_rgb_palette2();
        break;
    }
    SDL_RenderPresent(main_window);
}

void make_postprocess(void* px1)
{
    int i;

    unsigned int* src = px1;
    unsigned int* dst = texture_pixels;

    int s = IMAGE_SIZE / 4;
    for (i = 0; i < s; i++)
    {
        unsigned char r = 255.0 * src[i] / max_iter;
        unsigned char g = 128 + 127 * sin(src[i]);
        unsigned char b = 128 + 127 * cos(src[i]);
        dst[i] = 0xff000000 | r << 16 | g << 8 | b;
    }
}
