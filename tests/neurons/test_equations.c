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
//#define F1

unsigned int* pixels;
float learn_val = 0.01;
int iter;
int learnt;

float linear(float i) { return i; }

float linear2x(float i) { return -2 * i + 1.0; }

float linear05x(float i) { return 0.5 * i + 0.5; }

float fun_v(float i)
{
    if (i < 0.5) return -2.0 * i + 1.0;
    return 2.0 * i - 1.0;
}

struct neuron_t
{
    float a, b;
    float fa, fb;
    float (*expected)(float i);
};

struct neuron_2t
{
    float a1, b1;
    float a2, b2;
    float a3, a4, b3;
    float (*expected)(float i);
};

float value(struct neuron_t* n, float i)
{
    float v = n->a * i + n->b;
    return v;
    //	if (v < 0) return 0.01;
    // return 0.01;
}

float value2(struct neuron_2t* n, float i)
{
    float v1 = i * n->a1 + n->b1;
    float v2 = i * n->a2 + n->b2;
    float v3;

    if (v1 < 0) v1 = 0;
    if (v2 < 0) v2 = 0;

    v3 = v1 * n->a3 + v2 * n->a4 + n->b3;
    if (v3 < 0) v3 = 0;
    return v3;
}

float expected(struct neuron_t* n, float i) { return n->expected(i); }

float expected2(struct neuron_2t* n, float i) { return n->expected(i); }

void draw(float fx, float fy, unsigned int color)
{
    int x, y;

    if (fx < 0) return;
    if (fy < 0) return;

    x = fx * WIDTH;
    y = fy * HEIGHT;

    if (x > WIDTH) return;
    if (y > HEIGHT) return;

    pixels[(HEIGHT - y) * WIDTH + x] = 0xff000000 | color;
}

void get_ab(float x1, float y1, float x2, float y2, float* a, float* b)
{
    if (x2 == x1)
    {
        *a = 0;
    }
    else
    {
        *a = (y2 - y1) / (x2 - x1);
    }
    *b = y1 - *a * x1;
}

void new_values(struct neuron_t* n)
{
    float x1 = (0.01 * (rand() % 100));
    float y1 = (0.01 * (rand() % 100));
    float x2 = (0.01 * (rand() % 100));
    float y2 = (0.01 * (rand() % 100));
    get_ab(x1, y1, x2, y2, &n->a, &n->b);

    get_ab(0.0, n->expected(0.0), 1.0, n->expected(1.0), &n->fa, &n->fb);
    iter = 0;
    learnt = 0;
}

void new_values2(struct neuron_2t* n)
{
    float x1 = (0.01 * (rand() % 100));
    float y1 = (0.01 * (rand() % 100));
    float x2 = (0.01 * (rand() % 100));
    float y2 = (0.01 * (rand() % 100));
    get_ab(x1, y1, x2, y2, &n->a1, &n->b1);

    x1 = (0.01 * (rand() % 100));
    y1 = (0.01 * (rand() % 100));
    x2 = (0.01 * (rand() % 100));
    y2 = (0.01 * (rand() % 100));
    get_ab(x1, y1, x2, y2, &n->a2, &n->b2);

    x1 = (0.01 * (rand() % 100));
    y1 = (0.01 * (rand() % 100));
    x2 = (0.01 * (rand() % 100));
    y2 = (0.01 * (rand() % 100));
    get_ab(x1, y1, x2, y2, &n->a3, &n->b3);
    n->a4 = -n->a3;

    iter = 0;
    learnt = 0;
}

void draw_values(struct neuron_t* n)
{
    for (float p = 0; p < 1.0;)
    {
        draw(p, value(n, p), 0xffffff);
        p += 0.001;
    }
}

void draw_values2(struct neuron_2t* n)
{
    for (float p = 0; p < 1.0;)
    {
        draw(p, value2(n, p), 0xffffff);
        p += 0.001;
    }
}

void draw_expected(struct neuron_t* n)
{
    for (float p = 0; p < 1.0;)
    {
        draw(p, expected(n, p), 0x00ff00);
        p += 0.001;
    }
}

void draw_expected2(struct neuron_2t* n)
{
    for (float p = 0; p < 1.0;)
    {
        draw(p, expected2(n, p), 0x00ff00);
        p += 0.001;
    }
}

