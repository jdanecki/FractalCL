#ifdef FP_64_SUPPORT
#define FP_TYPE double
#else
#define FP_TYPE float
#endif

#ifdef HOST_APP
void julia_full(int x, int y, uint* pixels, unsigned int* colors, unsigned int mm, FP_TYPE ofs_lx, FP_TYPE step_x, FP_TYPE ofs_ty, FP_TYPE step_y, FP_TYPE er,
                unsigned int max_iter, int pal, int show_z, FP_TYPE c_x, FP_TYPE c_y)
#else
__kernel void julia_full(__global uint* pixels, __global unsigned int* colors, unsigned int mm, FP_TYPE ofs_lx, FP_TYPE step_x, FP_TYPE ofs_ty, FP_TYPE step_y,
                         FP_TYPE er, unsigned int max_iter, int pal, int show_z, FP_TYPE c_x, FP_TYPE c_y)
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
    z_julia_x = ofs_lx + x * step_x;
    z_julia_y = ofs_ty + y * step_y;

    i = 0;
    while (i < max_iter)
    {
        j_x = z_julia_x * z_julia_x - z_julia_y * z_julia_y + c_x;
        j_y = 2 * z_julia_x * z_julia_y + c_y;
        d = (j_x * j_x + j_y * j_y);
        if (d > er) break;
        z_julia_x = j_x;
        z_julia_y = j_y;
        i++;
    }

    color = colors[i % 360 + 360 * (i < max_iter)];
    color |= mm;
    pixels[y * WIDTH + x] = color;
}
