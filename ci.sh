#!/bin/bash -eux

export CXX=clang-18
export CLANG_FORMAT=clang-format-18
export LDLAGS="-lstdc++ -lm"

MELLOW_VERSION="v0.0.6"

export MELLOW=build/mellow-bootstrap-$MELLOW_VERSION


function build_bootstrap() {
  echo "Downloading and compiling mellow..."
  bootstrap_dir=.bootstrap
  mellow_dir=$bootstrap_dir/mellow-with-deps

  rm -rf $bootstrap_dir
  mkdir $bootstrap_dir
  pushd $bootstrap_dir
  curl -s -L "https://github.com/bec-ca/mellow/releases/download/${MELLOW_VERSION}/mellow-with-deps-${MELLOW_VERSION}.tar.gz" -o mellow-with-deps.tar.gz
  tar -xf mellow-with-deps.tar.gz
  popd

  pushd $mellow_dir
  make -j $(nproc) -f Makefile.bootstrap
  popd

  mkdir -p build
  cp $mellow_dir/build/bootstrap/mellow/mellow $MELLOW
}

if ! [ -f $MELLOW ]; then
  build_bootstrap
fi

make fetch
PROFILE=release make
