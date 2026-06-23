#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found" >&2
    exit 1
fi

files=()
while IFS= read -r path; do
    case "$path" in
        third_party/* | tests/external/* | */ftxui/*) continue ;;
    esac
    if [ -f "$path" ]; then
        files+=("$path")
    fi
done < <(git ls-files '*.cpp' '*.h')

if [ "${#files[@]}" -eq 0 ]; then
    echo "No source files to check"
    exit 0
fi

clang-format --dry-run --Werror "${files[@]}"
