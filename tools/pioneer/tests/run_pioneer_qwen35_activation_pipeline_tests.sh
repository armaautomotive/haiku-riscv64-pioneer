#!/bin/sh
# Offline activation-read pipeline and actual model-geometry schedule checks.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q35-activation-pipeline.XXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -pthread -fsanitize=address,undefined'
${CXX:-c++} $flags "$directory/pioneer_qwen35_activation_read_pipeline_test.cpp" -o "$temporary/pipeline"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pipeline"
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
done
${CXX:-c++} $flags -DPIONEER_QWEN35_PAIRED_RECURRENCE -DPIONEER_QWEN35_FUSED_RECURRENCE \
    "$directory/pioneer_qwen35_projection_triple_test.cpp" -o "$temporary/triple"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/triple"
${CXX:-c++} $flags "$directory/pioneer_qwen35_fused_recurrence_metadata_test.cpp" -o "$temporary/metadata"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/metadata"
${CXX:-c++} $flags -DPIONEER_QWEN35_PAIRED_RECURRENCE -DPIONEER_QWEN35_FUSED_RECURRENCE \
    "$directory/pioneer_qwen35_dynamic_token_test.cpp" -o "$temporary/fused-dynamic"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fused-dynamic"
