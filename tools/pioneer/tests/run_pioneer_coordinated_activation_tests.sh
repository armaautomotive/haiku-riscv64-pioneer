#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-coordinated-activation.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined \
 "$directory/pioneer_coordinated_activation_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Coordinated activation test retained at %s\n' "$temporary"
