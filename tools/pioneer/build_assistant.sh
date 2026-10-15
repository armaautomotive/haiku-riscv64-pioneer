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

for assistant_variant in normal p100a-preview; do
assistant_variant_flags=
assistant_variant_output=$assistant_output
if [ "$assistant_variant" = p100a-preview ]; then
	assistant_variant_flags=-DASSISTANT_P100A_PREVIEW
	assistant_variant_output=${assistant_output}P100A
fi
"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar $assistant_variant_flags \
	"$@" -B"$assistant_release/system/glue/" \
	-L"$assistant_release/kits" -L"$assistant_release/system/libroot" \
	-I"$assistant_root/headers/private/shared" \
	-I"$assistant_root/headers/private/libroot" \
	"$assistant_root/src/apps/assistant/Assistant.cpp" \
	"$assistant_root/src/apps/assistant/ModelManager.cpp" \
	"$assistant_root/src/apps/assistant/HardwareWindow.cpp" \
	"$assistant_root/src/apps/assistant/ModelStore.cpp" \
	"$assistant_root/src/apps/assistant/ToolBridge.cpp" \
	"$assistant_root/src/apps/assistant/ToolSession.cpp" \
	"$assistant_release/kits/shared/libshared.a" -lbe -o "$assistant_variant_output"
done
assistant_host_tools=$assistant_build/objects/darwin/arm64/release/tools
assistant_resources=$(mktemp -d /private/tmp/haiku-assistant-resources.XXXXXX)
"$assistant_host_tools/rc/rc" -o "$assistant_resources/Assistant.rsrc" \
	"$assistant_root/src/apps/assistant/Assistant.rdef"
"$assistant_host_tools/xres" -o "$assistant_output" \
	"$assistant_resources/Assistant.rsrc"
file "$assistant_output"
shasum -a 256 "$assistant_output"

"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
	"$@" -B"$assistant_release/system/glue/" -L"$assistant_release/kits" \
	-L"$assistant_release/system/libroot" \
	"$assistant_root/src/apps/assistant/HardwareApp.cpp" \
	"$assistant_root/src/apps/assistant/HardwareWindow.cpp" -lbe -o "${assistant_output}Hardware"
"$assistant_host_tools/rc/rc" -o "$assistant_resources/Hardware.rsrc" \
	"$assistant_root/src/apps/assistant/Hardware.rdef"
"$assistant_host_tools/xres" -o "${assistant_output}Hardware" "$assistant_resources/Hardware.rsrc"

"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
	"$@" -B"$assistant_release/system/glue/" -L"$assistant_release/kits" \
	-L"$assistant_release/system/libroot" \
	"$assistant_root/src/apps/assistant/HardwareTest.cpp" -lbe -o "${assistant_output}HardwareTest"

if [ -n "${ASSISTANT_LLAMA_LIB_DIR:-}" ]; then
	"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
		"$@" -I"$assistant_root/src/libs/llama.cpp/include" \
		-I"$assistant_root/src/libs/llama.cpp/ggml/include" \
		-I"$assistant_root/headers/private/libroot" \
		-B"$assistant_release/system/glue/" -L"$assistant_release/kits" \
		-L"$assistant_release/system/libroot" -L"$ASSISTANT_LLAMA_LIB_DIR" \
		-Wl,-rpath-link,"$ASSISTANT_LLAMA_LIB_DIR" \
		"$assistant_root/src/apps/assistant/Service.cpp" \
		"$assistant_root/src/apps/assistant/P100ABackend.cpp" \
		"$assistant_root/src/apps/assistant/ModelStore.cpp" \
		-lbe -l:libllama.so.0.2.0 -l:libggml.so.0 \
		-o "${assistant_output}Service"
	"$assistant_host_tools/rc/rc" -o "$assistant_resources/Service.rsrc" \
		"$assistant_root/src/apps/assistant/Service.rdef"
	"$assistant_host_tools/xres" -o "${assistant_output}Service" "$assistant_resources/Service.rsrc"
	shasum -a 256 "${assistant_output}Service"
fi

