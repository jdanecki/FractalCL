extern int test_function(void);

__kernel void test_kernel()
{
    printf("!!! test_kernel function=%d!!!\n", test_function());

#ifdef cl_khr_fp64
    double v1 = 1.0000000000000002;
    int i;
    double z_x = 0, z_y = 0;
    double j_x, j_y;
    double c_x = 0.3695692, c_y = 0.27758014;
    double d;
    double tmp1;

    for (i = 0; i < 2500; i++)
    {
        tmp1 = z_y * z_y;
        // j_x = z_x * z_x - tmp1;
        j_x = mad(z_x, z_x, -tmp1);
        j_x += c_x;

        j_y = 2 * z_x * z_y + c_y;
        d = (j_x * j_x + j_y * j_y);
        if (d > 2) break;
        z_x = j_x;
        z_y = j_y;
    }
    printf("cl_khr_fp64 defined: v1=%1.16f sum=%1.16f x=%1.16f y=%1.16f iter=%d\n", v1, v1 + v1, z_x, z_y, i);

#else
    float v1 = 1.0000001192;
    printf("cl_khr_fp64 not defined: v1=%1.10f sum=%1.10f\n", v1, v1 + v1);
#endif
}
