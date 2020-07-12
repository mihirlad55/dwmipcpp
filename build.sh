#!/usr/bin/bash
cmake -S . -B build/ -DBUILD_EXAMPLES:OPTION=ON
cd build/
make "$@"

