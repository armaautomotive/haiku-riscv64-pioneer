#!/bin/sh
# OFFLINE cross-link only,using frozen clean ABI libraries. No dirty llama
# implementation build,deployment,Assistant update or accelerator access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
PIONEER_Q38_DISPATCH_INFERENCE=1 exec sh "$directory/build_qwen38_inference.sh" "$@"
