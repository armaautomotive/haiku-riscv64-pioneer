/* SPDX-License-Identifier: MIT */
// Kernel-compatible RV64 compile/stack witness ONLY. These IO declarations
// intentionally have NO implementation;this object is NOT a native adapter
// or a linked driver. No VM/STL/test-harness frames contaminate its results.
// Deliberately NO PCI-write,NoC-command-write,host-data-load/store or release
// APIs:accidentally granting any of those capabilities makes compilation fail.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StateZeroController.h"
struct StateZeroCompileIO {
	bool StateZeroBeforeCohort(); bool StateZeroOtherCoresHeld();
	bool SelectRuntimeCore(unsigned); bool Q38PanelMappingStable(); bool BulkMappingStable();
	bool SpanViewCapacity(unsigned);
	uint32_t ResetRead(); uint32_t SourceRead(unsigned); uint32_t ResidentNocRead(uint32_t);
	uint32_t HostPciRead(unsigned,uint16_t,uint8_t);
	int HostPageMap(uint64_t&,uint64_t&); int OutputBufferMap(uint64_t&,uint64_t&);
	int OutputBufferPageMap(unsigned,uint64_t&,uint64_t&);
	int WideOutputMap(uint64_t&,uint64_t&); int WideOutputPageMap(unsigned,uint64_t&,uint64_t&);
	void QuarantineHostPage(); void QuarantineOutputBuffer(); void QuarantineWideOutput();
	uint32_t* Q38PanelSavedWords(); const uint32_t* BulkSavedWords();
	uint32_t RuntimeCodeRead(unsigned); void RuntimeCodeWrite(unsigned,uint32_t);
	uint32_t Q38PanelRead(uint32_t); void Q38PanelWrite(uint32_t,uint32_t);
	uint32_t BulkL1Read(unsigned); void BulkL1Write(unsigned,uint32_t);
	uint32_t RuntimeStatusRead(unsigned); void RuntimeStatusWrite(unsigned,uint32_t);
	void Pause(); uint64_t Q38CohortNow(); void RuntimeResetWrite(uint32_t); uint32_t RuntimePcRead();
};
extern "C" bool pioneer_state_zero_prepare_compile(StateZeroCompileIO& io,const tt_dram_samples_result& dram,
	const tt_compute_noc_result& noc,PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const PioneerQ38GdnIdentity& id,
	const tt_host_page_result& input,PioneerResidentLease& store,PioneerBulkUploadLease& bulk,
	PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,PioneerQ38StateZeroLease& lease)
{
	return PioneerQ38StateZeroPrepare(io,dram,noc,state,banks,id,input,store,bulk,transfer,wide,lease);
}
extern "C" bool pioneer_state_zero_run_compile(StateZeroCompileIO& io,const tt_dram_samples_result& dram,
	const tt_compute_noc_result& noc,PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const tt_host_page_result& input,
	PioneerResidentLease& store,PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,
	PioneerWideOutputLease& wide,PioneerQ38StateZeroLease& lease,const PioneerQ38GdnIdentity& id,
	unsigned region,unsigned first,unsigned pages,PioneerQ38StateZeroResult& result)
{
	return PioneerQ38StateZeroRun(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,id,region,first,pages,result);
}
extern "C" bool pioneer_state_zero_close_compile(StateZeroCompileIO& io,const tt_dram_samples_result& dram,
	const tt_compute_noc_result& noc,PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const tt_host_page_result& input,
	PioneerResidentLease& store,PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,
	PioneerWideOutputLease& wide,PioneerQ38StateZeroLease& lease,bool clean)
{
	return PioneerQ38StateZeroClose(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,clean);
}
