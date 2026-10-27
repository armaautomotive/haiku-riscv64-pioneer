#!/bin/sh
# CPU-only original reference diagnostic. Read-only SDK; outputs/tmp only.
# No installation, device FD, Work capability or driver build.
set -eu
python3 -c 'import os,sys; sys.exit(0 if not any(k.startswith("PIONEER_") for k in os.environ) else 2)'
[ "$#" = 2 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /private/tmp/*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
objects=$(mktemp -d /private/tmp/pioneer-q38-mixed-cpu-check.XXXXXXXX)
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*;do [ ! -d "$include" ] || set -- "$@" -isystem "$include";done
for library in "$build"/build_packages/gcc_syslibs-*/lib;do set -- "$@" -L"$library";done
umask 077
"$cxx" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 "$@" -c "$project/tools/pioneer/pioneer_q38_mixed_live_original_source.cpp" -o "$objects/original.o"
"$cxx" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 "$@" -c "$project/tools/pioneer/pioneer_q38_mixed_live_reference_check.cpp" -o "$objects/check.o"
"$cxx" "$@" -B"$release/system/glue/" -L"$release/system/libroot" "$objects/check.o" "$objects/original.o" -o "$output"
shasum -a 256 "$output" "$objects/check.o" "$objects/original.o"
printf 'CPU-only original-reference diagnostic built;objects at %s;NO installation/driver/device/Work/inference-speed grant\n' "$objects"
