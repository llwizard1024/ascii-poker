#!/bin/bash
set -e

rm -rf build
mkdir build
cd build

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..

arg=${1:-} # poker_client OR poker_server

if [[ -n $arg ]]; then
  cmake --build . --target $arg -j$(nproc)
else
  cmake --build . -j$(nproc)
fi