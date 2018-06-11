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

#ifndef FRACTAL_COMPLEX_H_
#define FRACTAL_COMPLEX_H_

typedef struct
{
    FP_TYPE x;
    FP_TYPE y;
} complex_t;

complex_t complex_t_mul(complex_t a, complex_t b);

complex_t complex_t_mul(complex_t a, complex_t b)
{
    complex_t res;
    res.x = a.x * b.x - a.y * b.y;
    res.y = a.x * b.y + a.y * b.x;
    return res;
}

complex_t complex_t_add(complex_t a, complex_t b);
complex_t complex_t_add(complex_t a, complex_t b)
{
    complex_t res;

    res.x = a.x + b.x;
    res.y = a.y + b.y;
    return res;
}

FP_TYPE complex_t_modul(complex_t z);
FP_TYPE complex_t_modul(complex_t z)
{
    FP_TYPE res;

    res = sqrt(z.x * z.x + z.y * z.y);
    return res;
}

FP_TYPE equation(int x, FP_TYPE x1, FP_TYPE y1, FP_TYPE x2, FP_TYPE y2);
FP_TYPE equation(int x, FP_TYPE x1, FP_TYPE y1, FP_TYPE x2, FP_TYPE y2)
{
    FP_TYPE a = (y1 - y2) / (x1 - x2);
    FP_TYPE b = (x1 * y2 - y1 * x2) / (x1 - x2);
    FP_TYPE y = a * x + b;

    return y;
}

#endif /* FRACTAL_COMPLEX_H_ */
