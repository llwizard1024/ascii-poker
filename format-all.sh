#!/bin/bash

find . -not -path "*/external/*" -not -path "*/include/*" \( -iname '*.h' -o -iname '*.cpp' \) | xargs clang-format -i --style=file
