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

OPENCL_LIB=$(shell pkg-config --libs OpenCL)
INCLUDE=../include
FP_64_SUPPORT=0
CFLAGS="-march=native -mtune=native -g -Wall -Wno-deprecated-declarations" 

ifeq ($(FP_64_SUPPORT), 1)
CFLAGS="$(CFLAGS) -DFP_64_SUPPORT=1"
endif

test_ocl: ../ocl.c test_ocl.c
	gcc -g -Wall -o $@ test_ocl.c ../ocl.c -I$(INCLUDE) ${OPENCL_LIB} -lm -lrt -lpthread -ldl -Wno-deprecated-declarations

