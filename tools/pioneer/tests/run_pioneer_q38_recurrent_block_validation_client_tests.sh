#!/bin/sh
set -eu
pioneer_test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
pioneer_test_tmp=$(mktemp -d /private/tmp/pioneer-recurrent-validation-client.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g0 -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$pioneer_test_dir/pioneer_q38_recurrent_block_validation_session_test.cpp" -o "$pioneer_test_tmp/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$pioneer_test_tmp/test"
printf 'Client TEST binary retained: %s\n' "$pioneer_test_tmp/test"
