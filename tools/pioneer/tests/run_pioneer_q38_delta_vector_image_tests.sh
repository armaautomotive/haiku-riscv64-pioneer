#!/bin/sh
set -eu
[ "$#" = 1 ] || [ "$#" = 2 ] || exit 2
flags=
code_hash=05bef31999bb3a8fa0280744af52645fc92e31a441207fe6e842fe8b53464a52
boot_hash=d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6
if [ "$#" = 2 ]; then
 case "$2" in
  --probe) flags=-DPIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
   code_hash=1c0766bedd173f0fcf383629dec7a333a17889ad25e8833b9a57002b6f6ae771;;
  --ggml) flags='-DPIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE -DPIONEER_Q38_DELTA_VECTOR_POST_SCALE'
   code_hash=0c61dc435a3c2e00c8ae8a11319a307c86707b410100f7fbc85d15f8587bfcbe;;
  *) exit 2;;
 esac
 boot_hash=e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b
fi
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-delta-vector-image.XXXXXXXX")
actual=$(shasum -a 256 "$1/code.bin"); [ "${actual%% *}" = "$code_hash" ]
boot=$(shasum -a 256 "$1/boot.bin"); [ "${boot%% *}" = "$boot_hash" ]
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off \
 $flags "$directory/pioneer_q38_delta_vector_image_test.cpp" -o "$temporary/test"
"$temporary/test" "$1"
printf 'Offline emitted vector GDN evidence retained at %s\n' "$temporary"
