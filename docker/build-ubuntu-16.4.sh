#!/bin/bash

git clone ../FractalCL FractalCL
docker build -f docker/Dockerfile-ubuntu-16.04 -t ubuntu-16.04:ci .

