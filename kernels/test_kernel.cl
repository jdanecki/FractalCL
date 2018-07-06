extern int test_function();

__kernel void test_kernel()
{
    printf("!!! test_kernel function=%d!!!\n", test_function());

#ifdef cl_khr_fp64
    printf("cl_khr_fp64 defined\n");
#else
    printf("cl_khr_fp64 not defined\n");
#endif
}
