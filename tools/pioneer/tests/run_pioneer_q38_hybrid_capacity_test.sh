#!/bin/sh
# CPU-only canonical-geometry arithmetic; does not allocate card memory.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-hybrid-capacity.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable \
 -fsanitize=address,undefined "$directory/pioneer_q38_hybrid_capacity_test.cpp" -o "$temporary/test"
"$temporary/test"
