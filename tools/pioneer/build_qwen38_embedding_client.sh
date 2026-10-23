#!/bin/sh
# Isolated SAME-FILE embedding client,SDK only,NO llama/driver/installation.
set -eu
python3 -c 'import os,sys; sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
[ "$#" = 2 ] || { [ "$#" = 3 ] && { [ "$3" = --validation ] || [ "$3" = --rms-validation ]; }; } || exit 2
role=-DPIONEER_Q38_EMBEDDING_CLIENT=1
source_name=pioneer_q38_embedding.cpp
validation=0
[ "$#" = 2 ] || validation=1
if [ "$validation" = 1 ];then role='-DPIONEER_Q38_EMBEDDING_VALIDATION_CLIENT=1 -DPIONEER_Q38_CPU_VALIDATION=1';fi
if [ "${3-}" = --rms-validation ];then
 role='-DPIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT=1 -DPIONEER_Q38_CPU_VALIDATION=1 -DPIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE=1'
 source_name=pioneer_q38_learned_rms.cpp
fi
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*;do [ ! -d "$include" ] || set -- "$@" -isystem "$include";done
for library in "$build"/build_packages/gcc_syslibs-*/lib;do set -- "$@" -L"$library";done
umask 077
"$cxx" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 $role "$@" -B"$release/system/glue/" -L"$release/system/libroot" \
 "$project/tools/pioneer/$source_name" -o "$output"
shasum -a 256 "$output"
