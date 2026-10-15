/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RESIDENT_SESSION_H
#define PIONEER_RESIDENT_SESSION_H
#include <stdint.h>
#include "BlackholeHostReadProbe.h"
enum {
	TT_RESIDENT_SESSION_REFUSED=0,TT_RESIDENT_SESSION_READY=1,TT_RESIDENT_SESSION_CLOSED=2,
	TT_RESIDENT_SESSION_PREFLIGHT_FAILED=3,TT_RESIDENT_SESSION_HOST_FAILED=4,TT_RESIDENT_SESSION_CLEANUP_FAILED=5,
	TT_RESIDENT_SESSION_CHUNK=4096
};
static inline bool PioneerResidentCapacity(uint64_t bytes)
{
	// Initial arena is bank 0 only, after the pinned HAL's low reservation.
	// 2GiB fits below the 0xff000000 NIU reservation; enough for ordinary
	// Qwen3-0.6B BF16 weights. 27B requires quantized multi-bank residency.
	return bytes>=4096 && bytes<=UINT64_C(0x80000000) && (bytes&4095)==0;
}
static inline bool PioneerResidentOffset(uint64_t capacity,uint64_t offset)
{
	return PioneerResidentCapacity(capacity) && (offset&4095)==0 && offset<capacity && capacity-offset>=4096;
}
static inline uint32_t PioneerResidentAddress(uint64_t capacity,uint64_t offset)
{
	return PioneerResidentOffset(capacity,offset)?uint32_t(UINT64_C(0x04000000)+offset):UINT32_MAX;
}
struct tt_resident_session_result {
	uint32_t version,state,active,host_ready,pci_restored,host_released,host_address_bits,cold;
	uint64_t bytes;
	uint32_t route_state,audit_state,capabilities,uploaded_pages;
};
struct tt_resident_session_idle_result {
	uint32_t ready,tlb_restored,recovery_required,before[3];
};
// No NIU/config/payload writes. Both outgoing-write and read IDs must be
// empty, including the former which the initial read-only route probe does
// not inspect. Only the whitelisted sole-tile TLB control is changed.
template<class IO>
bool PioneerResidentSessionIdle(IO& io,const tt_compute_noc_result& noc,tt_resident_session_idle_result& r)
{
	r={}; r.tlb_restored=1;
	if (noc.version!=1 || noc.state!=TT_COMPUTE_NOC_PASSED || !noc.tlb_restored || noc.recovery_required) return false;
	for (unsigned w=0;w<3;w++) r.before[w]=io.DramTlbRead(1,w);
	if (r.before[2]&0xffffc000) return false;
	r.tlb_restored=0;
	if (BlackholeDramConfigure(io,1,0xffa00000,1,2,false)) {
		bool stable=BlackholeResidentSourceStable(io,noc,noc.source[0][0]);
		r.ready=stable && BlackholeResidentIdle(io) && (io.ResidentNocRead(0x178)&0xffff)==0;
		for (unsigned w=0;w<10;w++) r.ready &= io.ResidentNocRead(w*4)==0;
		if (!stable) r.recovery_required=1;
	}
	for (unsigned attempt=0;attempt<2 && !r.tlb_restored;attempt++) {
		for (unsigned w=0;w<3;w++) io.DramTlbWrite(1,w,r.before[w]);
		r.tlb_restored=1;
		for (unsigned w=0;w<3;w++) r.tlb_restored &= io.DramTlbRead(1,w)==r.before[w];
	}
	if (!r.tlb_restored) r.recovery_required=1;
	return r.ready && !r.recovery_required;
}
#endif
