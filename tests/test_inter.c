/*
    Copyright (C) 2019  Jacek Danecki <jacek.m.danecki@gmail.com>

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

#include <stdio.h>

long double lx = -1.0f;
long double size = 1.0f;
int i;

#define WIDTH 1024

void inter()
{
    long double z_x0, z_x1;
    long double step = size / WIDTH;
    double z_x2;

    z_x0 = lx;
    z_x1 = lx + step;
    z_x2 = lx + step;

    //     printf("[%2d] zoom=%Le x0=%1.20Le x1=%1.20Le diff=%1.20Le step=%1.20Le err=%1.20Le\n", i, 1.0/size, z_x0, z_x1, z_x1-z_x0, step, z_x1-z_x0 - step);
    printf("[%2d] zoom=%Le %1.20Le %1.20e\n", i, 1.0 / size, z_x1, z_x2);
}

void test1()
{
    long double x1 = 2.71050543121376108502e-20;
    long double x2 = 5.00000000000000000000e-01;
    long double res1 = x1 + x1;
    long double res2 = x1 + x2;
    double x3 = x1;
    int i;
    printf("res1=%1.20Le res2=%1.20Le\n", res1, res2);

    for (i = 0; i < 100; i++)
    {
        printf("x1=%1.20Le x3=%1.20e\n", x1, x3);
        x1 /= 2.0;
        x3 /= 2.0;
    }
}

int main()
{
#if 1
    for (i = 0; i < 100; i++)
    {
        inter();
        lx += size / 4.0;
        size /= 2.0f;
    }
#else
    test1();
#endif
    return 0;
}
