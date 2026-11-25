#!/bin/sh
# Offline activation-read pipeline and actual model-geometry schedule checks.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q35-activation-pipeline.XXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -pthread -fsanitize=address,undefined'
${CXX:-c++} $flags "$directory/pioneer_qwen35_activation_read_pipeline_test.cpp" -o "$temporary/pipeline"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pipeline"
${CXX:-c++} $flags "$directory/pioneer_qwen35_activation_share_tree_test.cpp" -o "$temporary/sharing-tree"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/sharing-tree"
sharing_flags='-DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS'
for test in pioneer_qwen35_activation_share_startup_test pioneer_qwen35_activation_share_reader_test \
    pioneer_qwen35_activation_share_fleet_test \
    pioneer_qwen35_dataflow_role_memory_plan_test pioneer_qwen35_dataflow_role_code_owner_test; do
    ${CXX:-c++} $flags $sharing_flags "$directory/$test.cpp" -o "$temporary/$test"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$test"
done
${CXX:-c++} $flags $sharing_flags -DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS \
    "$directory/pioneer_qwen35_qualification_sharing_test.cpp" -o "$temporary/qualification-sharing"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/qualification-sharing"
for mode in ordinary paired fused; do
    case "$mode" in
        ordinary) extra= ;;
        paired) extra=-DPIONEER_QWEN35_PAIRED_RECURRENCE ;;
        fused) extra='-DPIONEER_QWEN35_PAIRED_RECURRENCE -DPIONEER_QWEN35_FUSED_RECURRENCE' ;;
    esac
    ${CXX:-c++} $flags $extra "$directory/pioneer_qwen35_activation_read_graph_test.cpp" -o "$temporary/$mode"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$mode"
    for test in pioneer_qwen35_activation_sharing_inventory pioneer_qwen35_activation_share_plan_test; do
        ${CXX:-c++} $flags $extra "$directory/$test.cpp" -o "$temporary/$test-$mode"
        ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$test-$mode"
    done
    ${CXX:-c++} $flags $extra -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
        -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS \
        "$directory/pioneer_qwen35_activation_share_plan_test.cpp" -o "$temporary/sharing-record-$mode"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/sharing-record-$mode"
done
python3 "$directory/pioneer_qwen35_activation_sharing_guard_test.py"
${CXX:-c++} $flags -DPIONEER_QWEN35_PAIRED_RECURRENCE -DPIONEER_QWEN35_FUSED_RECURRENCE \
    "$directory/pioneer_qwen35_projection_triple_test.cpp" -o "$temporary/triple"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/triple"
${CXX:-c++} $flags "$directory/pioneer_qwen35_fused_recurrence_metadata_test.cpp" -o "$temporary/metadata"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/metadata"
${CXX:-c++} $flags -DPIONEER_QWEN35_PAIRED_RECURRENCE -DPIONEER_QWEN35_FUSED_RECURRENCE \
    "$directory/pioneer_qwen35_dynamic_token_test.cpp" -o "$temporary/fused-dynamic"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fused-dynamic"
${CXX:-c++} $flags -DPIONEER_QWEN35_PAIRED_RECURRENCE -DPIONEER_QWEN35_FUSED_RECURRENCE \
    -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS \
    -DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS \
    "$directory/pioneer_qwen35_dynamic_token_test.cpp" -o "$temporary/sharing-dynamic"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/sharing-dynamic"
