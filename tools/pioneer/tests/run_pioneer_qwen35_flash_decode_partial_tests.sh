#!/bin/sh
# Host numerical witness only; this never loads a driver or accesses a card.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/q35-flash-partial.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_dataflow_flash_decode_partial_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_dataflow_flash_hybrid_worker_test.cpp" -o "$temporary/worker-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/worker-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_flash_partial_protocol_test.cpp" -o "$temporary/protocol-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/protocol-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_flash_code_owner_test.cpp" -o "$temporary/code-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/code-owner-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_flash_code_pci_view_test.cpp" -o "$temporary/code-view-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/code-view-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_flash_role_images_composition_test.cpp" -o "$temporary/role-images-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-images-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_flash_role_recovery_composition_test.cpp" -o "$temporary/role-recovery-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-recovery-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_flash_role_pci_scope_test.cpp" -o "$temporary/role-pci-scope-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-pci-scope-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/role_haiku_stubs" "$directory/pioneer_qwen35_flash_actual_role_pci_test.cpp" -o "$temporary/actual-role-pci-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/actual-role-pci-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_flash_hybrid_graph_test.cpp" -o "$temporary/graph-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_ffn_pair_sequence_test.cpp" -o "$temporary/ffn-pair-sequence"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/ffn-pair-sequence"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_ffn_pair_graph_test.cpp" -o "$temporary/ffn-pair-graph"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/ffn-pair-graph"
ffn_flags='-DPIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_PAIRED_RECURRENCE -DPIONEER_QWEN35_FUSED_RECURRENCE -DPIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY -DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS'
for header in PioneerQwen35DataflowFfnSequence.h PioneerQwen35DataflowGraphRunProtocol.h; do
 printf '#include "%s"\n' "$directory/../$header" > "$temporary/ffn-guard.cpp"
 # Deliberate splitting supplies the individual compiler flags.
 ${CXX:-c++} -std=c++17 $ffn_flags -fsyntax-only "$temporary/ffn-guard.cpp"
 for flags in '-DPIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY' \
  "$ffn_flags -DPIONEER_QWEN35_RECOVERY_QUALIFY" "$ffn_flags -DPIONEER_QWEN35_DEVICE_DISPATCH_TRIAL"; do
  if ${CXX:-c++} -std=c++17 $flags -fsyntax-only "$temporary/ffn-guard.cpp" > /dev/null 2>&1; then
   echo 'FAIL paired FFN graph build guard' >&2;exit 1
  fi
 done
done
for header in PioneerQwen35DataflowDenseAttentionPartial.h PioneerQwen35DataflowDenseAttentionHybrid.h \
 PioneerQwen35DataflowFlashHybridProgram.h \
 PioneerQwen35DataflowFlashPartialProtocol.h PioneerQwen35DataflowBlackholeFlashHybridIO.h; do
 printf '#include "%s"\n' "$directory/../$header" > "$temporary/guard.cpp"
 for flags in '' '-DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY' '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY' \
  '-DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_QUALIFY'; do
  # Deliberate word splitting provides separate compiler arguments.
  if ${CXX:-c++} -std=c++17 $flags -fsyntax-only "$temporary/guard.cpp" > /dev/null 2>&1; then
   echo 'FAIL flash-decode build guard' >&2;exit 1
  fi
 done
done
for header in PioneerQwen35DataflowFlashCodeOwner.h PioneerQwen35DataflowFlashCodePciView.h PioneerQwen35DataflowFlashRolePciScope.h; do
 printf '#include "%s"\n' "$directory/../$header" > "$temporary/guard.cpp"
 for flags in '' '-DPIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS' '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY' \
  '-DPIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_QUALIFY'; do
  if ${CXX:-c++} -std=c++17 $flags -fsyntax-only "$temporary/guard.cpp" > /dev/null 2>&1; then
   echo 'FAIL flash-code owner build guard' >&2;exit 1
  fi
 done
done
echo 'PASS flash-decode partials reject unguarded and qualification builds'
