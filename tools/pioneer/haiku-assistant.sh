#!/bin/sh
set -eu

assistant_app=${HAIKU_ASSISTANT:-/boot/home/config/non-packaged/apps/HaikuAssistant}
LLAMA_RUNTIME=${LLAMA_RUNTIME:-/boot/home/develop/llama-c060ca974c77/build-pioneer/bin/llama-completion}
LLAMA_MODEL=${LLAMA_MODEL:-/boot/home/models/Qwen3-0.6B-Q8_0.gguf}
LIBRARY_PATH=${LLAMA_RUNTIME%/*}:/boot/system/lib
export LLAMA_RUNTIME LLAMA_MODEL LIBRARY_PATH

if [ ! -x "$assistant_app" ]; then
	echo "Haiku Assistant is not installed at $assistant_app" >&2
	exit 1
fi
exec "$assistant_app" "$@"
