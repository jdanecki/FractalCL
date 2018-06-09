#!/bin/bash

set -e
mkdir -p build
cd build
cmake .. && make

ln -sf ../kernels/*.cl .
ln -sf ../*.ttf .
