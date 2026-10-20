/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcQueueProbe.h"
using uint32=uint32_t;
using area_id=int;
constexpr int B_OK=0;
struct tt_pioneer_profile {};
static std::vector<int> removed;
static unsigned failDelete=0,hardwareCleanup=0;
static int delete_area(area_id view) {
	removed.push_back(view); return removed.size()==failDelete?-1:0;
}
struct PioneerArcQueryIO {
	area_id queueWriteArea=101,msiArea=102,csmArea=103,scratchArea=104,tlbArea=105,area=106;
	volatile uint32* queueWrite=nullptr;
	volatile uint32* msi=nullptr;
	const volatile uint32* csm=nullptr;
	const volatile uint32* scratch=nullptr;
	const volatile uint32* nodeID=nullptr;
	volatile uint32* tlb=nullptr;
	PioneerArcQueryIO(const tt_pioneer_profile&,tt_arc_scratch_result&,tt_arc_telemetry_result&,tt_arc_queue_result&) {}
	bool Unmap() { hardwareCleanup++; return false; } // forbidden inherited diagnostic cleanup
};
#include "pioneer_q38_clock_io.inc"
int main()
{
	tt_pioneer_profile p; tt_arc_scratch_result a={}; tt_arc_telemetry_result t={}; tt_arc_queue_result r={};
	for (unsigned fault=0;fault<=6;fault++) {
		removed.clear(); failDelete=fault;
		PioneerQ38ClockIO io(p,a,t,r);
		assert(io.Unmap()==!fault && !hardwareCleanup);
		assert(removed.size()==(fault?fault:6));
		area_id values[]={io.queueWriteArea,io.msiArea,io.csmArea,io.scratchArea,io.tlbArea,io.area};
		for (unsigned i=0;i<6;i++) assert(values[i]==(i<(fault?fault-1:6)?-1:int(101+i)));
		if (!fault) { assert(io.Unmap() && removed.size()==6 && !hardwareCleanup); }
	}
	puts("PASS ACTUAL native clock Unmap:virtual views ONLY,single attempt,first failed deletion stops,NO inherited hardware cleanup/TLB writes/ARC messages");
}
