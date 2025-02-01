#!/bin/bash

project_path="$1"

echo "Building \"${project_path}\" project..."
cd "$project_path"
rm -r build 2> /dev/null
mkdir build
cd build
cmake ..
make -j
