#!/bin/sh
# Actual main over explicit no-model/no-device test hooks;compile/run only.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-output-cli-tests.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$here/cli-stubs" "$here/pioneer_q38_gdn_output_validation_cli_test.cpp" -o "$build/cli"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/cli"
printf 'CPU-only actual-main evidence retained at %s;NO device access\n' "$build"
