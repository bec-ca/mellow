#!/bin/bash -eu

function log {
  echo "$@" 1>&2
}

function build_bootstrap {
  log "Preparing dependencies..."

  ./download-pkg.sh bee https://github.com/bec-ca/bee/archive/refs/tags/v1.0.0.tar.gz
  ./download-pkg.sh command https://github.com/bec-ca/command/archive/refs/tags/v1.0.0.tar.gz
  ./download-pkg.sh diffo https://github.com/bec-ca/diffo/archive/refs/tags/v1.0.0.tar.gz
  ./download-pkg.sh yasf https://github.com/bec-ca/yasf/archive/refs/tags/v1.0.0.tar.gz

  log "Building bootstrap mellow..."
  make -f Makefile.bootstrap -j$(nproc)
}

function check_mellow {
  if [ -z "$1" ]; then
    return 1
  fi
  if ! "$1" help &> /dev/null; then
    log "Tried mellow at $1 but returned an error"
    return 1
  fi
  echo "$1"
  return 0
}

if [ -z ${FORCE_BOOTSTRAP:-} ]; then
  if check_mellow "${MELLOW:-}"; then
    exit 0
  fi
  if check_mellow "mellow"; then
    exit 0
  fi
fi

if check_mellow "./build/bootstrap/mellow/mellow"; then
  exit 0
fi

log "No mellow binary found, will build bootstrap"

build_bootstrap 1>&2

if check_mellow "$bootstrap_path"; then
  exit 0
fi

echo "Bootstrap build returned successfully, but binary not found"
exit 1