#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void dragon(__global uint *pixels, __global unsigned int *colors,
                     int mm, double ofs_lx, double step_x, double ofs_ty,
                     double step_y, double er, int max_iter, int pal,
                     int show_z, double c_x, double c_y, int ofs_x, int ofs_y) {
  int x, y;
  int r;
  float x1 = 0, y1 = 0;
  float xc = 0, yc = 0;

  if (get_global_id(0) == 0 && get_global_id(1) == 0) {
    for (r = 0; r < max_iter; r++) {
      if (cos(3.14 * sin(1.0 * r * r)) > er) {
        x1 = -0.3 * xc - 1.0;
        y1 = -0.3 * yc + 0.1;
      } else {
        x1 = 0.76 * xc - 0.4 * yc;
        y1 = 0.4 * xc + 0.76 * yc;
      }
      xc = x1;
      yc = y1;
      x = (ofs_lx + x1) * mm;
      y = (ofs_ty + y1) * mm;
      if (x < WIDTH / 2 && y < HEIGHT < 2 && x > -WIDTH / 2 &&
          y > -HEIGHT / 2) {
        pixels[(HEIGHT / 2 - y) * WIDTH + WIDTH / 2 + x] = 0xff0000 | r;
      }
    }
  } else {
    pixels[get_global_id(1) * WIDTH + get_global_id(0)] = 0;
  }
}
