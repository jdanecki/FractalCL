#include "fractal_types.h"

int test_function() { return 123; }

unsigned int set_color(struct KERNEL_ARGS args, unsigned int i, __global unsigned int* colors)
{
    unsigned int color, r, g, b, c;

    switch (args.pal)
    {
    case 1:
        color = 0xff000000 | (i * args.mm) | args.rgb;
        break;
    case 2:
        c = 1.0 * i / args.max_iter;
        r = 255 * (args.c1[0] + args.c2[0] * cos(6.2830 * (args.c3[0] * c + args.c4[0])));
        g = 255 * (args.c1[1] + args.c2[1] * cos(6.2830 * (args.c3[1] * c + args.c4[1])));
        b = 255 * (args.c1[2] + args.c2[2] * cos(6.2830 * (args.c3[2] * c + args.c4[2])));
        color = r << 16 | g << 8 | b;
        break;
    case 0:
        c = i * args.mm;
        color = colors[c % 360 + 360 * (c < args.max_iter)];
        color |= args.rgb;
        break;
    }
    return color;
}
