#include <time.h>
#include <unistd.h>
#include <string.h>
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <CL/cl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>

#define WIDTH 1920
#define HEIGHT 1080
#define WIDTH_FL (WIDTH * 1.0f)
#define HEIGHT_FL (HEIGHT * 1.0f)

#define BPP     4
#define PITCH (WIDTH * BPP)
#define IMAGE_SIZE (WIDTH * HEIGHT * BPP)

#define STRING(VALUE) #VALUE
#define STRING_MACRO(MACRO) STRING(MACRO)

int init_ocl();
int close_ocl();

enum fractals {
    JULIA,
    MANDELBROT,
	JULIA4,
	DRAGON,
	NR_FRACTALS
};

struct kernel_args {
		void * pixels;
		unsigned int * colors;
		int mm;
		double ofs_lx;
		double ofs_rx;
		double ofs_ty;
		double ofs_by;
		double er;
		int max_iter;
		int pal;
		int show_z;
		double c_x, c_y;
		int ofs_x, ofs_y;
};

struct ocl_fractal {
    char * name;
    enum fractals id;
    char * source;
    int fd;
    size_t filesize;
};

extern struct ocl_fractal fractals[NR_FRACTALS];

struct ocl_thread {
    pthread_t tid;
    int work;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int finished;
};

struct ocl_device {
    cl_device_id gpu;
    cl_command_queue queue;
    cl_platform_id id;
    cl_context ctx;
    int found;
    cl_program program;
    cl_kernel kernels[NR_FRACTALS];
    struct kernel_args args[NR_FRACTALS];
    cl_event event;
    char name[256];
    unsigned int eu;
    size_t wgs;
    struct ocl_thread thread;
    cl_mem cl_colors;
    cl_mem cl_pixels;
    unsigned int execution;
};

extern struct ocl_device intel;
extern volatile int nr_devices;

