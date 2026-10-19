#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-dispatch-profile.XXXXXXXX")
for mode in disabled enabled; do
 extra=
 [ "$mode" = disabled ] || extra=-DTT_PIONEER_Q38_DISPATCH_PROFILE
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $extra \
  "$directory/pioneer_q38_dispatch_profile_test.cpp" -o "$temporary/test-$mode"
 "$temporary/test-$mode"
done
printf 'CPU-clock-only profile evidence retained at %s\n' "$temporary"
