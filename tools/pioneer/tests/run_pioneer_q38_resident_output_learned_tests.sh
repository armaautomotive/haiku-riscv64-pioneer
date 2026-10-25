#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-output-learned-tests.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$here/pioneer_q38_resident_output_learned_test.cpp" -o "$build/test"
UBSAN_OPTIONS=halt_on_error=1 "$build/test"
