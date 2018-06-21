# Copyright (C) 2018  Jacek Danecki <jacek.m.danecki@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

project(FractalCL)
cmake_minimum_required(VERSION 3.2)

option(FP_64_SUPPORT "Use fp64 extension" ON)
option(SDL_ACCELERATED "Use SDL with GPU acceleration" OFF)
option(OPENCL_SUPPORT "Use OpenCL acceleration" ON)

find_package(PkgConfig)
pkg_check_modules(SDL sdl2)
pkg_check_modules(SDL_TTF SDL2_ttf)

include_directories(include)

add_definitions("-march=native -mtune=native -g -Wall -Wno-deprecated-declarations " ${SDL_CFLAGS} ${SDL_TTF_CFLAGS} -DHOST_APP)

if(FP_64_SUPPORT)
    add_definitions("-DFP_64_SUPPORT=1")
endif()

if(SDL_ACCELERATED)
	add_definitions("-DSDL_ACCELERATED=1")
endif()

if(OPENCL_SUPPORT)
    add_definitions("-DOPENCL_SUPPORT=1")
    find_package(OpenCL REQUIRED)
    set(OPTIONAL_LIBRARIES ${OpenCL_LIBRARY})
    set(OPTIONAL_SOURCES
        fractal_ocl.c
        ocl.c
        include/fractal_ocl.h
        )
endif()



add_executable(FractalCL
    fractal.c
    gui.c
	parameters.c
    include/fractal_complex.h
    include/fractal.h
    include/gui.h
    include/window.h
	include/parameters.h
    ${OPTIONAL_SOURCES}
)

target_link_libraries(FractalCL
    ${SDL_LDFLAGS} ${SDL_TTF_LDFLAGS}
    ${OPTIONAL_LIBRARIES} 
    -lm -lpthread
)

