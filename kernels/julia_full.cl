#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void julia_full(__global uint *pixels, __global unsigned int *colors,
                         int mm, double ofs_lx, double step_x, double ofs_ty,
                         double step_y, double er, int max_iter, int pal,
                         int show_z, double c_x, double c_y) {
  int x, y;

  int i;
  uint color = 0;
  double j_x, j_y;
  double z_julia_x, z_julia_y, d;

  x = get_global_id(0);
  y = get_global_id(1);

  z_julia_x = ofs_lx + x * step_x;
  z_julia_y = ofs_ty + y * step_y;

  i = 0;
  while (i < max_iter) {
    j_x = z_julia_x * z_julia_x - z_julia_y * z_julia_y + c_x;
    j_y = 2 * z_julia_x * z_julia_y + c_y;
    d = (j_x * j_x + j_y * j_y);
    if (d > er)
      break;
    z_julia_x = j_x;
    z_julia_y = j_y;
    i++;
  }

  color = colors[i % 360 + 360 * (i < max_iter)];
  pixels[y * WIDTH + x] = color;
}
