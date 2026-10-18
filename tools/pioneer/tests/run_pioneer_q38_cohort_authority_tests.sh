#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-cohort-authority.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/mocks" "$directory/pioneer_q38_cohort_authority_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -I"$directory/mocks" "$directory/pioneer_q38_cohort_authority_test.cpp" -o "$temporary/sealed"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/sealed"
printf 'Offline cohort authority evidence retained at %s\n' "$temporary"
