#!/bin/sh
# Test actual root plus BF16 peers;NO driver/card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
CXX="${CXX:-c++} -DPIONEER_Q38_TEST_BFLOAT" exec sh "$directory/run_pioneer_q38_vector_cooperative_tests.sh" "$@"
