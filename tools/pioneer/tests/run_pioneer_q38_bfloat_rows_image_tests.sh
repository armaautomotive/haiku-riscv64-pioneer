#!/bin/sh
# Test actual separately compiled BF16 candidate;NO driver/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
CXX="${CXX:-c++} -DPIONEER_Q38_TEST_BFLOAT" exec sh "$directory/run_pioneer_q38_vector_rows_image_tests.sh" "$@"
