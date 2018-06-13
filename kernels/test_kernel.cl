__kernel void test_kernel()
{
    printf("!!! test_kernel !!!\n");

#ifdef cl_khr_fp64
    printf("cl_khr_fp64 defined\n");
#else
    printf("cl_khr_fp64 not defined\n");
#endif
}
