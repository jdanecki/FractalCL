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

FP_64_SUPPORT=1
SDL_ACCELERATED=0

OPENCL_LIB=$(shell pkg-config --libs OpenCL)
SDL2_LIB=$(shell pkg-config --libs sdl2 SDL2_ttf)
SDL2_CFLAGS=$(shell pkg-config --cflags sdl2 SDL2_ttf)

INCLUDE=../include
CFLAGS=-g -Wall -Wno-deprecated-declarations -I$(INCLUDE) 

ifeq ($(FP_64_SUPPORT), 1)
CFLAGS+=-DFP_64_SUPPORT=1
endif

ifeq ($(SDL_ACCELERATED), 1)
CFLAGS+=-DSDL_ACCELERATED=1
endif

all: ocl sdl

ocl: ../ocl.c test_ocl.c Makefile
	gcc -o test_$@ test_ocl.c ../ocl.c $(CFLAGS) $(OPENCL_LIB) -lm -lrt -lpthread -ldl 

sdl: ../gui.c test_sdl.c Makefile
	gcc -o test_$@ test_sdl.c ../gui.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl

