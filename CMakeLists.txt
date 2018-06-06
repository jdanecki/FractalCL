project(FractalCL)
cmake_minimum_required(VERSION 3.2)

find_package(PkgConfig)
pkg_check_modules(SDL sdl)
find_package(OpenCL REQUIRED)

include_directories(include)

add_definitions("-march=native -mtune=native -g -Wall -Wno-deprecated-declarations " ${SDL_CFLAGS})

add_executable(FractalCL
    fractal_ocl.c
    ocl.c
    include/fractal_complex.h
    include/fractal_ocl.h
    julia.cl
    mandelbrot.cl
)

target_link_libraries(FractalCL
    ${SDL_LDFLAGS}
    ${OpenCL_LIBRARY}
    -lm -lrt -lpthread -ldl
)

