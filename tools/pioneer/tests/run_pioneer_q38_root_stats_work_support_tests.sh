#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-root-stats-work-support.XXXXXXXX")
awk '/PIONEER_NATIVE_Q8_WORK_SUPPORT_BEGIN/ {copying=1;next} /PIONEER_NATIVE_Q8_WORK_SUPPORT_END/ {found=1;exit}
 copying {print} END {if(!found)exit 1}' "$directory/../PioneerQ38WorkSession.h" > "$temporary/root_stats_work_support.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_root_stats_work_support_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline actual root-stats Work support evidence retained at %s\n' "$temporary"
