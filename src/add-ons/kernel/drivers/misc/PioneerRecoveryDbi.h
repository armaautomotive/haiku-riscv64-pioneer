/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RECOVERY_DBI_H
#define PIONEER_RECOVERY_DBI_H

#include <stdint.h>

// Blackhole DBI Device Control/Status, addressed through the selected PCIe
// NoC node. This helper deliberately does not configure an aperture: its IO
// must prove the route and quiescence and perform bounded 32-bit accesses.
// No hardware transport is wired up yet; recovery remains disabled.
static constexpr uint64_t PIONEER_RECOVERY_DBI_ADDRESS = UINT64_C(0xf800000000000078);
static constexpr uint32_t PIONEER_RECOVERY_DBI_MPS_MASK = 0x00e0;

struct PioneerRecoveryDbiSnapshot {
	uint32_t mps=0;
	bool valid=false;
};

static inline bool PioneerRecoveryDbiControlValid(uint32_t value)
{
	// Reject FLR, PCIe error/transaction-pending status, and failed reads.
	return value!=UINT32_MAX && !(value&0x8000) && !((value>>16)&0x2f)
		&& ((value>>5)&7)<=5;
}

// Capture while the session is healthy, not after a fault. The PCI config
// MPS is independently validated by the caller and must agree with DBI.
template<class IO>
bool PioneerRecoveryDbiCapture(IO& io,uint32_t pciControl,PioneerRecoveryDbiSnapshot& out)
{
	out={};
	if (pciControl>0xffff || (pciControl&0x8000) || !io.DbiReady()) return false;
	const uint32_t first=io.DbiRead(),second=io.DbiRead();
	if (!io.DbiReady() || first!=second || !PioneerRecoveryDbiControlValid(first)
		|| ((first^pciControl)&PIONEER_RECOVERY_DBI_MPS_MASK)) return false;
	out.mps=first&PIONEER_RECOVERY_DBI_MPS_MASK;
	out.valid=true;return true;
}

// A write is potentially irreversible even when verification fails: callers
// must retain their owner and mappings whenever the result is false. Upper
// status bits are written as zero so W1C status is never acknowledged; FLR
// is rejected rather than propagated. No unrelated control bits are changed.
template<class IO>
bool PioneerRecoveryDbiRestore(IO& io,uint32_t pciControl,
	const PioneerRecoveryDbiSnapshot& saved,bool& writeAttempted)
{
	writeAttempted=false;
	if (!saved.valid || (saved.mps&~PIONEER_RECOVERY_DBI_MPS_MASK)
		|| (saved.mps>>5)>5 || pciControl>0xffff || (pciControl&0x8000)
		|| ((pciControl^saved.mps)&PIONEER_RECOVERY_DBI_MPS_MASK)
		|| !io.DbiReady()) return false;
	const uint32_t first=io.DbiRead(),second=io.DbiRead();
	if (!io.DbiReady() || first!=second || !PioneerRecoveryDbiControlValid(first)) return false;
	const uint32_t wanted=((first&0xffff)&~PIONEER_RECOVERY_DBI_MPS_MASK)|saved.mps;
	if ((first&0xffff)!=wanted) {
		writeAttempted=true;
		io.DbiWrite(wanted);
	}
	const uint32_t after=io.DbiRead(),confirm=io.DbiRead();
	return io.DbiReady() && after==confirm && PioneerRecoveryDbiControlValid(after)
		&& (after&0xffff)==wanted;
}
#endif