"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
	-fPIC -shared "$@" -isystem "$assistant_sdk/headers/os/add-ons/input_server" \
	-B"$assistant_release/system/glue/" -L"$assistant_release/kits" \
	-L"$assistant_release/servers/input" -L"$assistant_release/system/libroot" \
	"$assistant_root/src/apps/assistant/ShortcutFilter.cpp" -lbe \
	"$assistant_release/servers/input/input_server" \
	-o "${assistant_output}Shortcut"
shasum -a 256 "${assistant_output}Shortcut"

"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
	"$@" -B"$assistant_release/system/glue/" -L"$assistant_release/kits" \
	-L"$assistant_release/system/libroot" \
	-I"$assistant_root/headers/private/libroot" \
	"$assistant_root/src/apps/assistant/ServiceControl.cpp" \
	"$assistant_root/src/apps/assistant/ModelStore.cpp" -lbe \
	-o "${assistant_output}Control"

"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
	"$@" -B"$assistant_release/system/glue/" -L"$assistant_release/kits" \
	-L"$assistant_release/system/libroot" -I"$assistant_root/headers/private/libroot" \
	"$assistant_root/src/apps/assistant/ModelStoreTest.cpp" \
	"$assistant_root/src/apps/assistant/ModelStore.cpp" -lbe \
	-o "${assistant_output}ModelStoreTest"

"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
	"$@" -I"$assistant_root/headers/private/app" -I"$assistant_root/headers/private/interface" \
	-isystem "$assistant_sdk/headers/os/add-ons/graphics" \
	-B"$assistant_release/system/glue/" -L"$assistant_release/kits" \
	-L"$assistant_release/system/libroot" \
	"$assistant_root/src/apps/assistant/ToolsService.cpp" -lbe \
	-o "${assistant_output}Tools"
"$assistant_host_tools/rc/rc" -o "$assistant_resources/Tools.rsrc" \
	"$assistant_root/src/apps/assistant/ToolsService.rdef"
"$assistant_host_tools/xres" -o "${assistant_output}Tools" "$assistant_resources/Tools.rsrc"
"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
	"$@" -B"$assistant_release/system/glue/" -L"$assistant_release/kits" \
	-L"$assistant_release/system/libroot" \
	"$assistant_root/src/apps/assistant/ToolsControl.cpp" -lbe \
	-o "${assistant_output}ToolsControl"
shasum -a 256 "${assistant_output}Tools" "${assistant_output}ToolsControl"

# Optional OFFLINE adapter test for native Haiku: software BF16 only, no
# device header/open/ioctl. The independently pinned C reference is data
# supplied explicitly by the developer, not downloaded or executed here.
if [ -n "${ASSISTANT_TINY_BOOTSTRAP_DIR:-}" ]; then
	assistant_cc=$assistant_build/cross-tools-riscv64/bin/riscv64-unknown-haiku-gcc
	"$assistant_cc" -std=c11 -O1 -ffp-contract=off -Wno-unknown-pragmas "$@" \
		-DPIONEER_TINY_REFERENCE_SOURCE="\"$ASSISTANT_TINY_BOOTSTRAP_DIR/run.c\"" \
		-c "$assistant_root/tools/pioneer/tests/pioneer_tiny_reference_bridge.c" \
		-o "${assistant_output}TinyReference.o"
	"$assistant_cxx" -std=gnu++17 -O1 -ffp-contract=off -Wall -Wextra -Werror \
		"$@" -B"$assistant_release/system/glue/" -L"$assistant_release/system/libroot" \
		"$assistant_root/tools/pioneer/tests/pioneer_assistant_p100a_test.cpp" \
		"${assistant_output}TinyReference.o" -o "${assistant_output}P100AOfflineTest"
fi

"$assistant_cxx" -std=gnu++11 -O2 -Wall -Wextra -Werror -Wno-multichar \
	"$@" -I"$assistant_root/headers/private/shared" \
	-B"$assistant_release/system/glue/" -L"$assistant_release/kits" \
	-L"$assistant_release/system/libroot" \
	"$assistant_root/src/apps/assistant/ToolFlowTest.cpp" \
	"$assistant_root/src/apps/assistant/ToolBridge.cpp" \
	"$assistant_root/src/apps/assistant/ToolSession.cpp" \
	"$assistant_release/kits/shared/libshared.a" -lbe \
	-o "${assistant_output}ToolFlowTest"
