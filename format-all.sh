#!/bin/bash

find . -not -path "*/external/*" \( -iname '*.h' -o -iname '*.cpp' \) | xargs clang-format -i --style=file
