#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-fidelity-view.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_native_fidelity_view_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if printf '#include "%s/PioneerQ38NativeFidelityView.h"\n' "$directory/.." | \
  ${CXX:-c++} -std=c++17 -D"$selection" -x c++ -c - -o "$temporary/forbidden-$selection.o" 2> "$temporary/forbidden-$selection.stderr";then exit 1;fi
 grep -q 'Unqualified native fidelity VIEW is CPU diagnostic data only' "$temporary/forbidden-$selection.stderr"
done
printf 'PASS native fidelity VIEW guards:3 driver/Work/validation selections refused\n'
printf 'CPU-only native operand VIEW evidence: %s\n' "$temporary"
