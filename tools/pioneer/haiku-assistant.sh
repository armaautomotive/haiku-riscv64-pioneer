#!/bin/sh
set -eu

assistant_app=${HAIKU_ASSISTANT:-/boot/home/config/non-packaged/apps/HaikuAssistant}
LLAMA_RUNTIME=${LLAMA_RUNTIME:-/boot/home/develop/llama-c060ca974c77/build-pioneer/bin/llama-completion}
LIBRARY_PATH=${LLAMA_RUNTIME%/*}:/boot/system/lib
export LLAMA_RUNTIME LIBRARY_PATH
# Do not set LLAMA_MODEL by default: the service uses the saved model choice.
# An explicitly exported LLAMA_MODEL remains a startup-only override.

if [ ! -x "$assistant_app" ]; then
	echo "Haiku Assistant is not installed at $assistant_app" >&2
	exit 1
fi
exec "$assistant_app" "$@"
