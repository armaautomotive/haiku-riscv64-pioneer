#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-kdl-timer.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/pioneer_kdl_timer.inc"; rmdir "$temporary"' EXIT HUP INT TERM
awk '
 /PIONEER_DIAGNOSTIC_KDL_TIMER_BEGIN/ {copying=1; next}
 /PIONEER_DIAGNOSTIC_KDL_TIMER_END/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$directory/../../../src/system/kernel/main.cpp" > "$temporary/pioneer_kdl_timer.inc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" "$directory/pioneer_kdl_timer_test.cpp" -o "$temporary/test"
"$temporary/test"
awk '
 /\/\/ welcome to the machine/ {ap=1}
 /arm_pioneer_diagnostic_kdl_timer\(currentCPU\);/ {calls++; if(!ap) exit 1}
 END {if(calls != 1) exit 1}
' "$directory/../../../src/system/kernel/main.cpp"
printf 'PASS: production diagnostic timer uses only AP1/full-memory/64-CPU opt-in, fixed one-shot and explicit debugger halt\n'
