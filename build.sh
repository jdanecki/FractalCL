#!/bin/bash

mkdir -p build
cd build
cmake ..
make

ln -sf ../*.cl .
ln -sf ../*.ttf .

