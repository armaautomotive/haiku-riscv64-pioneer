#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-host-page.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_host_page_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: dedicated high-address host page, non-cacheable-before-write ordering, full patterns, mapping stability and exact ownership/cleanup; NO device access\n'
