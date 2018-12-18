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

// learning tests with non linear function
//  2 neurons in input layer
//  1 neuron in output layer

//#define USE_SIGMOID

#include "gui.h"
#include <time.h>

unsigned int* pixels;
float a0, b0;         // expected
float a1, b1, a2, b2; // input layer
float a3, a4, b3;     // output layer
float learning = 0.01;

void draw(float fx, float fy, unsigned int color)
{
    int x, y;

    x = (fx + 1.0) * WIDTH / 2.0;
    y = (fy + 1.0) * HEIGHT / 2.0;

    if (x > WIDTH) return;
    if (y > HEIGHT) return;
    if (x < 0) return;
    if (y < 0) return;

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

#ifdef USE_SIGMOID
float sigmoid(float i) { return 1.0 / (1.0 + exp(-i)); }
#endif

float expected(float i)
{
    if (i < 0) return a0 * i + b0;
    return -a0 * i + b0;
}

void get_values(float i, float* v1, float* v2)
{
#ifdef USE_SIGMOID
    *v1 = sigmoid(a1 * i + b1);
    *v2 = sigmoid(a2 * i + b2);
#else
    *v1 = a1 * i + b1;
    *v2 = a2 * i + b2;
    if (*v1 < 0) *v1 = 0.01;
    if (*v2 < 0) *v2 = 0.01;
#endif
}

float f2(float i)
{
    float v1, v2;
    get_values(i, &v1, &v2);

    return a3 * v1 + a4 * v2 + b3;
}

float err_out(float i) { return (expected(i) - f2(i)); }

void new_values(float* a, float* b)
{
    float x1 = (0.01 * (rand() % 100));
    float y1 = (0.01 * (rand() % 100));
    float x2 = (0.01 * (rand() % 100));
    float y2 = (0.01 * (rand() % 100));

    get_ab(x1, y1, x2, y2, a, b);
}

void learn()
{
    for (float p = -1.0; p < 1.0;)
    {
        float e = err_out(p);
        float v1, v2;
        get_values(p, &v1, &v2);

        a3 += learning * e * v1;
        a4 += learning * e * v2;
        b3 += learning * e;

        float e1 = a3 * e;
        float e2 = a4 * e;
#ifdef USE_SIGMOID
        a1 += learning * e1 * p * v1 * (1.0 - v1);
        a2 += learning * e2 * p * v2 * (1.0 - v2);
#else
        a1 += learning * e1 * p;
        a2 += learning * e2 * p;
#endif
        b1 += learning * e1;
        b2 += learning * e2;

        p += 0.1;
    }
}

int main()
{
    char status_line[200];
    int fin = 0;
    float mx = 0, my = 0;

    srandom(time(0));

    if (posix_memalign((void**)&pixels, 4096, IMAGE_SIZE)) return 1;

    init_window();

    SDL_Rect window_rec;
    window_rec.w = WIDTH;
    window_rec.h = HEIGHT;
    window_rec.x = 0;
    window_rec.y = 0;

    new_values(&a0, &b0);
    new_values(&a1, &b1);
    new_values(&a2, &b2);
    new_values(&a3, &b3);
    a4 = -a3;

    while (!fin)
    {
        SDL_Event event;
        memset(pixels, 0, IMAGE_SIZE);

        for (float p = -1.0; p < 1.0;)
        {
            draw(p, expected(p), 0x00ff00);
            draw(p, f2(p), 0xffffff);
            draw(p, err_out(p), 0xff0000);

            draw(p, 0, 0xff);
            draw(0, p, 0xff);

            p += 0.001;
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
                case SDLK_F9:
                    new_values(&a0, &b0);
                    break;
                case SDLK_F10:
                    new_values(&a1, &b1);
                    break;
                case SDLK_F11:
                    new_values(&a2, &b2);
                    break;
                case SDLK_F12:
                    new_values(&a3, &b3);
                    a4 = -a3;
                    break;
                case SDLK_F1:
                    learn();
                    break;
                case '1':
                    a1 += 0.01;
                    break;
                case '2':
                    a1 -= 0.01;
                    break;
                case '3':
                    b1 += 0.01;
                    break;
                case '4':
                    b1 -= 0.01;
                    break;

                case '5':
                    a2 += 0.01;
                    break;
                case '6':
                    a2 -= 0.01;
                    break;
                case '7':
                    b2 += 0.01;
                    break;
                case '8':
                    b2 -= 0.01;
                    break;

                case 'q':
                    a3 += 0.01;
                    break;
                case 'w':
                    a3 -= 0.01;
                    break;
                case 'e':
                    a4 += 0.01;
                    break;
                case 'r':
                    a4 -= 0.01;
                    break;
                case 't':
                    b3 += 0.01;
                    break;
                case 'y':
                    b3 -= 0.01;
                    break;
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                mx = (2.0f * event.button.x) / WIDTH - 1.0;
                my = (2.0f * (HEIGHT - event.button.y)) / HEIGHT - 1.0;
            }
        }
        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
        SDL_RenderCopy(main_window, texture, NULL, &window_rec);

        sprintf(status_line, "mx=%f my=%f ", mx, my);
        write_text(status_line, WIDTH, 0);

        sprintf(status_line, "exp=%f ", expected(mx));
        write_text(status_line, WIDTH, 20);

        sprintf(status_line, "f2=%f ", f2(mx));
        write_text(status_line, WIDTH, 40);

        sprintf(status_line, "e=%f ", err_out(mx));
        write_text(status_line, WIDTH, 60);

        sprintf(status_line, "a0=%f b0=%f", a0, b0);
        write_text(status_line, WIDTH, 80);

        sprintf(status_line, "a1=%f b1=%f", a1, b1);
        write_text(status_line, WIDTH, 100);

        sprintf(status_line, "a2=%f b2=%f", a2, b2);
        write_text(status_line, WIDTH, 120);

        sprintf(status_line, "a3=%f a4=%f", a3, a4);
        write_text(status_line, WIDTH, 140);

        sprintf(status_line, "b3=%f", b3);
        write_text(status_line, WIDTH, 160);

        SDL_RenderPresent(main_window);
    }
    SDL_Quit();

    return 0;
}
