#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-host-path.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_host_path_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: read-only 64-bit PCIe SII / inbound-path audit, full-width TLB encoding, write-loss rollback and route/state refusal; NO DMA\n'
