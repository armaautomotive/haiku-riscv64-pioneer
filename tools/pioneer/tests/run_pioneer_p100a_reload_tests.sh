#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-p100a-reload.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for pair in DRIVER_RELOAD_STATE:reload_state DRIVER_RELOAD_CONTROL:reload_control PROBE_LOCK:lock QWEN35_PROBE_OPEN:probe_open QWEN35_PROBE_FREE:probe_free QWEN35_RECOVERY_STATUS:recovery_status QWEN35_RECOVERY_RUN:recovery_run QWEN35_DBI_CAPTURE:dbi_capture Q38_DISPATCH_COOKIE_RELEASE:cookie_release; do
 section=${pair%%:*}; name=${pair#*:}
 awk -v begin="PIONEER_${section}_BEGIN" -v end="PIONEER_${section}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/pioneer_$name.inc"
done
awk '/PIONEER_QWEN35_EXCLUSIVE_CLIENT_BEGIN/ {copying=1;next} /PIONEER_QWEN35_EXCLUSIVE_CLIENT_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_exclusive_client.inc"
awk '/PIONEER_QWEN35_LIVE_RESET_CLIENT_BEGIN/ {copying=1;next} /PIONEER_QWEN35_LIVE_RESET_CLIENT_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_live_reset_client.inc"
for pair in CLOSE:close END:end; do
 section=${pair%%:*}; name=${pair#*:}
 awk -v begin="PIONEER_QWEN35_${section}_CLIENT_BEGIN" -v end="PIONEER_QWEN35_${section}_CLIENT_END" \
  'index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}' \
  "$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_${name}_client.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/p100a_reload_stubs" -I"$temporary" "$directory/pioneer_p100a_reload_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/blackhole_asic_reset_test.cpp" -o "$temporary/asic-reset-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/asic-reset-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_post_reset_audit_test.cpp" -o "$temporary/post-reset-audit-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/post-reset-audit-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_post_reset_mapping_test.cpp" -o "$temporary/post-reset-mapping-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/post-reset-mapping-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_recovery_arc_queue_test.cpp" -o "$temporary/recovery-arc-queue-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/recovery-arc-queue-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_post_reset_pci_test.cpp" -o "$temporary/post-reset-pci-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/post-reset-pci-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_recovery_dbi_test.cpp" -o "$temporary/recovery-dbi-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/recovery-dbi-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_recovery_dbi_capture_test.cpp" -o "$temporary/recovery-dbi-capture-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/recovery-dbi-capture-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_recovery_owner_test.cpp" -o "$temporary/recovery-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/recovery-owner-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -pthread \
 -I"$temporary" "$directory/pioneer_qwen35_exclusive_open_test.cpp" -o "$temporary/exclusive-open-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/exclusive-open-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_exclusive_client_test.cpp" -o "$temporary/exclusive-client-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/exclusive-client-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_live_reset_client_test.cpp" -o "$temporary/live-reset-client-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/live-reset-client-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_live_reset_unload_test.cpp" -o "$temporary/live-reset-unload-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/live-reset-unload-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_post_reset_dispose_test.cpp" -o "$temporary/post-reset-dispose-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/post-reset-dispose-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_graph_recovery_proof_test.cpp" -o "$temporary/graph-recovery-proof-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-recovery-proof-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/recovery-run-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/recovery-run-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTEST_GRAPH_RECOVERY -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/graph-recovery-run-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-recovery-run-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTEST_GRAPH_RECOVERY -DTEST_LIVE_TAKEOVER -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/live-takeover-recovery-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/live-takeover-recovery-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTEST_RECOVERY_DISABLED -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/recovery-disabled-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/recovery-disabled-test"
printf 'Offline reload guard evidence retained at %s\n' "$temporary"
