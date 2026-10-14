#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-mmio-probe.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_mmio_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: fixed MMIO policy rejects unsafe layouts; bounds reads, forbids DMA, and checks rollback\n'
