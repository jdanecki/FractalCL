#!/bin/bash

git clone ../FractalCL FractalCL
docker build -f docker/Dockerfile-arch -t arch:ci .