float da(struct neuron_t* n, float i) { return learn_val * (expected(n, i) - value(n, i)) * i; }

float da2(struct neuron_2t* n, float i) { return learn_val * (expected2(n, i) - value2(n, i)) * i; }

float db(struct neuron_t* n, float i) { return learn_val * (expected(n, i) - value(n, i)); }

float db2(struct neuron_2t* n, float i) { return learn_val * (expected2(n, i) - value2(n, i)); }

void learn(struct neuron_t* n, float i)
{
    n->a += da(n, i);
    n->b += db(n, i);
}

void learn2(struct neuron_2t* n, float i)
{
    n->a1 += da2(n, i);
    n->b1 += db2(n, i);

    n->a2 += da2(n, i);
    n->b2 += db2(n, i);

    float v1 = i * n->a1 + n->b1;
    float v2 = i * n->a2 + n->b2;

    if (v1 < 0) v1 = 0;
    if (v2 < 0) v2 = 0;

    n->a3 += da2(n, v1);
    n->b3 += db2(n, v2);
}

int check(struct neuron_t* n)
{
    float v = 0.01;
    int i;
    for (i = 0; i < 100; i++)
    {
        float err = expected(n, v) - value(n, v);
        if (err * err > 0.001) return 0;
        v += 0.01;
    }
    return 1;
}

int check2(struct neuron_2t* n)
{
    float v = 0.01;
    int i;
    for (i = 0; i < 100; i++)
    {
        float err = expected2(n, v) - value2(n, v);
        if (err * err > 0.001) return 0;
        v += 0.01;
    }
    return 1;
}

int main()
{
    char status_line[200];
    int fin = 0;

    srandom(time(0));

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    struct neuron_t f1;
    struct neuron_2t f2;

    f1.expected = linear;
    new_values(&f1);

    f2.expected = fun_v;
    new_values2(&f2);

    float mx = 0, my = 0;

    SDL_Rect window_rec;

    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    while (!fin)
    {
        SDL_Event event;
        memset(pixels, 0, IMAGE_SIZE);

#ifdef F1
        draw_expected(&f1);
        draw_values(&f1);
#else
        draw_expected2(&f2);
        draw_values2(&f2);
#endif
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
                case SDLK_F1:
                    f1.expected = linear;
                    new_values(&f1);
                    break;
                case SDLK_F2:
                    f1.expected = linear2x;
                    new_values(&f1);
                    break;
                case SDLK_F3:
                    f1.expected = linear05x;
                    new_values(&f1);
                    break;
                case SDLK_F4:
                    f1.expected = fun_v;
                    new_values(&f1);
                    break;
                case SDLK_F9:
                    new_values(&f1);
                    break;
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                mx = 1.0f * event.button.x / WIDTH;
                my = 1.0f * (HEIGHT - event.button.y) / HEIGHT;
            }
        }
        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
        SDL_RenderCopy(main_window, texture, NULL, &window_rec);

        sprintf(status_line, "mx=%f my=%f ", mx, my);
        write_text(status_line, WIDTH, 0);
#ifdef F1
        sprintf(status_line, "a=%f b=%f ", f1.a, f1.b);
        write_text(status_line, WIDTH, 20);

        sprintf(status_line, "fa=%f fb=%f ", f1.fa, f1.fb);
        write_text(status_line, WIDTH, 40);
#else
        sprintf(status_line, "a1=%f b1=%f ", f2.a1, f2.b1);
        write_text(status_line, WIDTH, 20);

        sprintf(status_line, "a2=%f b2=%f ", f2.a2, f2.b2);
        write_text(status_line, WIDTH, 40);

        sprintf(status_line, "a3=%f a4=%f ", f2.a3, f2.a4);
        write_text(status_line, WIDTH, 60);

        sprintf(status_line, "b3=%f ", f2.b3);
        write_text(status_line, WIDTH, 80);
#endif
        if (!learnt)
        {
            float l = 0.01 * (rand() % 100);
#ifdef F1
            learn(&f1, l);
            learnt = check(&f1);
#else
            learn2(&f2, l);
            learnt = check2(&f2);
#endif
            iter++;
        }

        sprintf(status_line, "iter=%-5d ", iter);
        write_text(status_line, WIDTH, 100);

        SDL_RenderPresent(main_window);
    }
    SDL_Quit();

    return 0;
}
