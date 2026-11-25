#!/bin/sh
# Offline retained-session setup checks; does not access the device.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q35-guarded-setup.XXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
for test in guarded_mmio_begin guarded_baseline guarded_host_audits guarded_host_page guarded_lease_end guarded_role_select guarded_mmio_end fresh_recovery_session; do
    ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
        "$directory/pioneer_qwen35_${test}_test.cpp" -o "$temporary/$test"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$test"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" "$directory/pioneer_qwen35_guarded_role_fleet_test.cpp" \
    -o "$temporary/guarded_role_fleet"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/guarded_role_fleet"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" "$directory/pioneer_qwen35_guarded_role_image_test.cpp" \
    -o "$temporary/guarded_role_image"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/guarded_role_image"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_fabric_core_sequence_test.cpp" -o "$temporary/fabric_core_sequence"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fabric_core_sequence"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fabric_core_sequence" all
