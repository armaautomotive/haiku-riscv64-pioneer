#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-persistent-policy.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined "$directory/pioneer_persistent_policy_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'Persistent policy test retained at %s\n' "$temporary"
