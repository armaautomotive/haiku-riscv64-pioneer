#!/bin/sh
# OS mapping adapter only, not a native model/scope/launch capability.
set -eu
[ "$#" -le 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-layer-native-mapping.XXXXXXXX)
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT=1 -DPIONEER_Q38_DECODER_RUNTIME=1 -DPIONEER_Q38_GDN_MUTABLE_STATE=1 -DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1 -DPIONEER_Q38_PROJECTION_RESULT_STASH=1 \
 -DPIONEER_Q38_LEARNED_RMS_NATIVE=1 -DPIONEER_Q38_EMBEDDING_NATIVE=1 \
 "$directory/pioneer_q38_resident_layer_native_mapping_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
if [ "$#" = 1 ];then
 sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
 "$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
  -I"$sdk/headers" -I"$sdk/headers/os" -I"$sdk/headers/os/drivers" -I"$sdk/headers/os/kernel" -I"$sdk/headers/os/support" \
  -I"$sdk/headers/os/storage" \
  -idirafter "$sdk/headers/posix" -c \
  "$directory/pioneer_q38_resident_layer_native_mapping_kernel_compile.cpp" -o "$temporary/mapping.o"
 awk -F '\t' 'NF==3 {if($2+0>maximum)maximum=$2+0;if($2+0>2048||$3!="static")bad=1;seen=1} END {if(!seen||bad)exit 1;print "MAX_LAYER_MAPPING_KERNEL_FRAME_BYTES="maximum}' "$temporary/mapping.su"
 shasum -a 256 "$temporary/mapping.o"
 printf 'Native mapping adapter SDK compile ONLY:%s;SDK read-only,NO actual native factory/driver/link/install/card\n' "$temporary"
fi
