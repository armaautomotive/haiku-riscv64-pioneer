#!/bin/sh
[ "${PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE+x}" = "" ] || exit 2
[ "${PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_DENSE_PREPARE_IMAGE+x}${PIONEER_Q38_DENSE_ATTENTION_IMAGE+x}${PIONEER_Q38_DENSE_PROJECTION_IMAGE+x}${PIONEER_Q38_DENSE_PARTITION_IMAGE+x}${PIONEER_Q38_MIXED_WORKER_IMAGE+x}${PIONEER_Q38_MIXED_CAPSULE_IMAGE+x}${PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE+x}" = "" ] || exit 2
[ "${PIONEER_Q38_RESIDENT_CONTINUE_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_ADVANCE_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT+x}" != x ] || exit 2
# Separate CPU DEBUG69 client and independently compiled original observer.
# SDK only;NO build-mirror writes,installation,accelerator FD or Work grant.
set -eu
python3 -c 'import os,sys; sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
[ "$#" = 2 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
objects=$(mktemp -d /private/tmp/pioneer-q38-output-debug69-client.XXXXXXXX)
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*;do [ ! -d "$include" ] || set -- "$@" -isystem "$include";done
for library in "$build"/build_packages/gcc_syslibs-*/lib;do set -- "$@" -L"$library";done
umask 077
# NO native/device-image/Work or client flag on the observer translation unit.
"$cxx" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 "$@" -c "$project/tools/pioneer/pioneer_q38_original_source.cpp" -o "$objects/original.o"
"$cxx" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 -DPIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT=1 -DPIONEER_Q38_CPU_VALIDATION=1 \
 "$@" -c "$project/tools/pioneer/pioneer_q38_resident_layer_validation.cpp" -o "$objects/client.o"
"$cxx" "$@" -B"$release/system/glue/" -L"$release/system/libroot" \
 "$objects/client.o" "$objects/original.o" -o "$output"
shasum -a 256 "$output" "$objects/client.o" "$objects/original.o"
printf 'DEBUG69 client built only;objects retained at %s;NO installation or device access\n' "$objects"
