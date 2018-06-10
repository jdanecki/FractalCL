#!/bin/bash

for i in *.c kernels/*.cl include/*.h
do
	clang-format -i $i
done
