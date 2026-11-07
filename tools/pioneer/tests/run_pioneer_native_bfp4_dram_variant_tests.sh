#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-bfp4-variant.XXXXXXXX")
common='-std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined'
${CXX:-c++} $common "$directory/pioneer_native_bfp4_dram_variant_test.cpp" -o "$temporary/legacy"
${CXX:-c++} $common -DPIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC=1 "$directory/pioneer_native_bfp4_dram_variant_test.cpp" -o "$temporary/matched"
"$temporary/legacy"
"$temporary/matched"
if ${CXX:-c++} $common -DPIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC=0 -fsyntax-only "$directory/pioneer_native_bfp4_dram_variant_test.cpp" 2> "$temporary/zero.stderr";then exit 1;fi
rg -q 'matched-input BFP4 diagnostic role must be explicitly 1' "$temporary/zero.stderr"
