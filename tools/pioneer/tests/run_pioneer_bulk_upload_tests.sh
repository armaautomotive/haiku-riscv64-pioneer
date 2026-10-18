#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-bulk-upload.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_bulk_upload_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD "$directory/pioneer_bulk_upload_test.cpp" -o "$temporary/paired-test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/paired-test"
printf 'Offline bulk-upload test retained at %s\n' "$temporary"
