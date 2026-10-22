#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-predecoded-q4-page.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined "$directory/pioneer_q38_predecoded_q4_page_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
for role in ordinary freestanding mixed; do
 case "$role" in
  ordinary) flags=;;
  freestanding) flags=-DPIONEER_ATTENTION_FREESTANDING;;
  mixed) flags='-DPIONEER_Q38_PREDECODED_Q4_PAGE_STUDY -DPIONEER_ATTENTION_FREESTANDING';;
 esac
 if ${CXX:-c++} -std=c++17 -fsyntax-only -x c++ $flags "$directory/../PioneerQ38PredecodedQ4Page.h" > "$temporary/$role.log" 2>&1; then
  echo STOP_OFFLINE_PAGE_ROLE_WAS_NOT_REFUSED >&2;exit 1
 fi
 rg -q 'Separate CPU-memory page-encoding study ONLY' "$temporary/$role.log"
done
printf 'PASS3 ordinary/freestanding/mixed header-role refusals;NO producer or native selector\n'
printf 'Offline-only CPU-memory page adapter retained at %s;NO native selector or upload\n' "$temporary"
