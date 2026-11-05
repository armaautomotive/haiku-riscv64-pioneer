#!/bin/sh
set -eu
[ "$#" -le 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-block-state.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_q4_block_integer_state_test.cpp" -o "$temporary/state"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/state"
python3 "$directory/pioneer_q38_q4_block_integer_affine_roles_test.py"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_vector_q4_block_test.cpp" -o "$temporary/old-metadata"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/old-metadata"
if [ "$#" = 1 ];then
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DPIONEER_Q38_BLOCK_INTEGER_STATE_ORIGINAL_TEST=1 \
  "$directory/pioneer_q38_q4_block_integer_original_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/original"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/original" "$1"
fi
printf 'Offboard block-integer state evidence retained at %s\n' "$temporary"
