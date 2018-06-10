# FractalCL

Discover and explore fractals world with OpenCL acceleration.

# Features

* Animated fractals
* Mouse support to zoom in/out
* Keyboard support for changing fractals/kernel parameters
* OpenCL support to speed up fractals calculation
* 2 colors models: RGB and HSV
* OpenCL kernels can be executed on CPU

# Requirements 

* SDL library
* OpenCL GPU library

# Implemented fractals

* julia z^2+c
![julia](julia.png)

* julia z^3+c
![julia3](julia3.png)

* mandelbrot z^2+c
![mandelbrot](mandelbrot.png)

* dragon
![dragon](dragon.png)

# Tested platforms

* Intel OpenCL GPU on Linux: https://github.com/intel/compute-runtime

# Build instruction

* Run configure script to configure project
* Run make to build project

# Run instruction

* Execute FractalCL application in build directory. Use mouse and keyboard to change fractals parameters.
* Press ESC key to exit application.

