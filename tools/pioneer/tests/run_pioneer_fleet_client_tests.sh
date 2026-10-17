#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-fleet-client.XXXXXXXX")
${CXX:-c++} -std=c++11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/cli-stubs" "$directory/pioneer_fleet_client_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" > "$temporary/test.log" 2>&1
tail -n 1 "$temporary/test.log"
printf 'Offline fleet client test retained at %s\n' "$temporary"
