#!/bin/bash

for i in *.c kernels/*.cl include/*.h tests/*.c kernels/*.h
do
	clang-format -i $i
done
