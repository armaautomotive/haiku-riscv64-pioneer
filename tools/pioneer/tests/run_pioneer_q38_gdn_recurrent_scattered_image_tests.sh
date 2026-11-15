#!/bin/sh
# Offline candidate build and emitted-worker simulation only; never installs or
# selects the image on the Pioneer. No native runtime or speed claim.
set -eu
[ "$#" = 1 ] || exit 2
python3 -c 'import os,sys; sys.exit(0 if all(not k.startswith(("PIONEER_","TT_PIONEER_")) for k in os.environ) else 2)' || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
tools=$(CDPATH= cd -- "$directory/.." && pwd)
tmp=$(mktemp -d /private/tmp/pioneer-q38-gdn-scattered.XXXXXXXX)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
sh "$tools/build_qwen38_gdn_recurrent_image.sh" "$1" "$tmp/image" --scattered-cache
code_sha=$(shasum -a 256 "$tmp/image/code.bin" | awk '{print $1}')
boot_sha=$(shasum -a 256 "$tmp/image/boot.bin" | awk '{print $1}')
[ "$code_sha" = f6e37662c421b9744900afc687467e4828482e745ad158961a79be0e44fd7f49 ] || exit 1
[ "$boot_sha" = d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6 ] || exit 1
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined -DPIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT=1 \
 "$directory/pioneer_q38_gdn_recurrent_image_test.cpp" -o "$tmp/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$tmp/test" "$tmp/image"
python3 "$directory/pioneer_q38_gdn_l1_scatter_role_test.py"
printf 'OFFLINE ONLY: candidate hashes pinned; no Pioneer hardware, initialization, token commit, or tok/s measured.\n'
