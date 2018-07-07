#!/bin/bash

git clone ../FractalCL FractalCL
docker build -f docker/Dockerfile-fedora-28 -t fedora28:ci .

