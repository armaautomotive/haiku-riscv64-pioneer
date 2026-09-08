#!/bin/sh
set -eu

llama_runtime=${LLAMA_RUNTIME:-/boot/home/develop/llama-c060ca974c77/build-pioneer/bin/llama-completion}
llama_model=${LLAMA_MODEL:-/boot/home/models/Qwen3-0.6B-Q8_0.gguf}

if [ ! -x "$llama_runtime" ]; then
    echo "llama-chat: runtime not found: $llama_runtime" >&2
    exit 1
fi
if [ ! -r "$llama_model" ]; then
    echo "llama-chat: model not readable: $llama_model" >&2
    exit 1
fi

exec "$llama_runtime" -m "$llama_model" -t 4 -tb 4 -c 2048 \
    -b 64 -ub 64 -n 256 --conversation --jinja --simple-io "$@"
