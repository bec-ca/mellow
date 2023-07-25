#!/bin/bash -eu

MELLOW=$(./find-mellow.sh)

echo "Mellow command: $MELLOW"

export CLANG_FORMAT=clang-format-16

$MELLOW fetch
$MELLOW build
