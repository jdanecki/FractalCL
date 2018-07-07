#!/bin/bash

git clone ../FractalCL FractalCL
docker build -f docker/Dockerfile-fedora-27 -t fedora27:ci .

