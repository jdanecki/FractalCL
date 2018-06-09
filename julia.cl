#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void julia(__global uint *pixels, __global unsigned int *colors,
                    int mm, double ofs_lx, double step_x, double ofs_ty,
                    double step_y, double er, int max_iter, int pal, int show_z,
                    double c_x, double c_y, int ofs_x, int ofs_y) {
  int x, y;

  int i;
  unsigned int color = 0;
  double j_x, j_y;
  double z_x, z_y, d;

  x = ofs_x + 4 * get_global_id(0);
  y = ofs_y + 4 * get_global_id(1);

  z_x = ofs_lx + x * step_x;
  z_y = ofs_ty + y * step_y;

  i = 0;
  while (i < max_iter) {
    j_x = z_x * z_x - z_y * z_y + c_x;
    j_y = 2 * z_x * z_y + c_y;

    d = (j_x * j_x + j_y * j_y);
    if (d > er)
      break;

    z_x = j_x;
    z_y = j_y;
    i++;
  }
  if (pal)
    color = 0xff000000 + i * mm;
  else
    color = colors[i % 360 + 360 * (i < max_iter)];

  if (show_z) {
    unsigned int x1, y1;
    x1 = WIDTH / 2 + z_x * WIDTH / 4;
    y1 = HEIGHT / 2 + z_y * HEIGHT / 4;
    if (y1 < HEIGHT && x1 < WIDTH && x1 > 0 && y1 > 0) {
      pixels[y1 * WIDTH + x1] = color;
    } else
      return;
  } else {
    pixels[y * WIDTH + x] = color;
  }
}
