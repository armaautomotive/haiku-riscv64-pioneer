#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-qrj-client-tests.XXXXXXXX)
for name in reference session;do
 "${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  "$here/pioneer_q38_gdn_recurrent_validation_${name}_test.cpp" -o "$build/$name"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/$name"
done
