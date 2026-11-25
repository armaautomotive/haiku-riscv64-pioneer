#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-p100a-reload.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
awk '/PIONEER_Q35_PCI_RESERVATION_RELEASE_BEGIN/ {copying=1;next} /PIONEER_Q35_PCI_RESERVATION_RELEASE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/q35_pci_reservation_release.inc"
awk '/PIONEER_SG2042_RECOVERY_LEASE_BEGIN/ {copying=1;next} /PIONEER_SG2042_RECOVERY_LEASE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../../../src/add-ons/kernel/busses/pci/ecam/ECAMPCIController.cpp" > "$temporary/sg2042_recovery_controller.inc"
awk '/PIONEER_SG2042_CONFIG_ACCESS_BEGIN/ {copying=1;next} /PIONEER_SG2042_CONFIG_ACCESS_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../../../src/add-ons/kernel/busses/pci/ecam/ECAMPCIController.cpp" > "$temporary/sg2042_config_access.inc"
awk '/PIONEER_Q35_ROOT_RECOVERY_GATEWAY_BEGIN/ {copying=1;next} /PIONEER_Q35_ROOT_RECOVERY_GATEWAY_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/pioneer_q35_root_recovery_gateway.inc"
awk 'BEGIN {copying=0} /^namespace \{/ {copying=1} copying {print} END {if(!copying)exit 1}' \
 "$directory/../../../src/add-ons/kernel/drivers/misc/PioneerPciProfile.cpp" > "$temporary/pioneer_pci_profile_actual.inc"
