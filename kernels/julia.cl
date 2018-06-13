#ifdef FP_64_SUPPORT
#define FP_TYPE double
#else
#define FP_TYPE float
#endif

#ifdef HOST_APP
void julia(int x, int y, uint* pixels, unsigned int* colors, int mm, FP_TYPE ofs_lx, FP_TYPE step_x, FP_TYPE ofs_ty, FP_TYPE step_y, FP_TYPE er,
           int max_iter, int pal, int show_z, FP_TYPE c_x, FP_TYPE c_y)
#else

__kernel void julia(__global uint* pixels, __global unsigned int* colors, int mm, FP_TYPE ofs_lx, FP_TYPE step_x, FP_TYPE ofs_ty, FP_TYPE step_y,
                    FP_TYPE er, int max_iter, int pal, int show_z, FP_TYPE c_x, FP_TYPE c_y, int ofs_x, int ofs_y)
#endif
{
    int i;
    unsigned int color = 0;
    FP_TYPE j_x, j_y;
    FP_TYPE z_x, z_y, d;

#ifndef HOST_APP
    int x = ofs_x + 4 * get_global_id(0);
    int y = ofs_y + 4 * get_global_id(1);
#endif
    z_x = ofs_lx + x * step_x;
    z_y = ofs_ty + y * step_y;

    i = 0;
    while (i < max_iter)
    {
        j_x = z_x * z_x - z_y * z_y + c_x;
        j_y = 2 * z_x * z_y + c_y;

        d = (j_x * j_x + j_y * j_y);
        if (d > er) break;

        z_x = j_x;
        z_y = j_y;
        i++;
    }
    if (pal)
        color = 0xff000000 + i * mm;
    else
        color = colors[i % 360 + 360 * (i < max_iter)];

    if (show_z)
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
