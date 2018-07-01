#ifdef FP_64_SUPPORT
#define FP_TYPE double
#else
#define FP_TYPE float
#endif

#include "fractal_types.h"

#ifdef HOST_APP
void julia(int x, int y, uint* pixels, unsigned int* colors, struct kernel_args args)
#else

__kernel void julia(__global uint* pixels, __global unsigned int* colors, struct kernel_args args)
#endif
{
    unsigned int i;
    unsigned int color = 0;
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
    if (args.pal)
        color = 0xff000000 | i | args.mm;
    else
    {
        color = colors[i % 360 + 360 * (i < args.max_iter)];
        color |= args.mm;
    }

    if (args.show_z)
    {
        unsigned int x1, y1;
        x1 = WIDTH / 2 + z_x * WIDTH / 4;
        y1 = HEIGHT / 2 + z_y * HEIGHT / 4;
        if (y1 < HEIGHT && x1 < WIDTH && x1 > 0 && y1 > 0)
        {
            pixels[y1 * WIDTH + x1] = color;
        }
        else
            return;
    }
    else
    {
        pixels[y * WIDTH + x] = color;
    }
}
