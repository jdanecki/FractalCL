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

void show_double(double val)
{
    unsigned long* h = (unsigned long*)&val;
    printf("val=%1.20f %1.20e %lx %lx\n", val, val, *h, (*h) >> 52);
}

double make_double(int e, unsigned long f)
{
    unsigned long exp = e & 0xfff;
    unsigned long frac = f & 0xfffffffffffff;
    unsigned long d = (exp << 52) | frac;
    double* pd = (double*)&d;
    return *pd;
}

int main()
{
#if 0
   __float128 x0=-5.00000000000000006939e-01;
   __float128 step = 1.35525271560688054251e-20;
   __float128 x1 = x0 + step;

   printf("diff =  %.36Qg\n", x1 - x0 - step);
#endif

    /*   show_double(1.0000000000000002);
       double d = make_double(0x3ff, 0);
       show_double(d);

       show_double(-1.0000000000000002);
    */
    for (int i = 1; i < 10; i++) show_double(1.0 * i);
    return 0;
}
