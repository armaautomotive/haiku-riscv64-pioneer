#!/bin/sh
# Actual controller/emitted image,simulated device fabric only.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
actual=$(shasum -a 256 "$1/text.bin"); [ "${actual%% *}" = 39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b ]
actual=$(shasum -a 256 "$2/code.bin"); [ "${actual%% *}" = 518fbdefed49df2afd89837e6fd2edb08d0bf0c33df5a55e47ccb3ccbc21951b ]
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-rgdn-root-controller.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE -DPIONEER_Q38_LOCAL_DELTA_GGML \
 "$directory/pioneer_q38_rgdn_root_controller_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2"
printf 'Offline root GDN controller evidence retained at %s;NO native adapter/module/selection\n' "$temporary"
