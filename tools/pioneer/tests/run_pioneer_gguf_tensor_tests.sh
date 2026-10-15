#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-gguf-tensor.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_gguf_tensor_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: bounded read-only GGUFv3 Q8 tensor selection, truncation/count/string/type/range refusal; no model execution\n'
