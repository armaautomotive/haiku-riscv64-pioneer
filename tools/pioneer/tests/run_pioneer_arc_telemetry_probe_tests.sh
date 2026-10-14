#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-telem-probe.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_arc_telemetry_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: bounded whitelisted telemetry, malformed-pointer/table rejection, exact TLB restoration\n'
