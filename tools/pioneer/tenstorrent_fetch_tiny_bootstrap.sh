#!/bin/sh
# Fetch fixed public data/source only. Never executes downloaded scripts,
# pickle checkpoints or a model; does not connect to the Pioneer.
set -eu
[ "$#" = 1 ] || { echo 'Usage: tenstorrent_fetch_tiny_bootstrap.sh FRESH_ABSOLUTE_DIRECTORY' >&2; exit 2; }
output=$1
case "$output" in /*) ;; *) echo 'Output must be absolute' >&2; exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || { echo 'Output already exists' >&2; exit 1; }
umask 077
mkdir -m 700 "$output"
fetch() {
	curl --fail --silent --show-error --location --max-time 60 --output "$output/$1" "$2"
	[ "$(shasum -a 256 "$output/$1" | cut -d ' ' -f1)" = "$3" ] || {
		echo "STOP: checksum mismatch for $1; do not use these artifacts" >&2; exit 1;
	}
}
source_base=https://raw.githubusercontent.com/karpathy/llama2.c/350e04fe35433e6d2941dce5a1f53308f87058eb
model_base=https://huggingface.co/karpathy/tinyllamas/resolve/0bd21da7698eaf29a0d7de3992de8a46ef624add/stories260K
fetch LICENSE "$source_base/LICENSE" e87b912002f04cdfa837eaf7f70548ab79b9cb62a285495edf575f3da875aec4
fetch run.c "$source_base/run.c" 9c4f2d5c6ae01b71726d1cc37530d71e60bff0ec7cc012565f16a43c1ca658bd
fetch stories260K.bin "$model_base/stories260K.bin" b0a507e7ad0f626624f17112325e66691f9076d622e1d3274d103d00299f2696
fetch tok512.bin "$model_base/tok512.bin" 037cb335abb25d1fa9e8ecae30ed2a3a8ace9302862ebcdc05d51a6bbb10c312
printf 'Verified source/license/model/tokenizer in %s. No program/model executed.\n' "$output"
