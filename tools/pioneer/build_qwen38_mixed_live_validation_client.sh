#!/bin/sh
[ "${PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE+x}" = "" ] || exit 2
[ "${PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE+x}" != x ] || exit 2
set -eu
# Build ONLY. Observer and client are separate TUs;NO native/Work flags.
python3 -c 'import os,sys; sys.exit(0 if not any(k.startswith(("PIONEER_","TT_PIONEER_")) for k in os.environ) else 2)' || exit 2
[ "$#" = 2 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
objects=$(mktemp -d /private/tmp/pioneer-q38-dense-debug70-client.XXXXXXXX)
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*;do [ ! -d "$include" ] || set -- "$@" -isystem "$include";done
for library in "$build"/build_packages/gcc_syslibs-*/lib;do set -- "$@" -L"$library";done
umask 077
"$cxx" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 "$@" -c "$project/tools/pioneer/pioneer_q38_mixed_live_original_source.cpp" -o "$objects/original.o"
"$cxx" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 -DPIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT=1 -DPIONEER_Q38_CPU_VALIDATION=1 \
 "$@" -c "$project/tools/pioneer/pioneer_q38_mixed_live_validation.cpp" -o "$objects/client.o"
"$cxx" "$@" -B"$release/system/glue/" -L"$release/system/libroot" "$objects/client.o" "$objects/original.o" -o "$output"
shasum -a 256 "$output" "$objects/client.o" "$objects/original.o"
printf 'DEBUG70 client built ONLY;objects %s;NO installation or device access\n' "$objects"
