#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-resident-store.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_resident_store_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: persistent bank-0 chunk uploads/readback, exact three-step NoC proof, all lost writes, close rollback, no unsafe retry/free or remote read return\n'
