#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
test "$(shasum -a 256 "$1/text.bin" | cut -d ' ' -f 1)" = 377b6a60ec8eb054e8f28c59bccbc4380d62b78cf01c1663033a80e83fa116ae
test "$(shasum -a 256 "$1/boot.bin" | cut -d ' ' -f 1)" = e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-work-batch.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_REUSE_CROSS_QUANT \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DTT_PIONEER_Q38_WORK_MODE \
 "$directory/pioneer_q38_work_batch_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'Offline batch evidence retained at %s\n' "$temporary"
