#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-native-q8-namespace.XXXXXXXX")
for mode in ordinary producer qualifier reference group32; do
 flags=
 case "$mode" in
  producer) flags='-DPIONEER_Q38_NATIVE_Q8_LAYOUT -DPIONEER_Q38_EXPECT_SM7_NAMESPACE';;
  qualifier) flags='-DPIONEER_Q38_NATIVE_Q8_QUALIFICATION -DPIONEER_Q38_EXPECT_SM7_NAMESPACE';;
  reference) flags='-DPIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT -DPIONEER_Q38_EXPECT_SM7_NAMESPACE';;
  group32) flags=-DPIONEER_Q38_GROUP32_QUALIFICATION;;
 esac
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $flags \
  "$directory/pioneer_q38_native_q8_namespace_test.cpp" -o "$temporary/$mode"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$mode"
done
printf 'Offline namespace evidence retained at %s\n' "$temporary"
