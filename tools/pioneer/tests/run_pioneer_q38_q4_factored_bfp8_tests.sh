#!/bin/sh
# Local component/actual-ROM checks ONLY. Never opens a model or device.
set -eu
[ "$#" = 2 ] || exit 2
sdk=$1
output=$2
case "$sdk:$output" in /*:/*) ;; *) exit 2;; esac
test -d "$sdk" && test ! -e "$output" && test ! -L "$output" || exit 2
mkdir -m 700 "$output"
tests=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
compiler=${CXX:-c++}
"$compiler" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$tests/pioneer_q38_q4_factored_bfp8_test.cpp" -o "$output/component"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$output/component"
python3 "$tests/../build_q38_q4_factored_bfp8.py" "$sdk" "$output/inline" --inline
python3 "$tests/../build_q38_q4_factored_bfp8.py" "$sdk" "$output/mmio"
"$compiler" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$tests/pioneer_q38_q4_factored_bfp8_code_test.cpp" -o "$output/code-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 \
 "$output/code-test" "$output/inline/text.bin" "$output/inline/symbols.txt" inline
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 \
 "$output/code-test" "$output/mmio/text.bin" "$output/mmio/symbols.txt" mmio
python3 "$tests/pioneer_q38_q4_factored_bfp8_roles_test.py"
