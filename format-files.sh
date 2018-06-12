#!/bin/bash

for i in *.c kernels/*.cl include/*.h tests/*.c
do
	clang-format -i $i
done
