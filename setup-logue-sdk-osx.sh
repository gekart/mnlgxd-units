#!/bin/bash

git submodule update --init
cd logue-sdk
git submodule update --init
cd tools/gcc
./get_gcc_osx.sh
