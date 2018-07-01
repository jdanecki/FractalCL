#ifdef FP_64_SUPPORT
#define FP_TYPE double
#define KERNEL_ARGS kernel_args64
#else
#define FP_TYPE float
#define KERNEL_ARGS kernel_args32
#endif

#include "fractal_types.h"

#ifdef HOST_APP
void dragon(int px, int py, uint* pixels, unsigned int* colors, struct KERNEL_ARGS args)

#else

__kernel void dragon(__global uint* pixels, __global unsigned int* colors, struct KERNEL_ARGS args)
#endif
{
#ifndef HOST_APP
    int px = get_global_id(0);
    int py = get_global_id(1);
#endif
    int x, y;
    unsigned int r;
    float x1 = 0, y1 = 0;
    float xc = 0, yc = 0;

    if (px == 0 && py == 0)
    {
        for (r = 0; r < args.max_iter; r++)
        {
            int select_move = 0;
#ifdef HOST_APP
            if (rand() % 20 <= 10) select_move = 1;
#else
            if (cos(3.14f * sin(1.0f * r * r)) > args.er) select_move = 1;
#endif
            if (select_move)
            {
                x1 = -0.3 * xc - 1.0 + (args.c_x - 0.15f);
                y1 = -0.3 * yc + 0.1 + (args.c_y + 0.60f);
            }
            else
            {
                x1 = 0.76 * xc - 0.4 * yc + (args.c_x - 0.15f);
                y1 = 0.4 * xc + 0.76 * yc + (args.c_y + 0.60f);
            }
            xc = x1;
            yc = y1;
            x = (args.ofs_lx + x1) / args.step_x;
            y = (args.ofs_ty + y1) / args.step_y;
            if (x < WIDTH / 2 && y < HEIGHT / 2 && x > -WIDTH / 2 && y > -HEIGHT / 2)
            {
                pixels[(HEIGHT / 2 - y) * WIDTH + WIDTH / 2 + x] = 0xff0000 | r | args.mm;
            }
        }
    }
}
