#!/bin/bash

git clone ../FractalCL FractalCL
docker build -f docker/Dockerfile-fedora-31 -t fedora31:ci .

