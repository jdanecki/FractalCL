#ifdef FP_64_SUPPORT
#define FP_TYPE double
#define KERNEL_ARGS kernel_args64
#else
#define FP_TYPE float
#define KERNEL_ARGS kernel_args32
#endif

#include "fractal_types.h"

#ifdef HOST_APP
void julia3(int x, int y, uint* pixels, unsigned int* colors, struct KERNEL_ARGS args)
#else
__kernel void julia3(__global uint* pixels, __global unsigned int* colors, struct KERNEL_ARGS args)
#endif
{
    unsigned int i;
    unsigned int color = 0, r, g, b;
    float c;
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
        j_x = z_x * z_x * z_x - 3 * z_x * z_y * z_y + args.c_x;
        j_y = 3 * z_x * z_x * z_y - z_y * z_y * z_y + args.c_y;

        d = (j_x * j_x + j_y * j_y);
        if (d > args.er) break;

        z_x = j_x;
        z_y = j_y;
        i++;
    }

    switch (args.pal)
    {
    case 1:
        i *= args.mm;
        color = 0xff000000 | i | args.rgb;
        break;
    case 2:
        c = 1.0 * i / args.max_iter;
        r = 255 * (args.c1[0] + args.c2[0] * cos(6.2830 * (args.c3[0] * c + args.c4[0])));
        g = 255 * (args.c1[1] + args.c2[1] * cos(6.2830 * (args.c3[1] * c + args.c4[1])));
        b = 255 * (args.c1[2] + args.c2[2] * cos(6.2830 * (args.c3[2] * c + args.c4[2])));
        color = r << 16 | g << 8 | b;
        break;
    case 0:
        i *= args.mm;
        color = colors[i % 360 + 360 * (i < args.max_iter)];
        color |= args.rgb;
        break;
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
