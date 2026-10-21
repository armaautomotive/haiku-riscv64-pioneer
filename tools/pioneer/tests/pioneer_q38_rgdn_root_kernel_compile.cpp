/* SPDX-License-Identifier: MIT */
// OBJECT ONLY,NO native IO implementations/driver/selection.
#define PIONEER_Q38_RGDN_ROOT_QUALIFICATION
#include "pioneer_q38_state_zero_kernel_compile.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnRootController.h"
struct RgdnRootCompileIO:StateZeroCompileIO {
 bool ResidentGdnBeforeRoot(); bool ResidentGdnOtherCoresHeld();
};
extern "C" bool pioneer_rgdn_root_prepare_compile(RgdnRootCompileIO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
 PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const PioneerQ38GdnIdentity& identity,const tt_host_page_result& input,
 PioneerResidentLease& store,PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,
 PioneerQ38ResidentGdnRootLease& lease) {
 return PioneerQ38RgdnRootPrepare(io,dram,noc,state,banks,identity,input,store,bulk,transfer,wide,lease);
}
extern "C" bool pioneer_rgdn_root_run_compile(RgdnRootCompileIO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
 PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const tt_host_page_result& input,PioneerResidentLease& store,
 PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,
 PioneerQ38ResidentGdnRootLease& lease,tt_q38_rgdn_root_request& job) {
 return PioneerQ38RgdnRootRun(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,job);
}
extern "C" bool pioneer_rgdn_root_close_compile(RgdnRootCompileIO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
 PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const tt_host_page_result& input,PioneerResidentLease& store,
 PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,
 PioneerQ38ResidentGdnRootLease& lease,bool clean) {
 return PioneerQ38RgdnRootClose(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,clean);
}
