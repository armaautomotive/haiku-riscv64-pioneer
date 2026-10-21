#!/bin/sh
# OFFLINE actual compiled peers, never a hardware runner.
set -eu
[ "$#" = 2 ] || { [ "$#" = 3 ] && [ "$3" = --work-pilot ]; } || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is(){ actual=$(shasum -a 256 "$1");[ "${actual%% *}" = "$2" ]; }
hash_is "$1/text.bin" 209fc6b8d40ed0ac7216c16ec9657ed0d83259e2ded53b35524c8cc6507bf0ef
hash_is "$1/affine.bin" 083330550580cc8ba821456c95b256ff8ea1d9742bb12230b06af469392f856a
hash_is "$2/text.bin" ec4db9c2c0c1c1297daa1fd6b25baabea17f7e1461b6b37a47811c7ee04dc573
hash_is "$2/affine.bin" 7a3f7b2dea02e85129c84b837794642a4f6b85ce4b4fbfdd52f3043697d2b230
hash_is "$2/fetch.bin" 5a8bd4a83693e947af8093e6607be8e4b678788334c384ce14748cf72074f298
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-signed-mag2-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION -DPIONEER_Q38_TEST_AFFINE_REPLAY -DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE \
 -DPIONEER_Q38_WORK_DEFER_AFFINE -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM \
 -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_REPLAY_OPERAND \
 -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_PIPELINED_MATRIX \
 -DPIONEER_Q38_INT_SFPU -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DPIONEER_Q38_TEST_INT_ACCUMULATOR \
 -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 "$directory/pioneer_q38_signed_magnitude2_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline actual signed magnitude2 evidence retained at %s;NO controller/native access\n' "$temporary"
