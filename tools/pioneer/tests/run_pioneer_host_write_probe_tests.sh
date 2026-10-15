#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-host-write.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_host_write_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: fixed 64B/4KiB card-to-owned-host writes, explicit host visibility, distinct L1 readback, all lost writes, no retry or release after uncertain completion\n'
