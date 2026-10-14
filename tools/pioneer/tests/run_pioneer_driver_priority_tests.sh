#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-driver-priority.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
awk '
$0 == "get_priority(const char* path)" { print "static int32"; copy = 1 }
copy { print }
copy && $0 == "}" { exit }
' "$directory/../../../src/system/kernel/device_manager/legacy_drivers.cpp" > "$temporary/production_priority.inc"
test -s "$temporary/production_priority.inc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I "$temporary" "$directory/pioneer_driver_priority_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: production driver priority, nested non-packaged paths and component boundaries\n'
