#!/bin/sh
# Actual lease + actual compiled instruction bytes, CPU-only fake MMIO.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-bfp-owner-tests.XXXXXXXX")
python3 "$directory/../build_native_bfp_diagnostic.py" "$1" "$temporary/build"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 -I"$temporary/build" "$directory/pioneer_native_bfp_owner_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary/build/bfp4" "$temporary/build/bfp8"
python3 "$directory/pioneer_native_bfp_close_test.py"
python3 "$directory/pioneer_native_bfp_control_test.py"
python3 "$directory/pioneer_native_bfp_mapping_test.py"
python3 "$directory/pioneer_native_bfp_parent_test.py"
printf 'CPU-only native BFP owner evidence retained: %s\n' "$temporary"
