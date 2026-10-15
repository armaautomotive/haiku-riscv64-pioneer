#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-runtime-output.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_runtime_output_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: owned output DMA, posted-write visibility/order, no payload PCI reads, faults quarantine without accepting output\n'
