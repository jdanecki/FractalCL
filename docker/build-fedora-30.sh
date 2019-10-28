#!/bin/bash

git clone ../FractalCL FractalCL
docker build -f docker/Dockerfile-fedora-30 -t fedora30:ci .

