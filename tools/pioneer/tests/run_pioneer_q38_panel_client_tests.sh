#!/bin/sh
set -eu
[ "$#" -le 1 ] || exit 2
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
if [ "$#" = 1 ]; then
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DPIONEER_Q38_PUBLIC_FIXTURES -I"$directory/cli-stubs" \
  "$directory/pioneer_q38_panel_client_test.cpp" -o "$temporary/fixture"
 if UBSAN_OPTIONS=halt_on_error=1 "$temporary/fixture" "$1" > "$temporary/fixture.log" 2>&1; then
  tail -n 1 "$temporary/fixture.log"
 else
  tail -n 30 "$temporary/fixture.log"
  exit 1
 fi
fi
printf 'Offline local-panel client evidence retained at %s\n' "$temporary"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -I"$directory/cli-stubs" "$directory/pioneer_q38_panel_client_test.cpp" -o "$temporary/affine"
if UBSAN_OPTIONS=halt_on_error=1 "$temporary/affine" > "$temporary/affine.log" 2>&1; then
 tail -n 1 "$temporary/affine.log"
else
 tail -n 30 "$temporary/affine.log"
 exit 1
fi