awk '/PIONEER_DM_DRIVER_PIN_BEGIN/ {copying=1;next} /PIONEER_DM_DRIVER_PIN_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../../../src/system/kernel/device_manager/device_manager.cpp" > "$temporary/pioneer_dm_driver_pin.inc"
for pair in DRIVER_RELOAD_STATE:reload_state DRIVER_RELOAD_CONTROL:reload_control PROBE_LOCK:lock QWEN35_PROBE_OPEN:probe_open QWEN35_PROBE_FREE:probe_free QWEN35_RECOVERY_STATUS:recovery_status QWEN35_RECOVERY_RUN:recovery_run QWEN35_DBI_CAPTURE:dbi_capture Q38_DISPATCH_COOKIE_RELEASE:cookie_release; do
 section=${pair%%:*}; name=${pair#*:}
 awk -v begin="PIONEER_${section}_BEGIN" -v end="PIONEER_${section}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/pioneer_$name.inc"
done
awk '/PIONEER_QWEN35_RECOVERY_QUALIFY_ROUTING_BEGIN/ {copying=1;next} /PIONEER_QWEN35_RECOVERY_QUALIFY_ROUTING_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/pioneer_recovery_qualify_routing.inc"
awk '/PIONEER_QWEN35_QUALIFY_GUARD_CHECK_BEGIN/ {copying=1;next} /PIONEER_QWEN35_QUALIFY_GUARD_CHECK_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/pioneer_qualify_guard_check.inc"
awk '/PIONEER_QWEN35_EXCLUSIVE_CLIENT_BEGIN/ {copying=1;next} /PIONEER_QWEN35_EXCLUSIVE_CLIENT_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_exclusive_client.inc"
awk '/PIONEER_QWEN35_LIVE_RESET_CLIENT_BEGIN/ {copying=1;next} /PIONEER_QWEN35_LIVE_RESET_CLIENT_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_live_reset_client.inc"
awk '/PIONEER_QWEN35_QUALIFY_CLIENT_BEGIN/ {copying=1;next} /PIONEER_QWEN35_QUALIFY_CLIENT_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_qualify_client.inc"
awk '/PIONEER_QWEN35_FLEET_INSPECTION_CLIENT_BEGIN/ {copying=1;next} /PIONEER_QWEN35_FLEET_INSPECTION_CLIENT_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_fleet_inspection_client.inc"
awk '/PIONEER_QWEN35_PROJECTION_CLIENT_BEGIN/ {copying=1;next} /PIONEER_QWEN35_PROJECTION_CLIENT_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_projection_client.inc"
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
 -I"$directory/p100a_reload_stubs" "$directory/pioneer_p100a_fabric_guard_test.cpp" -o "$temporary/fabric-guard-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fabric-guard-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/p100a_reload_stubs" "$directory/pioneer_p100a_fabric_guard_load_test.cpp" -o "$temporary/fabric-guard-load-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fabric-guard-load-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_pci_reservation_release_test.cpp" -o "$temporary/q35-pci-reservation-release-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-pci-reservation-release-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/blackhole_asic_reset_test.cpp" -o "$temporary/asic-reset-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/asic-reset-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_pcie_link_reset_test.cpp" -o "$temporary/pcie-link-reset-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pcie-link-reset-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -pthread \
 "$directory/sg2042_recovery_lease_test.cpp" -o "$temporary/sg2042-recovery-lease-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/sg2042-recovery-lease-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -pthread \
 -DPIONEER_SG2042_RECOVERY_WITNESS -I"$temporary" "$directory/sg2042_recovery_controller_test.cpp" -o "$temporary/sg2042-recovery-controller-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/sg2042-recovery-controller-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_pci_root_lease_scope_test.cpp" -o "$temporary/pioneer-pci-root-lease-scope-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pioneer-pci-root-lease-scope-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_root_recovery_gateway_test.cpp" -o "$temporary/pioneer-q35-root-recovery-gateway-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pioneer-q35-root-recovery-gateway-test"
${CXX:-c++} -std=gnu++20 -O1 -g -Wall -Wextra -Werror -Wno-missing-field-initializers -fsanitize=address,undefined -pthread \
 -I"$temporary" "$directory/pioneer_pci_profile_controller_test.cpp" -o "$temporary/pioneer-pci-profile-controller-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pioneer-pci-profile-controller-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -pthread \
 -I"$temporary" "$directory/pioneer_device_manager_pin_test.cpp" -o "$temporary/pioneer-device-manager-pin-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pioneer-device-manager-pin-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_post_reset_audit_test.cpp" -o "$temporary/post-reset-audit-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/post-reset-audit-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_post_reset_mapping_test.cpp" -o "$temporary/post-reset-mapping-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/post-reset-mapping-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_guarded_fleet_audit_test.cpp" -o "$temporary/q35-guarded-fleet-audit-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-guarded-fleet-audit-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_role_views_test.cpp" -o "$temporary/q35-fabric-role-views-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-role-views-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_auto_role_recovery_test.cpp" -o "$temporary/q35-fabric-auto-role-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-auto-role-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_pcie_root_aer_snapshot_test.cpp" -o "$temporary/root-aer-snapshot-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/root-aer-snapshot-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_recovery_arc_queue_test.cpp" -o "$temporary/recovery-arc-queue-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/recovery-arc-queue-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_arc_queue_test.cpp" -o "$temporary/q35-fabric-arc-queue-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-arc-queue-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_arc_owner_test.cpp" -o "$temporary/q35-fabric-arc-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-arc-owner-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_host_scope_test.cpp" -o "$temporary/q35-fabric-host-scope-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-host-scope-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_recovery_transaction_test.cpp" -o "$temporary/q35-fabric-recovery-transaction-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-recovery-transaction-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_recovery_composed_test.cpp" -o "$temporary/q35-fabric-recovery-composed-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-recovery-composed-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_host_composed_test.cpp" -o "$temporary/q35-fabric-host-composed-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-host-composed-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_recovery_start_test.cpp" -o "$temporary/q35-fabric-recovery-start-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-recovery-start-test"
awk '/PIONEER_Q35_FABRIC_REQUEST_BEGIN/ {copying=1;next} /PIONEER_Q35_FABRIC_REQUEST_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/pioneer_q35_fabric_request.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$temporary" \
 "$directory/pioneer_qwen35_fabric_recovery_request_test.cpp" -o "$temporary/q35-fabric-recovery-request-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-recovery-request-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_risc_reset_sample_test.cpp" -o "$temporary/q35-risc-reset-sample-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-risc-reset-sample-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_firmware_reset_test.cpp" -o "$temporary/firmware-reset-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/firmware-reset-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_firmware_reset_transport_test.cpp" -o "$temporary/firmware-reset-transport-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/firmware-reset-transport-test"
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsyntax-only \
 "$directory/pioneer_firmware_reset_compile.cpp"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_post_reset_pci_test.cpp" -o "$temporary/post-reset-pci-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/post-reset-pci-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_guarded_master_disable_test.cpp" -o "$temporary/q35-guarded-master-disable-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-guarded-master-disable-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_fabric_pcie_idle_audit_test.cpp" -o "$temporary/q35-fabric-pcie-idle-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/q35-fabric-pcie-idle-test"
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
 -I"$temporary" "$directory/pioneer_qwen35_projection_client_test.cpp" -o "$temporary/projection-client-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-client-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_live_reset_client_test.cpp" -o "$temporary/live-reset-client-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/live-reset-client-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_recovery_qualification_routing_test.cpp" -o "$temporary/qualification-routing-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/qualification-routing-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_QWEN35_SPAN_QUALIFY -I"$temporary" "$directory/pioneer_qwen35_recovery_qualification_routing_test.cpp" -o "$temporary/span-qualification-routing-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/span-qualification-routing-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_QWEN35_FLEET_INSPECTION_QUALIFY -I"$temporary" "$directory/pioneer_qwen35_recovery_qualification_routing_test.cpp" -o "$temporary/fleet-inspection-routing-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fleet-inspection-routing-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_QWEN35_FLEET_INSPECTION_QUALIFY -DPIONEER_QWEN35_RISC_RESET_SAMPLE_QUALIFY \
 -I"$temporary" "$directory/pioneer_qwen35_recovery_qualification_routing_test.cpp" -o "$temporary/risc-reset-sample-routing-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/risc-reset-sample-routing-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_QWEN35_FLEET_INSPECTION_QUALIFY -DPIONEER_QWEN35_ROLE_STAGE_QUALIFY -I"$temporary" "$directory/pioneer_qwen35_recovery_qualification_routing_test.cpp" -o "$temporary/role-stage-routing-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-stage-routing-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/role_haiku_stubs" "$directory/pioneer_qwen35_fleet_inspection_control_test.cpp" -o "$temporary/fleet-inspection-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fleet-inspection-control-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_QWEN35_RISC_RESET_SAMPLE_QUALIFY -I"$directory/role_haiku_stubs" \
 "$directory/pioneer_qwen35_fleet_inspection_control_test.cpp" -o "$temporary/risc-reset-sample-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/risc-reset-sample-control-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_active_root_lease_test.cpp" -o "$temporary/active-root-lease-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/active-root-lease-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_fleet_inspection_client_test.cpp" -o "$temporary/fleet-inspection-client-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fleet-inspection-client-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_recovery_qualification_guard_test.cpp" -o "$temporary/qualification-guard-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/qualification-guard-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_qwen35_recovery_qualification_client_test.cpp" -o "$temporary/qualification-client-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/qualification-client-test"
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
 -DTEST_FIRMWARE_RESET -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/firmware-recovery-run-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/firmware-recovery-run-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTEST_FIRMWARE_RESET -DTEST_GRAPH_RECOVERY -DTEST_LIVE_TAKEOVER -DTEST_RECOVERY_QUALIFY \
 -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/firmware-qualification-run-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/firmware-qualification-run-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTEST_GRAPH_RECOVERY -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/graph-recovery-run-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-recovery-run-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTEST_GRAPH_RECOVERY -DTEST_LIVE_TAKEOVER -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/live-takeover-recovery-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/live-takeover-recovery-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTEST_GRAPH_RECOVERY -DTEST_LIVE_TAKEOVER -DTEST_RECOVERY_QUALIFY -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/idle-qualification-recovery-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/idle-qualification-recovery-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTEST_RECOVERY_DISABLED -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/recovery-disabled-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/recovery-disabled-test"
python3 "$directory/pioneer_qwen35_fleet_inspection_qualify_script_test.py"
PYTHONDONTWRITEBYTECODE=1 python3 "$directory/pioneer_qwen35_recovery_trace_qualify_test.py"
PYTHONDONTWRITEBYTECODE=1 python3 "$directory/pioneer_qwen35_risc_sample_qualify_test.py"
printf 'Offline reload guard evidence retained at %s\n' "$temporary"
