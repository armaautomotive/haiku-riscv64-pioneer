#!/bin/bash
# Build the pinned SOPHGO Pioneer firmware on native RISC-V Linux.
# Output only: this script does not deploy firmware or change boot variables.
set -eo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
	echo "Usage: $0 /absolute/path/to/firmware-build/source [RELEASE|DEBUG]" >&2
	exit 2
fi
build_target=${2:-RELEASE}
case "$build_target" in
	RELEASE) build_defines=() ;;
	DEBUG) build_defines=(-D DEBUG_ON_SERIAL_PORT=TRUE) ;;
	*) echo 'Build target must be RELEASE or DEBUG' >&2; exit 2 ;;
esac
cd "$1"
[[ $(uname -m) == riscv64 ]] || { echo 'Native RISC-V Linux required' >&2; exit 1; }
[[ $(git rev-parse HEAD) == cd2d36fde3e0247a91fdb192f17882a4dc317159 ]] || {
	echo 'Unexpected sophgo-edk2 revision' >&2; exit 1;
}
for spec in \
	edk2:e1d3e494c275068932730e3e6bf44139d3675dae \
	edk2-platforms:02e6b7618e4090cad54b2724f6a535990658a539 \
	edk2-non-osi:3c7fbde3a7e33710a8d62267452a5acfde3c315d; do
	[[ $(git -C "${spec%%:*}" rev-parse HEAD) == "${spec#*:}" ]] || exit 1
done
export WORKSPACE="$PWD"
export PACKAGES_PATH="$PWD/edk2:$PWD/edk2-platforms:$PWD/edk2-non-osi"
export EDK_TOOLS_PATH="$PWD/edk2/BaseTools"
export TMPDIR="$PWD/../tmp"
export CCACHE_DIR="$PWD/../ccache"
export PATH="/usr/bin:/bin:$PATH"
export GCC5_RISCV64_PREFIX=/usr/bin/
mkdir -p "$TMPDIR" "$CCACHE_DIR"
set --
source edk2/edksetup.sh
if [[ -f "$PWD/../host-deps/include/uuid/uuid.h" ]]; then
	CPATH="$PWD/../host-deps/include" LIBRARY_PATH="$PWD/../host-deps/lib" \
		make -C edk2/BaseTools -j8
else
	make -C edk2/BaseTools -j8
fi
# Explicitly expose the DT to Haiku.
build -a RISCV64 -t GCC5 -b "$build_target" -n 8 "${build_defines[@]}" \
	-D ACPI_ENABLE=FALSE \
	-p Platform/Sophgo/SG2042Pkg/MilkV-Pioneer/MilkV-Pioneer.dsc
