/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowDeviceBarrier.h"

// Compile the real NoC-backed barrier for the worker ISA; never executed by
// the current service or exposed as a card operation.
extern "C" unsigned PioneerQwen35DeviceBarrierCompileProbe(
	unsigned core,unsigned epoch,unsigned step,unsigned status,unsigned detail)
{
	PioneerQwen35::DataflowBlackholeBarrierIO io(core);
	PioneerQwen35::DataflowDeviceBarrier<PioneerQwen35::DataflowBlackholeBarrierIO> barrier(io);
	if (!barrier.Start(core,epoch,step,status,detail)) return PioneerQwen35::DataflowBarrierUnsafe;
	for (unsigned poll=0;poll<65536;poll++) {
		const auto result=barrier.Poll();
		if (result!=PioneerQwen35::DataflowBarrierPending) return result;
	}
	barrier.Expire();return barrier.Poll();
}
