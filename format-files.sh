#!/bin/bash

for i in *.c *.cl include/*.h
do
	clang-format -i $i
done

