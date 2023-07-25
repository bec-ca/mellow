#!/bin/bash -eu

MELLOW=$(./find-mellow.sh)

echo "Mellow command: $MELLOW"

compgen -c
export CLANG_FORMAT=clang-format16

$MELLOW fetch
$MELLOW build
