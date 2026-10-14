#!/bin/sh
# Private build artifacts only; never installs or loads the module.
set -eu
[ "$#" -eq 1 ] || { printf 'Usage: %s OUTPUT_DIRECTORY\n' "$0" >&2; exit 2; }
[ "$(uname -s)" = Linux ] && [ "$(uname -m)" = riscv64 ] \
	&& [ "$(uname -r)" = 6.1.31 ] || { printf 'Recorded native Linux 6.1.31 required\n' >&2; exit 1; }
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir=$script_dir/kexec_kernel_trace
mkdir -p "$1"
output_dir=$(CDPATH= cd -- "$1" && pwd)
[ "$output_dir" != "$source_dir" ] || { printf 'Use a separate build directory\n' >&2; exit 1; }
cp "$source_dir/Makefile" "$source_dir/polled.h" \
	"$source_dir/pioneer_kexec_kernel_trace.c" "$output_dir/"
make -C /usr/src/kernels/6.1.31 M="$output_dir" -j4 modules
modinfo "$output_dir/pioneer_kexec_kernel_trace.ko"
sha256sum "$output_dir/pioneer_kexec_kernel_trace.ko"
