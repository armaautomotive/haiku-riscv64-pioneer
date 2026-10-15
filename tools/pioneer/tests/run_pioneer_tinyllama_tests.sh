#!/bin/sh
set -eu
[ "$#" = 1 ] || { echo 'Usage: run_pioneer_tinyllama_tests.sh PINNED_BOOTSTRAP_DIRECTORY' >&2; exit 2; }
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
inputs=$(CDPATH= cd -- "$1" && pwd)
verify() { [ "$(shasum -a 256 "$inputs/$1" | cut -d ' ' -f1)" = "$2" ]; }
verify run.c 9c4f2d5c6ae01b71726d1cc37530d71e60bff0ec7cc012565f16a43c1ca658bd
verify LICENSE e87b912002f04cdfa837eaf7f70548ab79b9cb62a285495edf575f3da875aec4
verify stories260K.bin b0a507e7ad0f626624f17112325e66691f9076d622e1d3274d103d00299f2696
verify tok512.bin 037cb335abb25d1fa9e8ecae30ed2a3a8ace9302862ebcdc05d51a6bbb10c312
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-tinyllama.XXXXXXXX")
trap 'rm -f "$temporary/reference.o" "$temporary/test" "$temporary/assistant-test"; rmdir "$temporary"' EXIT HUP INT TERM
${CC:-cc} -std=c11 -O1 -ffp-contract=off -Wno-unknown-pragmas -fsanitize=address,undefined \
 -DPIONEER_TINY_REFERENCE_SOURCE="\"$inputs/run.c\"" -c "$directory/pioneer_tiny_reference_bridge.c" -o "$temporary/reference.o"
${CXX:-c++} -std=c++17 -O1 -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_tinyllama_test.cpp" "$temporary/reference.o" -o "$temporary/test"
"$temporary/test" "$inputs/stories260K.bin" "$inputs/tok512.bin"
${CXX:-c++} -std=c++17 -O1 -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_assistant_p100a_test.cpp" "$temporary/reference.o" -o "$temporary/assistant-test"
"$temporary/assistant-test" "$inputs/stories260K.bin" "$inputs/tok512.bin"
