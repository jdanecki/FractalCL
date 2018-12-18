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

#include <math.h>
#include <stdio.h>

int max_iter = 1360;

unsigned int get_iter(double z_x, double z_y)
{
    unsigned int iter = 0;
    double j_x, j_y;
    double er = 4;
    double d;

    while (iter < max_iter)
    {
        //        printf("z_x=%0.15f, z_y=%0.15f\n", z_x, z_y);
        j_x = z_x * z_x - z_y * z_y + 0.15f;
        j_y = 2.0f * z_x * z_y - 0.6f;

        d = (j_x * j_x + j_y * j_y);
        //        printf("j_x=%0.15f j_y=%0.15f d=%0.15f iter=%u\n",j_x, j_y, d, iter);
        if (d > er) break;

        z_x = j_x;
        z_y = j_y;
        iter++;
    }
    printf("iter=%u\n", iter);

    return iter;
}

int main()
{
    get_iter(-0.09509506513131782413, -0.50412115957199910010);
    get_iter(0.72379302649810739467, 0.76042429385487242399);
    return 0;
}
