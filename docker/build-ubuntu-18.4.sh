#!/bin/bash

git clone ../FractalCL FractalCL
docker build -f docker/Dockerfile-ubuntu-18.04 -t ubuntu-18.04:ci .

