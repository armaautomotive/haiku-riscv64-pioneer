#!/bin/sh
# CPU-only byte endpoint component;no device IO or native owner.
set -eu
[ "$#" -eq 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-q38-native-mutable.XXXXXXXX)
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_token_native_mutable_bytes_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
