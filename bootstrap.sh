#!/bin/bash -eu

sources="$(find mellow build/external-packages | grep '\.cpp$' | grep -v main | grep -v test | grep -v 'sys\..pp') mellow/mellow_main.cpp"

mkdir -p build/bootstrap

clang++ $sources  -o build/bootstrap/mellow -iquote . -iquote build/external-packages -std=c++20 -O3 -flto -pthread -lpthread
