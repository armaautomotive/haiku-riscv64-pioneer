#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-public-repeat-loop.XXXXXXXX")
source=$directory/../pioneer_q38_inference.cpp
for section in STOP REPEAT_LOOP; do
 case "$section" in STOP) name=stop;; REPEAT_LOOP) name=repeat_loop;; esac
 awk -v begin="PIONEER_PUBLIC_${section}_BEGIN" -v end="PIONEER_PUBLIC_${section}_END" '
  index($0,begin) {copying=1;next} index($0,end) && copying {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/pioneer_public_$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_public_repeat_loop_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline actual public-repeat loop evidence retained at %s\n' "$temporary"
