#!/bin/bash -eu

echo "Preparing dependencies..."

./download-pkg.sh bee https://github.com/bec-ca/bee/archive/refs/tags/v1.0.0.tar.gz
./download-pkg.sh command https://github.com/bec-ca/command/archive/refs/tags/v1.0.0.tar.gz
./download-pkg.sh diffo https://github.com/bec-ca/diffo/archive/refs/tags/v1.0.0.tar.gz
./download-pkg.sh yasf https://github.com/bec-ca/yasf/archive/refs/tags/v1.0.0.tar.gz

echo "Compiling mellow via bootstrap"

./bootstrap.sh

echo "Running mellow"

./build/bootstrap/mellow fetch
./build/bootstrap/mellow build
