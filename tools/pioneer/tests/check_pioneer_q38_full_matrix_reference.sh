#!/bin/sh
# Separate offline public numerical observer. Native program has NO CPU dot.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
actual=$(shasum -a 256 "$1"); actual=${actual%% *}
[ "$actual" = c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747 ] || exit 1
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-full-reference.XXXXXXXX")
${CXX:-c++} -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_full_matrix_reference.cpp" -o "$temporary/reference"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/reference" "$1" "$2"
for fault in first middle last; do
 if ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/reference" "$1" "$2" "--test-corrupt-$fault"; then
  echo 'STOP:independent observer accepted injected incorrect output' >&2; exit 1
 fi
done
printf 'Observer rejects injected first/middle/last row errors;exported file unchanged\n'
printf 'Independent offline observer retained at %s;NO accelerator access\n' "$temporary"
