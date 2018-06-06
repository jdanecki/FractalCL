#!/bin/bash

for i in *.c *.cl
do
	clang-format -i $i
done

