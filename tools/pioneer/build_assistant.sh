#!/bin/sh
set -eu

# Standalone cross-build; does not rebuild or deploy the operating system.
assistant_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
assistant_build=${HAIKU_BUILD_OUTPUT:-/Volumes/HaikuBuildLocal/generated.riscv64}
assistant_output=${1:-/private/tmp/HaikuAssistant}
assistant_sdk=$assistant_build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
assistant_release=$assistant_build/objects/haiku/riscv64/release
assistant_cxx=$assistant_build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++

set -- -I"$assistant_sdk/headers" -idirafter "$assistant_sdk/headers/posix"
for assistant_include in "$assistant_sdk/headers/os" "$assistant_sdk"/headers/os/*; do
	if [ -d "$assistant_include" ]; then
		set -- "$@" -isystem "$assistant_include"
	fi
done
for assistant_lib in "$assistant_build"/build_packages/gcc_syslibs-*/lib; do
	set -- "$@" -L"$assistant_lib"
done

"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
	"$@" -B"$assistant_release/system/glue/" \
	-L"$assistant_release/kits" -L"$assistant_release/system/libroot" \
	"$assistant_root/src/apps/assistant/Assistant.cpp" -lbe -o "$assistant_output"
assistant_host_tools=$assistant_build/objects/darwin/arm64/release/tools
assistant_resources=$(mktemp -d /private/tmp/haiku-assistant-resources.XXXXXX)
"$assistant_host_tools/rc/rc" -o "$assistant_resources/Assistant.rsrc" \
	"$assistant_root/src/apps/assistant/Assistant.rdef"
"$assistant_host_tools/xres" -o "$assistant_output" \
	"$assistant_resources/Assistant.rsrc"
file "$assistant_output"
shasum -a 256 "$assistant_output"
