#!/bin/bash -eu

echo "Downloading clang 16..."
base_name=clang+llvm-16.0.0-x86_64-linux-gnu-ubuntu-18.04
archive_name=$base_name.tar.xz
curl -L -s --show-error -o "$archive_name" "https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.0/$archive_name"
tar -xf "$archive_name"

export CLANG_FORMAT="$base_name/bin/clang-format"

"$CLANG_FORMAT" --version

MELLOW=$(./find-mellow.sh)
echo "Mellow command: $MELLOW"

$MELLOW fetch
$MELLOW build
