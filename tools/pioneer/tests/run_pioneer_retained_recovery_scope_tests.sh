#!/bin/sh
# Isolated integration of actual driver pin and controller lease methods on fakes.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-retained-scope.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT
source=$directory/../../../src/system/kernel/device_manager/device_manager.cpp
for pair in INIT_LIFETIME:init_lifetime RETAINED_DRIVER_PIN:retained_driver_pin RETAINED_PIN_SERVICE:retained_pin_service; do
 section=${pair%%:*}; name=${pair#*:}
 awk -v begin="PIONEER_DM_${section}_BEGIN" -v end="PIONEER_DM_${section}_END" \
  'index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}' \
  "$source" > "$temporary/pioneer_dm_$name.inc"
done
source=$directory/../../../src/add-ons/kernel/busses/pci/ecam/ECAMPCIController.cpp
awk '/PIONEER_SG2042_RECOVERY_LEASE_BEGIN/ {copying=1;next} /PIONEER_SG2042_RECOVERY_LEASE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/sg2042_recovery_controller.inc"
source=$directory/../../../src/add-ons/kernel/busses/pci/ecam/kernel_interface.cpp
awk '/PIONEER_SG2042_RETAINED_LEASE_SERVICE_BEGIN/ {copying=1;next} /PIONEER_SG2042_RETAINED_LEASE_SERVICE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/sg2042_retained_lease_service.inc"
header=$directory/../../../src/add-ons/kernel/drivers/misc/PioneerRetainedRecoveryScope.h
printf 'using status_t=int; enum { B_OK=0, B_ERROR=-1, B_DEV_NOT_READY=-2 };\n#include "%s"\n' "$header" > "$temporary/guard.cpp"
for flags in '' '-DPIONEER_QWEN35_RETAINED_SCOPE_WITNESS' '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY' \
 '-DPIONEER_QWEN35_RETAINED_SCOPE_WITNESS -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_QUALIFY'; do
 # Deliberate word splitting: each flag is a separate compiler argument.
 if ${CXX:-c++} -std=c++17 $flags -fsyntax-only "$temporary/guard.cpp" > /dev/null 2>&1; then
  echo 'FAIL retained scope build guard' >&2
  exit 1
 fi
done
echo 'PASS retained scope rejects unguarded and qualification builds'
for retained in PIONEER_SG2042_RETAINED_LEASE_WITNESS PIONEER_SG2042_RETAINED_LEASE_SERVICE; do
 ${CXX:-c++} -std=c++20 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -pthread \
  -DPIONEER_SG2042_RECOVERY_WITNESS -D"$retained" \
  -DPIONEER_QWEN35_RETAINED_SCOPE_WITNESS -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
  -I"$temporary" "$directory/pioneer_retained_recovery_scope_test.cpp" -o "$temporary/test"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
done
