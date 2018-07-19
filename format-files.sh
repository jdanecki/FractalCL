#!/bin/bash

for i in *.c kernels/*.cl include/*.h tests/*.c kernels/*.h tests/neurons/*.c
do
	clang-format -i $i
done

