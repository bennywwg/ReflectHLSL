#/bin/bash
git submodule update --init --recursive
cd parsegen
cmake ./
make
cd ../