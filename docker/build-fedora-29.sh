#!/bin/bash

git clone ../FractalCL FractalCL
docker build -f docker/Dockerfile-fedora-29 -t fedora29:ci .

