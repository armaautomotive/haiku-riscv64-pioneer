#!/bin/sh
# Compile only: older image headers are placeholders; never link or load this object.
set -eu
[ "$#" = 2 ] || exit 2
build=$(CDPATH= cd -- "$1" && pwd)
images=$(CDPATH= cd -- "$2" && pwd)
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
project=$(CDPATH= cd -- "$directory/../../.." && pwd)
sdk="$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop"
cxx="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++"
test -x "$cxx"
for header in \
 qwen35-resident-service-image/PioneerQwen35DataflowServiceImage.h \
 qwen35-dataflow-service-image/PioneerQwen35DataflowServiceImage.h \
 qwen35-shared-role-images/PioneerQwen35SharedRoleImages.h \
 qwen35-local-restore-images/PioneerQwen35LocalRestoreImages.h \
 qwen35-snapshot-service-restore-images/PioneerQwen35RoleServiceProbeImages.h; do
 test -f "$images/$header"
done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q35-fused-driver.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
set -- -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -ffreestanding \
 -fno-semantic-interposition -fno-tree-vectorize -fno-use-cxa-atexit -fno-strict-aliasing \
 -fno-delete-null-pointer-checks -mcmodel=medany -fpic -D_KERNEL_MODE \
 -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS -DTT_PIONEER_EXPERIMENTAL_Q38_PANEL \
 -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK \
 -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DTT_PIONEER_DRIVER_RELOAD \
 -DPIONEER_DRIVER_BUILD_TAG=0x2026112100010125ULL -DPIONEER_QWEN35_DATAFLOW_REWRITE \
 -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_RUN_ENABLE \
 -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY \
 -DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS \
 -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE \
 -DPIONEER_QWEN35_FUSED_RECURRENCE -DPIONEER_QWEN35_RESIDENT_SERVICE_WITNESS \
 -DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS -DPIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY \
 -DPIONEER_QWEN35_TREE_SETTLEMENT \
 -isystem "$project/headers/private/kernel" -isystem "$project/headers/private/kernel/arch/riscv64" \
 -isystem "$project/headers/private/kernel/arch/generic" -isystem "$project/headers/private" \
 -isystem "$project/headers/private/drivers" -isystem "$project/headers/private/system" \
 -isystem "$project/headers/private/system/arch/riscv64" -isystem "$project/build/config_headers" \
 -isystem "$project/headers/private/shared" -I"$build/objects/haiku/riscv64/release/system/kernel" \
 -I"$build/objects/haiku/riscv64/debug/system/kernel" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
 -I"$images/qwen35-resident-service-image" -I"$images/qwen35-dataflow-service-image" \
 -I"$images/qwen35-shared-role-images" -I"$images/qwen35-local-restore-images" \
 -I"$images/qwen35-snapshot-service-restore-images"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
source="$project/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
"$cxx" "$@" -fsyntax-only "$source"
"$cxx" "$@" -DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY \
 -DPIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY -DPIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY \
 -fstack-usage -c "$source" -o "$temporary/fused-driver.o"
echo 'PASS standard syntax and fused tree Haiku driver object; placeholder images, NO LINK OR HARDWARE'
