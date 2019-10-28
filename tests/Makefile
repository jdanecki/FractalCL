# Copyright (C) 2018-2019  Jacek Danecki <jacek.m.danecki@gmail.com>
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

FP_64_SUPPORT=1
SDL_ACCELERATED=1

OPENCL_LIB=$(shell pkg-config --libs OpenCL)
SDL2_LIB=$(shell pkg-config --libs sdl2 SDL2_ttf SDL2_gfx)
SDL2_CFLAGS=$(shell pkg-config --cflags sdl2 SDL2_ttf SDL2_gfx)

CFLAGS=-g -Wall -Wno-deprecated-declarations -I../include -Ikernels -DHOST_APP

ifeq ($(FP_64_SUPPORT), 1)
CFLAGS+=-DFP_64_SUPPORT=1
endif

ifeq ($(SDL_ACCELERATED), 1)
CFLAGS+=-DSDL_ACCELERATED=1
endif

all: test_ocl test_sdl test_complex test_sdl_render test_fractal test_plasma test_neurons test_iter \
	search_fractal test_mpfr test_fractal_mpfr test_fractal_gmp test_gtk test_inter test_perf

test_ocl: ../ocl.c ../timer.c test_ocl.c Makefile
	gcc -o $@ test_ocl.c ../ocl.c ../timer.c $(CFLAGS) $(OPENCL_LIB) -lm -lrt -lpthread -ldl -DDATA_PATH=`pwd`

test_sdl: ../gui.c ../timer.c test_sdl.c Makefile
	gcc -o $@ $@.c ../gui.c ../timer.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl -DDATA_PATH=`pwd`/..

test_perf: ../gui.c ../timer.c test_perf.c Makefile ../ocl.c
	gcc -o $@ $@.c ../gui.c ../timer.c ../ocl.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) $(OPENCL_LIB) -lm -lrt -lpthread -ldl -DDATA_PATH=`pwd`/..

test_plasma: ../gui.c test_plasma.c Makefile
	gcc -o $@ $@.c ../gui.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl -DDATA_PATH=`pwd`/..

test_sdl_render: ../gui.c ../timer.c test_sdl_render.c Makefile
	gcc -o $@ $@.c ../gui.c ../timer.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl -DDATA_PATH=`pwd`/..

test_fractal: ../gui.c ../timer.c test_fractal.c Makefile
	gcc -o $@ $@.c ../gui.c ../timer.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl -DDATA_PATH=`pwd`/..

search_fractal: ../gui.c ../timer.c search_fractal.c Makefile
	gcc -o $@ $@.c ../gui.c ../timer.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl -DDATA_PATH=`pwd`/..

test_fractal_mpfr: ../gui.c ../timer.c test_fractal_mpfr.c Makefile
	gcc -o $@ $@.c ../gui.c ../timer.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl -lmpfr -DDATA_PATH=`pwd`/.. -mfpmath=sse -march=native -msse2 -mtune=native

test_mpfr: test_mpfr.c Makefile
	gcc -o $@ $@.c -g -Wall -lmpfr -mfpmath=sse -march=native -msse2 -mtune=native

test_fractal_gmp: ../gui.c ../timer.c test_fractal_gmp.c Makefile
	gcc -o $@ $@.c ../gui.c ../timer.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl -lgmp -DDATA_PATH=`pwd`/.. -mfpmath=sse -march=native -msse2 -mtune=native

test_gtk:  test_gtk.c ../timer.c
	gcc test_gtk.c -g -o test_gtk `pkg-config gtk+-3.0 --cflags --libs` -lm -I../include ../timer.c

test_complex: test_complex.c Makefile
	gcc -o $@ $@.c $(CFLAGS)

test_inter: test_inter.c Makefile
	gcc -o $@ $@.c $(CFLAGS)

test_iter: test_iter.c Makefile
	gcc -o $@ $@.c $(CFLAGS) -lm

test_neurons:
	make -C neurons

