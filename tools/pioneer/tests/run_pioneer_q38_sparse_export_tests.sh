#!/bin/sh
# OFFLINE ONLY: emitted root+peer instructions, not a hardware benchmark.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is() { actual=$(shasum -a 256 "$1"); [ "${actual%% *}" = "$2" ]; }
hash_is "$1/text.bin" 8163fb311edbc4d86983a55bd3961b9e6932ee2b0c48ed4b421bcb21862596b5
hash_is "$1/output.bin" 50c328edcdd289e56652e463bf414185a8f7d69be71e98374e0cd83a09587dce
hash_is "$2/text.bin" 73b7170e8daf0caa3db5e9a9f4a3306b6c64b648a254609ee77f0e95ec954900
hash_is "$2/output.bin" fc7337cc327bb186049ef40279e556b796d0cbb436850fefb77aa361d0224627
case "${PIONEER_Q38_TEST_INPUT_SUM_CACHE:-0}" in
 0) peer_cpp=
    hash_is "$3/text.bin" 8d0a4339c676509ff45c8802f00cb2a875dd4644537df158a9087464fc234a5a
    hash_is "$3/affine.bin" 521420fa2a52603dc4c77f95cfab3df78f36f59532de38d8a99d7e2244ef892d;;
 1) peer_cpp=-DPIONEER_Q38_WORK_INPUT_SUM_CACHE
    hash_is "$3/text.bin" c2f5dbf162261221c0cdfafe45e24f778aaae32b180dbaa01453f6c4881f87d0
    hash_is "$3/affine.bin" f5b36fa25338b9e63f8a10f0cf6bb60cc397e91261bfececcec603eab1b94250;;
 2) peer_cpp='-DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM'
    hash_is "$3/text.bin" 9a3f8f13b3dc8d147bd07df92f8437bc0b681e7298a4fde36166bd1a2cde07a8
    hash_is "$3/affine.bin" 9258c8b82bc0d419d4497f1ab816e5e0bc54e1fe6f34e67afa0c31a5088b0e3c;;
 *) exit 2;;
esac
hash_is "$3/boot.bin" e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b
for root in "$1" "$2"; do
 hash_is "$root/boot.bin" e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b
done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-sparse-export.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror $peer_cpp \
 -fsanitize=address,undefined -fno-omit-frame-pointer \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS \
 -DPIONEER_Q38_WORK_COHORT_CACHE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK \
 -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_INT_SFPU -DPIONEER_Q38_PIPELINED_MATRIX \
 -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_REPLAY_OPERAND \
 -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST -DPIONEER_Q38_QUEUED_MATRIX_ENTRY \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 "$directory/pioneer_q38_sparse_export_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2" "$3"
printf 'Offline sparse-export instruction evidence retained at %s\n' "$temporary"
