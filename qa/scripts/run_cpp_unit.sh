#!/usr/bin/env bash
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build="$repo/qa/cpp_unit/build"
cmake -S "$repo/qa/cpp_unit" -B "$build" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$build" -j"$(nproc)"
ctest --test-dir "$build" --output-on-failure
