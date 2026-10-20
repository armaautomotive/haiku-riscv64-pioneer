#!/bin/sh
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
for entry in "$1:text.bin:0527f64a1efef48f82246280e70ff5fc5ee9c3bf9596a4be5c035b5b9f838aa8" \
 "$1:affine.bin:701bbeafc03242fe09624a8b62f476f2acf31267a24325854f6296b38b75655f" \
 "$2:text.bin:8d0a4339c676509ff45c8802f00cb2a875dd4644537df158a9087464fc234a5a" \
 "$2:affine.bin:521420fa2a52603dc4c77f95cfab3df78f36f59532de38d8a99d7e2244ef892d"; do
 path=${entry%:*}; expected=${entry##*:}; base=${path%:*}; section=${path##*:}
 actual=$(shasum -a 256 "$base/$section"); [ "${actual%% *}" = "$expected" ]
done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-queued-entry.XXXXXXXX")
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off \
 "$directory/pioneer_q38_queued_entry_test.cpp" -o "$temporary/test"
"$temporary/test" "$1" "$2"
printf 'Offline queued-entry evidence retained at %s\n' "$temporary"
