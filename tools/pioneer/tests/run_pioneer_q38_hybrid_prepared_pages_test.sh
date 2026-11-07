#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-hybrid-prepared.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -Wno-deprecated-declarations \
 -fsanitize=address,undefined "$directory/pioneer_q38_hybrid_prepared_pages_test.cpp" \
 "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
"$temporary/test" "$temporary"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsyntax-only "$directory/pioneer_q38_hybrid_prepared_actual_compile.cpp"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -Wno-deprecated-declarations \
 -DPIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC=1 \
 -DPIONEER_BFP4_IMPLEMENTATION_SHA256='"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
 "$directory/../pioneer_q38_hybrid_bfp4_prepare.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/preparer"
if "$temporary/preparer" --invalid "$temporary/not-created" > /dev/null 2>&1; then exit 1; else test "$?" -eq 2; fi
test ! -e "$temporary/not-created"
printf 'Hybrid prepared-page fixture retained: %s\n' "$temporary"
