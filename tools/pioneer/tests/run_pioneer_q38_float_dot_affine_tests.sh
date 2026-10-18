#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
CXX="${CXX:-c++} -DPIONEER_Q38_AFFINE_FLOAT_DOT_TEST" \
 exec sh "$directory/run_pioneer_q38_vector_affine_tests.sh" "$@"
