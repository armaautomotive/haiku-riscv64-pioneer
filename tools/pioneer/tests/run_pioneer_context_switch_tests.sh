#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-context-switch.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/pioneer_context_switch.inc"; rmdir "$temporary"' EXIT HUP INT TERM
awk '
 /^arch_thread_context_switch\(Thread \*from, Thread \*to\)$/ { copying = 1; next }
 copying && /^void$/ { found = 1; exit }
 copying { print }
 END { if (!found) exit 1 }
' "$directory/../../../src/system/kernel/arch/riscv64/arch_thread.cpp" > "$temporary/pioneer_context_switch.inc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" "$directory/pioneer_context_switch_test.cpp" -o "$temporary/test"
"$temporary/test"
