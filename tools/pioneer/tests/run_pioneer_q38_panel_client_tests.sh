#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-panel-client.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/cli-stubs" "$directory/pioneer_q38_panel_client_test.cpp" -o "$temporary/test"
if UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" > "$temporary/test.log" 2>&1; then
 tail -n 1 "$temporary/test.log"
else
 tail -n 30 "$temporary/test.log"
 exit 1
fi
printf 'Offline local-panel client evidence retained at %s\n' "$temporary"
