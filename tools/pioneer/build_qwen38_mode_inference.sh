#!/bin/sh
# Explicit separate executables:WORK never contains the CPU reference.
set -eu
[ "$#" = 4 ] || exit 2
mode=$1; shift
case "$mode" in work|cpu-validated) ;; *) exit 2;; esac
PIONEER_Q38_EXECUTION_MODE=$mode PIONEER_Q38_DISPATCH_INFERENCE=1 PIONEER_Q38_VECTOR_INFERENCE=1 PIONEER_Q38_CPU_POOL_PROBE=0 \
 exec sh "$(dirname "$0")/build_qwen38_inference.sh" "$@"
