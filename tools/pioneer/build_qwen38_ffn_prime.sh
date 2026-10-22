#!/bin/sh
# Isolated SAME authenticated source -> exact semantic FFN spans. Uses
# packaged SDK only;NO llama source/library build,install,driver or execution.
set -eu
[ "$#" = 2 ] || [ "$#" = 3 ] || exit 2
role_flags=
if [ "$#" = 3 ];then
 case "$3" in
  --model-init) role_flags=-DPIONEER_Q38_FFN_MODEL_INIT_CLIENT=1;;
  --native-graph) role_flags='-DPIONEER_Q38_FFN_MODEL_INIT_CLIENT=1 -DPIONEER_Q38_FFN_NATIVE_GRAPH_CLIENT=1';;
  --native-graph-host-profile) role_flags='-DPIONEER_Q38_FFN_MODEL_INIT_CLIENT=1 -DPIONEER_Q38_FFN_NATIVE_GRAPH_CLIENT=1 -DPIONEER_Q38_FFN_HOST_PROFILE_CLIENT=1';;
  *) exit 2;;
 esac
fi
# No inherited experiment can silently change this source/transport role.
for key in $(env | sed -n 's/^\(PIONEER_[A-Z0-9_]*\)=.*/\1/p');do
 value=$(printenv "$key" || :);[ "$value" = 0 ] || exit 2
done
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*;do [ ! -d "$include" ] || set -- "$@" -isystem "$include";done
for library in "$build"/build_packages/gcc_syslibs-*/lib;do set -- "$@" -L"$library";done
umask 077
"$cxx" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_WORK_INFERENCE -DPIONEER_Q38_SPAN_PRELOAD -DPIONEER_Q38_SPAN_FAST -DPIONEER_Q38_FFN_PRIME=1 \
 $role_flags "$@" -B"$release/system/glue/" -L"$release/system/libroot" \
 "$project/tools/pioneer/pioneer_q38_ffn_prime.cpp" -o "$output"
shasum -a 256 "$output"
