#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-host-dma-audit.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_host_dma_audit_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: full-address DBI route, read-only PCIe pending/error and firmware HDMA ch0 audit, all setup/rollback write faults; no bus mastering\n'
