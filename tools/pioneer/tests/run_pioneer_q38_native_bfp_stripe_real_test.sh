#!/bin/sh
# Existing read-only original model and existing dedicated BFP8 cache only.
set -eu
[ "$#" = 2 ] || exit 2
case "$1:$2" in /*:/*) ;; *) exit 2;; esac
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-bfp8-stripe-real.XXXXXXXX")
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off \
 "$directory/pioneer_q38_native_bfp_stripe_real_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
"$temporary/test" "$1" "$2"
