#!/bin/bash -eu

./download-pkg.sh bee https://github.com/bec-ca/bee/archive/refs/tags/v2.2.1.tar.gz
./download-pkg.sh command https://github.com/bec-ca/command/archive/refs/tags/v2.1.0.tar.gz
./download-pkg.sh diffo https://github.com/bec-ca/diffo/archive/refs/tags/v2.0.0.tar.gz
./download-pkg.sh yasf https://github.com/bec-ca/yasf/archive/refs/tags/v2.0.2.tar.gz

stage_dir=.staging
pkg_name=mellow-with-deps
pkg_dir=$stage_dir/$pkg_name
pkg_file=$pkg_name.tar.gz
mkdir -p $pkg_dir
cp -r mellow $pkg_dir/
cp -r build/external-packages/* $pkg_dir/
cp mbuild $pkg_dir/
tar -czf $pkg_file -C $stage_dir $pkg_name

echo "Created package $pkg_file"
