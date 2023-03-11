#!/bin/bash -eu

MELLOW=$(./find-mellow.sh)

echo "Mellow command: $MELLOW"

$MELLOW fetch
$MELLOW build
