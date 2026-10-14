#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-bar4-probe.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_bar4_address_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: BAR4 test refuses active/assigned devices, never enables decoding, and verifies restoration\n'
