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

//#define USE_SIGMOID

#include "gui.h"
#include <time.h>

unsigned int* pixels;
float a0, b0;                         // expected
float a1, b1, a2, b2, a3, b3, a4, b4; // input layer - 4 neurouns
float out1, out2, out3, out4, out_b;  // output layer - 1 neuron
float learning = 0.01;
float zoom = 1.0;

void draw(float fx, float fy, unsigned int color)
{
    int x, y;

    x = (fx + (1.0 * zoom)) * WIDTH / (2.0 * zoom);
    y = (fy + (1.0 * zoom)) * HEIGHT / (2.0 * zoom);

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

float perpendicular(float a, float b, float x) { return x * (a + 1.0 / a) + b; }

float expected1(float i)
{
    float b;
    if (!a0) return 0;
    float y = a0 * i + b0;
    if (i < -0.5) return y;
    b = perpendicular(a0, b0, -0.5);
    if (i < 0)
    {
        return -1.0 / a0 * i + b;
    }
    b = perpendicular(-1.0 / a0, b, 0);
    if (i < 0.5)
    {
        return a0 * i + b;
    }
    b = perpendicular(a0, b, 0.5);
    return -1.0 / a0 * i + b;
}

float expected2(float i)
{
    float b;
    if (!a0) return 0;
    float y = a0 * i + b0;
    if (i < 0) return y;
    b = perpendicular(a0, b0, 0);
    return -1.0 / a0 * i + b;
}

float (*expected)(float) = expected1;

void get_values(float i, float* v1, float* v2, float* v3, float* v4)
{
#ifdef USE_SIGMOID
    *v1 = sigmoid(a1 * i + b1);
    *v2 = sigmoid(a2 * i + b2);
    *v3 = sigmoid(a3 * i + b3);
    *v4 = sigmoid(a4 * i + b4);
#else
    *v1 = a1 * i + b1;
    *v2 = a2 * i + b2;
    *v3 = a3 * i + b3;
    *v4 = a4 * i + b4;
    if (*v1 < 0) *v1 = 0.01; // lrelu
    if (*v2 < 0) *v2 = 0.01;
    if (*v3 < 0) *v3 = 0.01;
    if (*v4 < 0) *v4 = 0.01;
#endif
}

float f2(float i)
{
    float v1, v2, v3, v4;
    get_values(i, &v1, &v2, &v3, &v4);

    return out1 * v1 + out2 * v2 + out3 * v3 + out4 * v4 + out_b;
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
        float v1, v2, v3, v4;
        get_values(p, &v1, &v2, &v3, &v4);

        out1 += learning * e * v1;
        out2 += learning * e * v2;
        out3 += learning * e * v3;
        out4 += learning * e * v4;
        out_b += learning * e;

        float e1 = out1 * e;
        float e2 = out2 * e;
        float e3 = out3 * e;
        float e4 = out4 * e;
#ifdef USE_SIGMOID
        a1 += learning * e1 * p * v1 * (1.0 - v1);
        a2 += learning * e2 * p * v2 * (1.0 - v2);
        a3 += learning * e3 * p * v3 * (1.0 - v3);
        a4 += learning * e4 * p * v4 * (1.0 - v4);
#else
        a1 += learning * e1 * p;
        a2 += learning * e2 * p;
        a3 += learning * e3 * p;
        a4 += learning * e4 * p;
#endif
        b1 += learning * e1;
        b2 += learning * e2;
        b3 += learning * e3;
        b4 += learning * e4;
        p += 0.1;
    }
}

#define CALIBRATION

