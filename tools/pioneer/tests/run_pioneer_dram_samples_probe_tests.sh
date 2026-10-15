#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-dram-samples.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_dram_samples_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: seven DRAM routes, 120-core ownership, reversible distinct high-address samples and fault/alias tests\n'
