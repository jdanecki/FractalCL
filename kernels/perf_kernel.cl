#include "fractal_types.h"

__kernel void perf_kernel(__global unsigned int* output, int k)
{
    int x = get_global_id(0);
    int y;
    unsigned int sum = 0;
    for (y = 0; y < 0x4fff; y++) sum++;

    output[y * WIDTH + x] = 0x8000 | sum + k * 0x80; //(y * WIDTH + x) << 8;

    //	if (x==0) printf("k=%d\n", sum);
    //	printf("[%d, %d] = %d\n", x, y, x*y);
}
