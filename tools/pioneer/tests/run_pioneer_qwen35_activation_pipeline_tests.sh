#!/bin/sh
# Offline activation-read pipeline and actual model-geometry schedule checks.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q35-activation-pipeline.XXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -pthread -fsanitize=address,undefined'
${CXX:-c++} $flags "$directory/pioneer_qwen35_activation_read_pipeline_test.cpp" -o "$temporary/pipeline"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pipeline"
for mode in ordinary paired; do
    extra=
    [ "$mode" = ordinary ] || extra=-DPIONEER_QWEN35_PAIRED_RECURRENCE
    ${CXX:-c++} $flags $extra "$directory/pioneer_qwen35_activation_read_graph_test.cpp" -o "$temporary/$mode"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$mode"
done
