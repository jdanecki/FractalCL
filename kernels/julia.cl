#include "fractal_types.h"

#ifdef HOST_APP
unsigned int julia(int x, int y, uint* pixels, unsigned int* colors, struct KERNEL_ARGS args)
#else

__kernel void julia(__global uint* pixels, __global unsigned int* colors, struct KERNEL_ARGS args)
#endif
{
    unsigned int i;

    FP_TYPE j_x, j_y;
    FP_TYPE z_x, z_y, d;
#ifndef HOST_APP
    int x = args.ofs_x + 4 * get_global_id(0);
    int y = args.ofs_y + 4 * get_global_id(1);
#endif
    z_x = args.ofs_lx + x * args.step_x;
    z_y = args.ofs_ty + y * args.step_y;

    i = 0;
    while (i < args.max_iter)
    {
        j_x = z_x * z_x - z_y * z_y + args.c_x;
        j_y = 2 * z_x * z_y + args.c_y;

        d = (j_x * j_x + j_y * j_y);
        if (d > args.er) break;

        z_x = j_x;
        z_y = j_y;
        i++;
    }
    //    pixels[y * WIDTH + x] = i;
    pixels[y * WIDTH + x] = set_color(args, i, colors);
#ifdef HOST_APP
    return i;
#endif
}
