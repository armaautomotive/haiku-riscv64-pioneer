#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-q8-domain-read.XXXXXXXX")
awk '/PIONEER_Q38_Q8_DOMAIN_READ_BEGIN/{copying=1;next}/PIONEER_Q38_Q8_DOMAIN_READ_END/{found=1;exit}copying{print}END{if(!found)exit 1}' \
 "$directory/../PioneerQ38PublicModelSource.h" > "$temporary/pioneer_q38_q8_domain_read.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$temporary" \
 "$directory/pioneer_q38_q8_domain_read_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline extracted inventory reader retained at %s\n' "$temporary"
