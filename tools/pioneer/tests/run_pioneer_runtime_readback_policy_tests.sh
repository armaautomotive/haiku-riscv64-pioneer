#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-readback-policy.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++11 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_runtime_readback_policy_test.cpp" -o "$temporary/test"
"$temporary/test"
