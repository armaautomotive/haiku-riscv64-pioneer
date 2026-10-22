#!/bin/sh
# Build only;separate exact-tag35 client. REAL source->upload->workspace
# zero-CLOSE on ONE FD. No llama source/libs/Assistant/model computation.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$directory/build_qwen38_ffn_prime.sh" "$1" "$2" --model-init
