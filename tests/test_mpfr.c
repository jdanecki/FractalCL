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

#include <gmp.h>
#include <mpfr.h>
#include <stdio.h>

int res = 200;

void foo(mpfr_t zx)
{
    printf("foo zx=");
    mpfr_out_str(stdout, 10, 0, zx, MPFR_RNDD);
    puts("");
}

int main()
{
    int x;
    mpfr_t zx, size, lx;
    mpfr_init2(zx, res);
    mpfr_init_set_d(lx, -0.123661097601143424, MPFR_RNDD);
    mpfr_init_set_d(size, 0.00000000000022737, MPFR_RNDD);

    mpfr_t temp;
    mpfr_init2(temp, res);

    printf("default prec=%ld\n", mpfr_get_default_prec());

    for (x = 0; x < 10; x++)
    {
        double stepx = 1.0f * x / 1000;

        mpfr_mul_d(temp, size, stepx, MPFR_RNDD);
        mpfr_add(zx, lx, temp, MPFR_RNDD);
        printf("zx=");
        mpfr_out_str(stdout, 10, 0, zx, MPFR_RNDD);
        puts("");

        //        foo(zx);
    }
    mpfr_clear(zx);
    mpfr_clear(size);
    mpfr_clear(lx);
    mpfr_clear(temp);
    return 0;
}
