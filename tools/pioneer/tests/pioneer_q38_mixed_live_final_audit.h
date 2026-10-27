/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_FINAL_AUDIT_TEST_H
#define PIONEER_Q38_MIXED_LIVE_FINAL_AUDIT_TEST_H
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeHostDmaAudit.h"
// TEST raw-register callback boundary. No native owner,permission,closed
// result or clock publication crosses this interface. Every result below
// is updated by the VERBATIM production AfterNodeId/probe chain.
struct MixedFinalAuditEndpoints {
 enum Kind{Tlb,Scratch,Csm,Niu,Reset,Source,DramId,Compute,Inbound,Sii,Pci,Hdma};
 void* object;
 uint32_t (*read)(void*,Kind,unsigned,unsigned,unsigned);
 void (*writeTlb)(void*,unsigned,unsigned,uint32_t);
};
struct MixedFinalAuditResults {
 tt_arc_scratch_result& arc;tt_arc_telemetry_result& telemetry;tt_tensix_reset_result& tensix;
 tt_dram_samples_result& dram;tt_compute_noc_result& noc;tt_host_path_result& path;tt_host_dma_audit_result& audit;
 bool& dirty;bool& telemetryAttempted;bool& resetAttempted;bool& dramAttempted;
 bool& nocAttempted;bool& pathAttempted;bool& auditAttempted;
};
bool ExecuteMixedFinalAudit(const MixedFinalAuditEndpoints&,MixedFinalAuditResults&,uint32_t);
#endif
