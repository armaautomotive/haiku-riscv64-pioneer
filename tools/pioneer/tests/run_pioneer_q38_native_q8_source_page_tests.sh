#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-native-q8-source-page.XXXXXXXX")
awk '/PIONEER_Q38_AUTHENTICATED_WEIGHT_PAGE_BEGIN/{copying=1;next}/PIONEER_Q38_AUTHENTICATED_WEIGHT_PAGE_END/{found=1;exit}copying{print}END{if(!found)exit 1}' \
 "$directory/../PioneerQ38PublicModelSource.h" > "$temporary/pioneer_q38_native_q8_source_page.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DPIONEER_Q38_NATIVE_Q8_LAYOUT -I"$temporary" \
 "$directory/pioneer_q38_native_q8_source_page_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline extracted source-page evidence retained at %s\n' "$temporary"
