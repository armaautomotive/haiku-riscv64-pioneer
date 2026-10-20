#!/bin/sh
# OFFLINE ONLY: no hardware, firmware messages or native capability.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-arc-queue.XXXXXXXX")
for test in arc_queue normal_clock clock_lease clock_cycle; do
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  "$directory/pioneer_q38_${test}_test.cpp" -o "$temporary/$test"
 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$test"
done
printf 'Offline strict ARC queue evidence retained at %s\n' "$temporary"
