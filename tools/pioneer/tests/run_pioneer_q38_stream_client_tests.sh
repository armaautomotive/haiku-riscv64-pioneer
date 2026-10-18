#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-stream-client.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/cli-stubs" "$directory/pioneer_q38_stream_client_test.cpp" -o "$temporary/test"
if UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" > "$temporary/test.log" 2>&1; then
 tail -n 1 "$temporary/test.log"
else
 tail -n 30 "$temporary/test.log"
 exit 1
fi
printf 'Offline fullK real-client evidence retained at %s\n' "$temporary"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_STREAM_DIAGNOSTICS -I"$directory/cli-stubs" "$directory/pioneer_q38_stream_client_test.cpp" -o "$temporary/diagnostic"
if UBSAN_OPTIONS=halt_on_error=1 "$temporary/diagnostic" "$1" > "$temporary/diagnostic.log" 2>&1; then
 tail -n 1 "$temporary/diagnostic.log"
else
 tail -n 40 "$temporary/diagnostic.log"
 exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS -DPIONEER_Q38_STREAM_DIAGNOSTICS -I"$directory/cli-stubs" \
 "$directory/pioneer_q38_stream_client_test.cpp" -o "$temporary/rows"
if UBSAN_OPTIONS=halt_on_error=1 "$temporary/rows" "$1" > "$temporary/rows.log" 2>&1; then
 tail -n 1 "$temporary/rows.log"
else
 tail -n 40 "$temporary/rows.log"
 exit 1
fi
