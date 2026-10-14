#!/bin/sh
# Native build only. Does not install, arm, load or reboot anything.
set -eu
[ "$#" -eq 1 ] || { printf 'Usage: %s OUTPUT_DIRECTORY\n' "$0" >&2; exit 2; }
[ "$(uname -s)" = Linux ] && [ "$(uname -m)" = riscv64 ] \
	|| { printf '%s\n' 'Build this userspace diagnostic on native RISC-V Linux' >&2; exit 1; }
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p "$1"
${CC:-cc} -std=c11 -O2 -march=rv64gc -mabi=lp64d -Wall -Wextra -Werror \
	"$script_dir/kexec_trace/main.c" "$script_dir/kexec_trace/trace.c" \
	-o "$1/PioneerKexecTrace"
file "$1/PioneerKexecTrace"
sha256sum "$1/PioneerKexecTrace"
