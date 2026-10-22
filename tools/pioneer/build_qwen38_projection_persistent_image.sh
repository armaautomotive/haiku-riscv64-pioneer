#!/bin/sh
# OFFLINE complete new peer image;no native registry/coordinator access.
set -eu
[ "$#" = 2 ] || exit 2
for key in $(env | sed -n 's/^\(PIONEER_[A-Z0-9_]*\)=.*/\1/p');do
 [ "$(printenv "$key" || :)" = 0 ] || exit 2
done
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec env PIONEER_Q38_FFN_PERSISTENT=1 sh "$directory/build_qwen38_ffn_persistent_image.sh" "$1" "$2" --projection-input
