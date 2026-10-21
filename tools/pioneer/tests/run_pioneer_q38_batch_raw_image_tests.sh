#!/bin/sh
set -eu
[ "$#" = 2 ] || { [ "$#" = 3 ] && [ "$3" = --work-pilot ]; } || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is(){ actual=$(shasum -a 256 "$1");[ "${actual%% *}" = "$2" ]; }
hash_is "$1/text.bin" 8ea00560451af37e708f2f45f9660627834a2202b01f5f3ba03969c40e878beb
hash_is "$1/affine.bin" 7a3f7b2dea02e85129c84b837794642a4f6b85ce4b4fbfdd52f3043697d2b230
hash_is "$1/fetch.bin" b6316e76b73f0e9db9bb240cbff5f5104d1b8faba959996428ac16ef56b49c33
hash_is "$2/text.bin" 6f7ceb71adf49d35330a3e02cc2a5678289272b27ddaab1e3dc160c8dc4dd498
hash_is "$2/affine.bin" 1568cfc02a808a218551d0683642429f203217f5a21254d857c4d846a1d82b35
hash_is "$2/fetch.bin" 0986412151b4c104cf02483d6c6b5a88b2887167beda1e88bf1c31b0e1b5c86b
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-batch-raw-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION -DPIONEER_Q38_TEST_AFFINE_REPLAY -DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE \
 -DPIONEER_Q38_WORK_DEFER_AFFINE -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM \
 -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_REPLAY_OPERAND \
 -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_PIPELINED_MATRIX \
 -DPIONEER_Q38_INT_SFPU -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DPIONEER_Q38_TEST_INT_ACCUMULATOR \
 -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 "$directory/pioneer_q38_batch_raw_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline direct batch-input actual peer evidence retained at %s;NO controller/native access\n' "$temporary"
