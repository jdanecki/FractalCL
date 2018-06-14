#include "fractal_complex.h"
#include <stdio.h>

int main()
{
    unsigned int res = 0;

    complex_t z1 = {1.0f, 2.0f};
    complex_t z2 = {3.0f, 4.0f};
    complex_t z3, z4;

    z3 = complex_t_add(z1, z2);
    printf("(%2.0f + i%-2.0f) + (%2.0f + i%-2.0f) = %2.0f + i%-2.0f\n", z1.x, z1.y, z2.x, z2.y, z3.x, z3.y);
    if (z3.x != 4) res |= 1;
    if (z3.y != 6) res |= 2;

    z3 = complex_t_mul(z1, z2);
    printf("(%2.0f + i%-2.0f) * (%2.0f + i%-2.0f) = %2.0f + i%-2.0f\n", z1.x, z1.y, z2.x, z2.y, z3.x, z3.y);
    if (z3.x != -5) res |= 3;
    if (z3.y != 10) res |= 4;

    z4 = complex_t_div(z3, z2);
    printf("(%2.0f + i%-2.0f) / (%2.0f + i%-2.0f) = %2.0f + i%-2.0f\n", z3.x, z3.y, z2.x, z2.y, z4.x, z4.y);
    if (z4.x != z1.x) res |= 16;
    if (z4.y != z1.y) res |= 32;

    if (!res)
        printf("test passed\n");
    else
        printf("test failed res=%x\n", res);
    return res;
}
