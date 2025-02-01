#!/bin/bash

files=$(find . -regex '.*\.\(cpp\|hpp\|h\)' -not -path "*/build/*" -not -path "*/.build/*" -not -path "*/cmake-*/*" 2> /dev/null)
echo $files
echo $files | xargs clang-format --style=file -i
