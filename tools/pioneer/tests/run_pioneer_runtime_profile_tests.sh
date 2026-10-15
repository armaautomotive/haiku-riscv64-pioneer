#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-runtime-profile.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_runtime_profile_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: session CPU phase metadata, sequence, partial/finished masks, immutable snapshots and clock regressions; NO hardware\n'
