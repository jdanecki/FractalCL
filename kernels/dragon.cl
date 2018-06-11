#ifdef HOST_APP
void dragon(int px, int py, uint* pixels, unsigned int* colors, int mm,
            double ofs_lx, double step_x, double ofs_ty, double step_y,
            double er, int max_iter, int pal, int show_z, double c_x,
            double c_y)
#else
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void dragon(__global uint* pixels, __global unsigned int* colors,
                     int mm, double ofs_lx, double step_x, double ofs_ty,
                     double step_y, double er, int max_iter, int pal,
                     int show_z, double c_x, double c_y)
#endif
{
#ifndef HOST_APP
    int px = get_global_id(0);
    int py = get_global_id(1);
#endif
    int x, y;
    int r;
    float x1 = 0, y1 = 0;
    float xc = 0, yc = 0;

    if (px == 0 && py == 0)
    {
        for (r = 0; r < max_iter; r++)
        {
            int select_move = 0;
#ifdef HOST_APP
            if (rand() % 20 <= 10) select_move = 1;
#else
            if (cos(3.14 * sin(1.0 * r * r)) > er) select_move = 1;
#endif
            if (select_move)
            {
                x1 = -0.3 * xc - 1.0;
                y1 = -0.3 * yc + 0.1;
            }
            else
            {
                x1 = 0.76 * xc - 0.4 * yc;
                y1 = 0.4 * xc + 0.76 * yc;
            }
            xc = x1;
            yc = y1;
            x = (ofs_lx + x1) * mm;
            y = (ofs_ty + y1) * mm;
            if (x < WIDTH / 2 && y < HEIGHT / 2 && x > -WIDTH / 2 &&
                y > -HEIGHT / 2)
            {
                pixels[(HEIGHT / 2 - y) * WIDTH + WIDTH / 2 + x] = 0xff0000 | r;
            }
        }
    }
}