int main()
{
    char status_line[200];
    int fin = 0;
    float mx = 0, my = 0;
    int learn_loop = 0;
    int iter = 0;

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
    new_values(&a4, &b4);
    new_values(&out1, &out_b);
    out2 = -out1;
    out3 = out1 * out2;
    out4 = -out3;

    while (!fin)
    {
        SDL_Event event;
        memset(pixels, 0, IMAGE_SIZE);

        for (float p = -1.0 * zoom; p < 1.0 * zoom;)
        {
            draw(p, expected(p), 0x00ff00);
            draw(p, f2(p), 0xffffff);
            draw(p, err_out(p), 0xff0000);

#ifdef CALIBRATION
            draw(p, 0.25, 0x800000);
            draw(p, -0.25, 0x800000);
            draw(p, 0.5, 0x008000);
            draw(p, -0.5, 0x008000);
            draw(p, 0.75, 0x8080);
            draw(p, -0.75, 0x8080);

            draw(0.25, p, 0x800000);
            draw(-0.25, p, 0x800000);
            draw(0.5, p, 0x008000);
            draw(-0.5, p, 0x008000);
            draw(0.75, p, 0x8080);
            draw(-0.75, p, 0x8080);
#endif
            draw(p, 0, 0x80);
            draw(0, p, 0x80);

            p += 0.001;
        }
        if (learn_loop)
        {
            learn();
            iter++;
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
                case SDLK_F5:
                    new_values(&a0, &b0);
                    break;
                case SDLK_F6:
                    new_values(&a1, &b1);
                    break;
                case SDLK_F7:
                    new_values(&a2, &b2);
                    break;
                case SDLK_F8:
                    new_values(&a3, &b3);
                    break;
                case SDLK_F9:
                    new_values(&a4, &b4);
                    break;

                case SDLK_F10:
                    new_values(&out1, &out_b);
                    out2 = -out1;
                    out3 = out1 * out2;
                    out4 = -out3;
                    break;
                case SDLK_F1:
                    learn();
                    iter++;
                    break;
                case SDLK_SPACE:
                    learn_loop ^= 1;
                    iter = 0;
                    break;
                case SDLK_F2:
                    out1 = out2 = out3 = out4 = 1.0;
                    out_b = 0;
                    break;

                case SDLK_F3:
                    zoom += 1.0;
                    break;
                case SDLK_F4:
                    zoom -= 1.0;
                    if (zoom < 1.0) zoom = 1.0;
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

                case '9':
                    a3 += 0.01;
                    break;
                case '0':
                    a3 -= 0.01;
                    break;
                case '-':
                    b3 += 0.01;
                    break;
                case '=':
                    b3 -= 0.01;
                    break;

                case '[':
                    a4 += 0.01;
                    break;
                case ']':
                    a4 -= 0.01;
                    break;
                case ',':
                    b4 += 0.01;
                    break;
                case '.':
                    b4 -= 0.01;
                    break;

                case 'q':
                    out1 += 0.01;
                    break;
                case 'w':
                    out1 -= 0.01;
                    break;
                case 'e':
                    out2 += 0.01;
                    break;
                case 'r':
                    out2 -= 0.01;
                    break;
                case 't':
                    out3 += 0.01;
                    break;
                case 'y':
                    out3 -= 0.01;
                    break;
                case 'u':
                    out4 += 0.01;
                    break;
                case 'i':
                    out4 -= 0.01;
                    break;

                case 'a':
                    out_b += 0.01;
                    break;
                case 's':
                    out_b -= 0.01;
                    break;
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                if (event.button.x > WIDTH) continue;
                mx = zoom * ((2.0f * event.button.x) / WIDTH - 1.0);
                my = zoom * ((2.0f * (HEIGHT - event.button.y)) / HEIGHT - 1.0);
            }
        }
        SDL_UpdateTexture(texture, NULL, pixels, WIDTH * 4);
        SDL_RenderCopy(main_window, texture, NULL, &window_rec);

        int ty = 0;
        sprintf(status_line, "mx=%f my=%f ", mx, my);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "exp=%f ", expected(mx));
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "f2=%f ", f2(mx));
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "e=%f ", err_out(mx));
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "a0=%f b0=%f ", a0, b0);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "a1=%f b1=%f ", a1, b1);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "a2=%f b2=%f ", a2, b2);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "a3=%f b3=%f ", a3, b3);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "a4=%f b4=%f ", a4, b4);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "out1=%f ", out1);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "out2=%f ", out2);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "out3=%f ", out3);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "out4=%f ", out4);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "out_b=%f ", out_b);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "zoom=%f ", zoom);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        sprintf(status_line, "iter=%5d ", iter);
        write_text(status_line, WIDTH, ty);
        ty += 20;

        SDL_RenderPresent(main_window);
    }
    SDL_Quit();

    return 0;
}
