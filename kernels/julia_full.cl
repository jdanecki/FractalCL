#ifdef FP_64_SUPPORT
#define FP_TYPE double
#define KERNEL_ARGS kernel_args64
#else
#define FP_TYPE float
#define KERNEL_ARGS kernel_args32
#endif

#include "fractal_types.h"

#ifdef HOST_APP
void julia_full(int x, int y, uint* pixels, unsigned int* colors, struct KERNEL_ARGS args)
#else
__kernel void julia_full(__global uint* pixels, __global unsigned int* colors, struct KERNEL_ARGS args)
#endif
{
    unsigned int i;
    uint color = 0;
    FP_TYPE j_x, j_y;
    FP_TYPE z_julia_x, z_julia_y, d;

#ifndef HOST_APP
    int x = get_global_id(0);
    int y = get_global_id(1);
#endif
    z_julia_x = args.ofs_lx + x * args.step_x;
    z_julia_y = args.ofs_ty + y * args.step_y;

    i = 0;
    while (i < args.max_iter)
    {
        j_x = z_julia_x * z_julia_x - z_julia_y * z_julia_y + args.c_x;
        j_y = 2 * z_julia_x * z_julia_y + args.c_y;
        d = (j_x * j_x + j_y * j_y);
        if (d > args.er) break;
        z_julia_x = j_x;
        z_julia_y = j_y;
        i++;
    }

    color = colors[i % 360 + 360 * (i < args.max_iter)];
    color |= args.mm;
    pixels[y * WIDTH + x] = color;
}
