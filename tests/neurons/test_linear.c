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

// learning tests with linear function

#include "gui.h"
#include <time.h>

unsigned int* pixels;
float a1, b1, a2, b2;
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

float expected(float i) { return a1 * i + b1; }

float learnt(float i) { return a2 * i + b2; }

float err(float i) { return (expected(i) - learnt(i)); }

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
        float e = err(p);
        a2 += learning * e * p;
        b2 += learning * e;
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

    new_values(&a1, &b1);
    new_values(&a2, &b2);

    while (!fin)
    {
        SDL_Event event;
        memset(pixels, 0, IMAGE_SIZE);

        for (float p = -1.0; p < 1.0;)
        {
            draw(p, expected(p), 0x00ff00); // green
            draw(p, learnt(p), 0xffffff);   // white
            draw(p, err(p), 0xff0000);      // red

            draw(p, 0, 0xff); // blue
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
                    new_values(&a1, &b1);
                    break;
                case SDLK_F10:
                    new_values(&a2, &b2);
                    break;
                case SDLK_F1:
                    learn();
                    break;
                case 'a':
                    a2 += 0.01;
                    break;
                case 's':
                    a2 -= 0.01;
                    break;
                case 'b':
                    b2 += 0.01;
                    break;
                case 'n':
                    b2 -= 0.01;
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

        sprintf(status_line, "expected=%f ", expected(mx));
        write_text(status_line, WIDTH, 20);

        sprintf(status_line, "learnt=%f ", learnt(mx));
        write_text(status_line, WIDTH, 40);

        sprintf(status_line, "e=%f ", err(mx));
        write_text(status_line, WIDTH, 60);

        sprintf(status_line, "a1=%f b1=%f", a1, b1);
        write_text(status_line, WIDTH, 80);

        sprintf(status_line, "a2=%f b2=%f", a2, b2);
        write_text(status_line, WIDTH, 100);

        SDL_RenderPresent(main_window);
    }
    SDL_Quit();

    return 0;
}
