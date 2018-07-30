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

SDL_ACCELERATED=1

SDL2_LIB=$(shell pkg-config --libs sdl2 SDL2_ttf SDL2_gfx)
SDL2_CFLAGS=$(shell pkg-config --cflags sdl2 SDL2_ttf SDL2_gfx)

CFLAGS=-g -Wall -Wno-deprecated-declarations -I../../include -DDATA_PATH=`pwd`/../..

ifeq ($(SDL_ACCELERATED), 1)
CFLAGS+=-DSDL_ACCELERATED=1
endif

all: test_equations test_linear test_non_linear test_neurons

test_equations: ../../gui.c test_equations.c Makefile
	gcc -o $@ $@.c ../../gui.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl

test_linear: ../../gui.c test_linear.c Makefile
	gcc -o $@ $@.c ../../gui.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl

test_non_linear: ../../gui.c test_non_linear.c Makefile
	gcc -o $@ $@.c ../../gui.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl

test_neurons: ../../gui.c test_neurons.c Makefile
	gcc -o $@ $@.c ../../gui.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIB) -lm -lrt -lpthread -ldl

