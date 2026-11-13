#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-gdn-observer.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_SYNTHETIC_GDN_OBSERVER_TEST=1 "$directory/pioneer_q38_synthetic_gdn_observer.cpp" \
 "$directory/pioneer_q38_synthetic_gdn_observer_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
for forbidden in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE; do
 if ${CXX:-c++} -std=c++17 -DPIONEER_Q38_SYNTHETIC_GDN_OBSERVER_TEST=1 -D"$forbidden" \
  -fsyntax-only "$directory/pioneer_q38_synthetic_gdn_observer.cpp" >"$temporary/role.out" 2>&1; then
  printf 'Observer incorrectly accepted forbidden role: %s\n' "$forbidden" >&2
  exit 1
 fi
done
printf 'PASS synthetic GDN observer rejects kernel and Work roles\n'
