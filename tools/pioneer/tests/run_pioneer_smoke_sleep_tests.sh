#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-smoke-sleep.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/pioneer_sleep.inc"; rmdir "$temporary"' EXIT HUP INT TERM
awk '
 /PIONEER_SMOKE_SLEEP_BEGIN/ {copying=1; next}
 /PIONEER_SMOKE_SLEEP_END/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$directory/../pioneer_smp_smoke.c" > "$temporary/pioneer_sleep.inc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" "$directory/pioneer_smoke_sleep_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: production CPU smoke sleep keeps one deadline, reports interruptions, preserves errors and bounds retries\n'
