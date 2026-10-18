/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_MMIO_END_H
#define PIONEER_Q38_DISPATCH_MMIO_END_H
#include "BlackholePioneerMmioProbe.h"
//54 ONLY:caller has proved ALL participants terminal,held,restored and idle.
// First failed unmap/config readback stops this transaction. No retries or
// later PCI writes;caller RETAINS its entire owner/reservation/host buffers.
template<class IO> bool PioneerQ38DispatchMmioEnd(IO& io,tt_pioneer_mmio_result& r,tt_pioneer_mmio_transaction& t)
{
	if (!t.config_attempted) return r.restored==1;
	t.config_attempted=false; // terminal attempt;even a mistaken repeat does NO IO
	uint32_t originalState=r.state; r.state=TT_PIONEER_MMIO_RESTORE_FAILED; r.restored=0;
	if (t.map_attempted && !io.Unmap()) return false;
	t.map_attempted=false;
	io.CardWrite(4,2,0); if (io.CardRead(4,2)) return false;
	io.RootWrite(4,2,0); if (io.RootRead(4,2)) return false;
	const unsigned order[]={5,4,1,0,3,2};
	for (unsigned i:order) {
		io.CardWrite(0x10+i*4,4,r.bars_before[i]);
		if (io.CardRead(0x10+i*4,4)!=r.bars_before[i]) return false;
	}
	const unsigned bridgeOrder[]={1,2,0};
	for (unsigned i:bridgeOrder) {
		io.RootWrite(0x24+i*4,4,r.bridge_before[i]);
		if (io.RootRead(0x24+i*4,4)!=r.bridge_before[i]) return false;
	}
	bool okay=true;
	for (unsigned i=0;i<6;i++) { r.bars_after[i]=io.CardRead(0x10+i*4,4); okay &= r.bars_after[i]==r.bars_before[i]; }
	for (unsigned i=0;i<3;i++) { r.bridge_after[i]=io.RootRead(0x24+i*4,4); okay &= r.bridge_after[i]==r.bridge_before[i]; }
	r.commands_after[0]=io.CardRead(4,2); r.commands_after[1]=io.RootRead(4,2);
	okay &= !r.commands_after[0] && !r.commands_after[1];
	if (!okay) return false;
	r.state=originalState; r.restored=1; r.stage=5; return true;
}
#endif
