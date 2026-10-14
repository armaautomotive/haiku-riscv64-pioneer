#!/bin/sh
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-handoff-tests.XXXXXX")
trap 'rm -f "$temporary/handoff-test" "$temporary/pioneer_handoff.inc"; rmdir "$temporary"' EXIT HUP INT TERM
awk '
 /^pioneer_prepare_kernel\(preloaded_elf64_image\* image\)$/ { copying = 1; next }
 copying && /^#endif$/ { found = 1; exit }
 copying { print }
 END { if (!found) exit 1 }
' "$test_dir/../../../src/system/boot/platform/riscv/start.cpp" > "$temporary/pioneer_handoff.inc"
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" "$test_dir/pioneer_kexec_handoff_test.cpp" -o "$temporary/handoff-test"
"$temporary/handoff-test"
