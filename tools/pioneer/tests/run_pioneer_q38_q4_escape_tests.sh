#!/bin/sh
# SPDX-License-Identifier: MIT
# Pure lossless DATA tests only: no model download, driver or device access.
set -eu
cd "$(dirname "$0")/../../.."
q4_escape_build=$(mktemp -d /private/tmp/pioneer-q4-escape-tests.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror \
 -fsanitize=address,undefined tools/pioneer/tests/pioneer_q38_q4_escape_tiles_test.cpp \
 -o "$q4_escape_build/test"
"$q4_escape_build/test"
"${CXX:-c++}" -std=c++17 -O3 -Wall -Wextra -Werror \
 tools/pioneer/tests/pioneer_q38_q4_escape_tiles_test.cpp -o "$q4_escape_build/test-optimized"
"$q4_escape_build/test-optimized"
for q4_escape_define in _KERNEL_MODE PIONEER_ATTENTION_FREESTANDING PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION __FAST_MATH__ NDEBUG; do
 if "${CXX:-c++}" -std=c++17 -D"$q4_escape_define" -fsyntax-only \
  tools/pioneer/tests/pioneer_q38_q4_escape_tiles_test.cpp > "$q4_escape_build/$q4_escape_define.stdout" \
  2> "$q4_escape_build/$q4_escape_define.stderr"; then
  printf 'FAIL: forbidden DATA-test build accepted %s\n' "$q4_escape_define" >&2
  exit 1
 fi
done
printf 'PASS_Q4_ESCAPE_SANITIZED_OPTIMIZED_AND_SIX_BUILD_REFUSALS; artifacts %s\n' "$q4_escape_build"
