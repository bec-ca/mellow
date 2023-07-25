#!/bin/bash -eu

export CLANG_FORMAT="clang-format-16"

"$CLANG_FORMAT" --version

MELLOW=$(./find-mellow.sh)
echo "Mellow command: $MELLOW"

$MELLOW fetch
$MELLOW build
