#ifdef FP_64_SUPPORT
#define FP_TYPE double
#else
#define FP_TYPE float
#endif

#ifdef HOST_APP
void burning_ship(int x, int y, uint* pixels, unsigned int* colors, unsigned int mm, FP_TYPE ofs_lx, FP_TYPE step_x, FP_TYPE ofs_ty, FP_TYPE step_y, FP_TYPE er,
                  unsigned int max_iter, int pal, int show_z)
#else
__kernel void burning_ship(__global uint* pixels, __global unsigned int* colors, unsigned int mm, FP_TYPE ofs_lx, FP_TYPE step_x, FP_TYPE ofs_ty,
                           FP_TYPE step_y, FP_TYPE er, unsigned int max_iter, int pal, int show_z, int ofs_x, int ofs_y)
#endif
{
    unsigned int i;
    unsigned int color = 0;
    FP_TYPE j_x, j_y;
    FP_TYPE z_x = 0, z_y = 0;
    FP_TYPE d;
    FP_TYPE c_x, c_y;

#ifndef HOST_APP
    int x = ofs_x + 4 * get_global_id(0);
    int y = ofs_y + 4 * get_global_id(1);
#endif

    c_x = ofs_lx + x * step_x;
    c_y = ofs_ty + y * step_y;

    i = 0;
    while (i < max_iter)
    {
        j_x = z_x * z_x - z_y * z_y + c_x;
        j_y = 2 * fabs(z_x * z_y) + c_y;

        d = (j_x * j_x + j_y * j_y);
        if (d > er) break;

        z_x = j_x;
        z_y = j_y;
        i++;
    }
    if (pal)
        color = 0xff000000 | i | mm;
    else
    {
        color = colors[i % 360 + 360 * (i < max_iter)];
        color |= mm;
    }
    if (show_z)
    {
        int x1, y1;
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
