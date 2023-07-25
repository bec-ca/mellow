#!/bin/bash -eu

pkg_name=$1
url=$2

echo "Downloading $pkg_name..."

repo_dir=$PWD

pkg_dir=$repo_dir/build/external-packages
mkdir -p $pkg_dir

tmp_dir=$(mktemp -d)
cd $tmp_dir

curl -L -s --show-error -o $pkg_name.tar.gz $url
tar -xf $pkg_name.tar.gz

rm -rf $pkg_dir/$pkg_name
mv */$pkg_name $pkg_dir/$pkg_name
