#!/bin/sh
# Offline redistributed+scattered emitted-worker integration only.
# No native selection, installation, card I/O, token ownership or TPS claim.
set -eu
[ "$#" = 1 ] || [ "$#" = 2 ] || exit 2
[ "$#" != 2 ] || [ "$2" = --no-gddr-writeback ] || exit 2
python3 -c 'import os,sys; sys.exit(0 if all(not k.startswith(("PIONEER_","TT_PIONEER_")) for k in os.environ) else 2)' || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
root=$(CDPATH= cd -- "$directory/.." && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-q38-gdn-redistributed-scatter.XXXXXXXX)
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
image="$temporary/image"
no_gddr_writeback=
if [ "$#" = 2 ]; then
	no_gddr_writeback=-DPIONEER_Q38_GDN_L1_NO_GDDR_WRITEBACK_EXPERIMENT=1
	sh "$root/build_qwen38_gdn_recurrent_redistributed_image.sh" "$1" "$image" --scattered-cache --no-gddr-writeback
else
	sh "$root/build_qwen38_gdn_recurrent_redistributed_image.sh" "$1" "$image" --scattered-cache
fi
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
	-fsanitize=address,undefined -DPIONEER_Q38_GDN_REDISTRIBUTED_IMAGE=1 \
	-DPIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT=1 \
	$no_gddr_writeback \
	-DPIONEER_Q38_GDN_L1_ALLOCATOR_INTEGRATION_TEST=1 \
	"$directory/pioneer_q38_gdn_recurrent_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$image"
python3 "$directory/pioneer_q38_gdn_l1_scatter_role_test.py"
printf 'OFFLINE ONLY: redistributed worker exercised actual allocated L1 maps and explicit GDDR spills; no Pioneer/runtime selection/TPS.\n'
