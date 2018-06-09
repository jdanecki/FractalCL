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

typedef struct {
  double x;
  double y;
} complex_t;

complex_t complex_t_mul(complex_t a, complex_t b);

complex_t complex_t_mul(complex_t a, complex_t b) {
  complex_t res;
  res.x = a.x * b.x - a.y * b.y;
  res.y = a.x * b.y + a.y * b.x;
  return res;
}

complex_t complex_t_add(complex_t a, complex_t b);
complex_t complex_t_add(complex_t a, complex_t b) {
  complex_t res;

  res.x = a.x + b.x;
  res.y = a.y + b.y;
  return res;
}

double complex_t_modul(complex_t z);
double complex_t_modul(complex_t z) {
  double res;

  res = sqrt(z.x * z.x + z.y * z.y);
  return res;
}

double equation(int x, double x1, double y1, double x2, double y2);
double equation(int x, double x1, double y1, double x2, double y2) {
  double a = (y1 - y2) / (x1 - x2);
  double b = (x1 * y2 - y1 * x2) / (x1 - x2);
  double y = a * x + b;

  return y;
}

#endif /* FRACTAL_COMPLEX_H_ */
