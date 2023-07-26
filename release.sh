#!/bin/bash -eu

./ci.sh

stage_dir=.staging
pkg_name=mellow-with-deps
pkg_dir=$stage_dir/$pkg_name
pkg_file=$pkg_name.tar.gz

rm -rf $stage_dir
mkdir -p $pkg_dir
cp -r mellow mbuild Makefile.bootstrap $pkg_dir/
mkdir -p $pkg_dir/build
cp -r build/external-packages $pkg_dir/build/
tar -czf $pkg_file -C $stage_dir $pkg_name

echo "Created package $pkg_file"
