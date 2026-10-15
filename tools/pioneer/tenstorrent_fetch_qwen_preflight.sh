#!/bin/sh
# Read-only bounded ranges from an immutable public GGUF, never a full model.
set -eu
[ "$#" = 1 ] || { echo 'Usage: tenstorrent_fetch_qwen_preflight.sh FRESH_ABSOLUTE_DIRECTORY' >&2; exit 2; }
output=$1
case "$output" in /*) ;; *) echo 'Output must be absolute' >&2; exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || { echo 'Output already exists' >&2; exit 1; }
umask 077
mkdir -m 700 "$output"
url=https://huggingface.co/ggml-org/Qwen3.8-27B-GGUF/resolve/71bc7b627595dc8a91039addd9c791ae548d6747/Qwen3.8-27B-Q4_K_M.gguf
# Full source: 18,973,870,528 bytes, LFS SHA-256
# c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747.
# Range hashes below are locally pinned; they do NOT verify the full model.
fetch() {
	name=$1 first=$2 last=$3 expected=$4
	count=$((last-first+1))
	curl --fail --silent --show-error --location --max-time 60 \
	 --range "$first-$last" --max-filesize "$count" \
	 --dump-header "$output/$name.headers" --output "$output/$name" \
	 "$url?download=true&range=$first-$last"
	[ "$(wc -c < "$output/$name" | tr -d ' ')" = "$count" ]
	[ "$(shasum -a 256 "$output/$name" | cut -d ' ' -f1)" = "$expected" ] || {
		echo "STOP: range checksum mismatch: $name" >&2; exit 1;
	}
	awk -v expected="bytes $first-$last/18973870528" '
	 {sub(/\r$/, ""); if(tolower($1)=="content-range:") {sub(/^[^:]*: */, ""); actual=$0}}
	 END {if(actual!=expected) exit 1}
	' "$output/$name.headers"
}
fetch qwen38-q4km-header.raw 0 16777215 f0887b115aad2adb31420eb735b9c33ab848e00cb4a0ffffa6692674434a7bd0
fetch qwen38-q4km-ffn-gate-rows0-15.raw 1908404672 1908450751 bd8891785542ab68f1968df38dd007135b1f56d5bca21d2a45ad5c8d3d113d16
fetch qwen38-q8-attn-gate-rows0-15.raw 1769120192 1769207231 95c167156d8318c8ccf13c2bee6b2eedd97b6b42ea5e61ba31cd481a1520669e
fetch qwen38-q6-attn-out-rows0-15.raw 2595768384 2595849023 c8ac08c1ee982a25ee531660c2523510fdc07ab5b1ee46957df411c89900f400
printf 'Verified bounded Qwen header/three tensor samples in %s; NOT a runnable model. No code or device executed.\n' "$output"
