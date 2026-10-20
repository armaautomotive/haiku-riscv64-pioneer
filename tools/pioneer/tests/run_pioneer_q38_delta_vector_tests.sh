#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-delta-vector.XXXXXXXX")
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off \
 "$directory/pioneer_q38_delta_vector_test.cpp" -o "$temporary/test"
"$temporary/test"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
 "$directory/pioneer_q38_delta_state_layout_test.cpp" -o "$temporary/layout"
"$temporary/layout"
printf 'Offline vector GDN evidence retained at %s\n' "$temporary"
