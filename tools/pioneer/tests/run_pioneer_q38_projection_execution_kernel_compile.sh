#!/bin/sh
# Template kernel compilation ONLY,not actual native context/scope/card.
set -eu
[ "$#" = 1 ] || { [ "$#" = 2 ] && [ "$2" = --result-stashes ]; } || exit 2
stash_flags=
[ "${2-}" != --result-stashes ] || stash_flags=-DPIONEER_Q38_PROJECTION_RESULT_STASH=1
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-projection-execution-compile.XXXXXXXX")
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
"$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE $stash_flags \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_projection_execution_kernel_compile.cpp" -o "$temporary/kernel.o"
awk -F '\t' 'NF==3 {if($2+0>2048)bad=1;if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_PROJECTION_EXECUTION_TEMPLATE_KERNEL_FRAME_BYTES="maximum";NOT_NATIVE_ADAPTER_OR_CARD"}' "$temporary/kernel.su"
printf 'Projection execution template kernel object:%s;NO device access\n' "$temporary"
