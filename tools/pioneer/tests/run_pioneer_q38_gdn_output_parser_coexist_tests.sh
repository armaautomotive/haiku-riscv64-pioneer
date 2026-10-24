#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-gdn-output-parser-coexist.XXXXXXXX)
for mode in 0 1;do
 flag=
 if [ "$mode" = 1 ];then flag=-DPIONEER_Q38_PROJECTION_RESULT_STASH=1;fi
 "${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $flag \
  "$here/pioneer_q38_gdn_output_parser_coexist_test.cpp" -o "$build/test-$mode"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test-$mode"
done
