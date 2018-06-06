#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void dragon(__global uint *pixels, __global unsigned int *colors,
                     int mm, double ofs_lx, double step_x, double ofs_ty,
                     double step_y, double er, int max_iter, int pal,
                     int show_z, double c_x, double c_y, int ofs_x, int ofs_y) {
  int x, y;
  int r;
  float x1 = 0, y1 = 0;
  float xc = 0, yc = 0;

  if (get_global_id(0) == 0) {
    for (r = 0; r < 100000; r++) {
      if (cos(3.14 * sin(1.0 * r)) > 0) {
        x1 = -0.4 * xc - 1.0;
        y1 = -0.4 * yc + 0.1;
      } else {
        x1 = 0.76 * xc - 0.4 * yc;
        y1 = 0.4 * xc + 0.76 * yc;
      }
      xc = x1;
      yc = y1;
      x = x1 * mm * 10;
      y = y1 * mm * 10;
      if (x > WIDTH)
        x = WIDTH - 1;
      if (y > HEIGHT)
        y = HEIGHT - 1;
      pixels[(HEIGHT / 2 + y) * WIDTH + WIDTH / 2 + x] = 0xffffff;
    }
  }
}
