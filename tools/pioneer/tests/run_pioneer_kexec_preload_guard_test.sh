#!/bin/sh
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-preload-guard.XXXXXX")
trap 'rm -f "$temporary/guard-test" "$temporary/pioneer_preload_guard.inc"; rmdir "$temporary"' EXIT HUP INT TERM
# Use the production guard, replacing only the later normal platform body.
awk '
 /^platform_start_kernel\(void\)$/ { copying = 1; next }
 copying && /^\tstatic struct kernel_args\* args = &gKernelArgs;/ {
   print "normalReached = true;"; print "}"; found = 1; exit
 }
 copying { print }
 END { if (!found) exit 1 }
' "$test_dir/../../../src/system/boot/platform/riscv/start.cpp" > "$temporary/pioneer_preload_guard.inc"
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" "$test_dir/pioneer_kexec_preload_guard_test.cpp" -o "$temporary/guard-test"
"$temporary/guard-test"
