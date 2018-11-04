#!/usr/bin/env sh
FILE_LIST="$(find . -type f -name '*.cpp' -o -name '*.h' -o -name '*.cuh' -o -name '*.cu' | grep -v build | grep -v external)"
clang-format -i -style=file $FILE_LIST
