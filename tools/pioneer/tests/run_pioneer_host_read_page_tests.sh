#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-host-read-page.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_host_read_page_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: fixed 4KiB owned host read, 64-flit completion, private L1 backup, every lost write and unchanged cold/quarantine safeguards\n'
