#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-host-read.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_host_read_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: fixed owned high-address host read, exact NoC completion proof, BME-off rollback and wired quarantine after uncertainty; no retry or host writes\n'
