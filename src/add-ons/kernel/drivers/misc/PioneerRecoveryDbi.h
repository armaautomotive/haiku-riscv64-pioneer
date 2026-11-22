/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RECOVERY_DBI_H
#define PIONEER_RECOVERY_DBI_H

#include <stdint.h>

// Blackhole DBI Device Control/Status, addressed through the selected PCIe
// NoC node. This helper deliberately does not configure an aperture: its IO
// must prove the route and quiescence and perform bounded 32-bit accesses.
// Recovery remains disabled until the hardware transport is qualified.
static constexpr uint64_t PIONEER_RECOVERY_DBI_ADDRESS = UINT64_C(0xf800000000000078);
static constexpr uint32_t PIONEER_RECOVERY_DBI_MPS_MASK = 0x00e0;

// The aperture uses the same 2 MiB BAR0 TLB encoding as the existing host
// DMA audit, but targets the beginning of DBI instead of the HDMA window.
// It is restricted to the currently supported PCIe node (2,0).
static inline uint32_t PioneerRecoveryDbiTlbWord(unsigned word,bool writable)
{
	const uint64_t base=PIONEER_RECOVERY_DBI_ADDRESS & ~UINT64_C(0x1fffff);
	if (word==0) return uint32_t(base>>21);
	if (word==1) return uint32_t((base>>53)&0x7ff)|(2U<<11)|(2U<<23);
	return word==2?(writable?0x40U:0xa40U):UINT32_MAX;
}

// IO owns the aperture and supplies independent PCI, route and quiescence
// checks. Never destroy it or its owner while Dirty(): even a failed TLB write
// may have changed the device. No destructor attempts hardware cleanup.
template<class IO>
class PioneerRecoveryDbiAperture {
public:
	explicit PioneerRecoveryDbiAperture(IO& io):fIO(io) {}
	bool Begin(bool writable)
	{
		if (fAttempted || !fIO.DbiRouteStable()) return false;
		fAttempted=true;fWritable=writable;
		for (unsigned i=0;i<3;i++) fSaved[i]=fIO.DbiTlbRead(i);
		if ((fSaved[2]&0xffffc000) || !fIO.DbiRouteStable()) return false;
		fDirty=true;
		for (unsigned i=0;i<3;i++) fIO.DbiTlbWrite(i,PioneerRecoveryDbiTlbWord(i,writable));
		fActive=true;
		return DbiReady();
	}
	bool DbiReady()
	{
		if (!fActive || !fIO.DbiRouteStable()) return false;
		for (unsigned i=0;i<3;i++)
			if (fIO.DbiTlbRead(i)!=PioneerRecoveryDbiTlbWord(i,fWritable)) return false;
		return fIO.DbiRouteStable();
	}
	uint32_t DbiRead() { return DbiReady()?fIO.DbiApertureRead():UINT32_MAX; }
	void DbiWrite(uint32_t value) { if (fWritable && DbiReady()) fIO.DbiApertureWrite(value); }
	bool Finish()
	{
		if (!fDirty) return true;
		if (!fIO.DbiRouteStable()) return false;
		fActive=false;
		for (unsigned i=0;i<3;i++) fIO.DbiTlbWrite(i,fSaved[i]);
		for (unsigned i=0;i<3;i++) if (fIO.DbiTlbRead(i)!=fSaved[i]) return false;
		if (!fIO.DbiRouteStable()) return false;
		fDirty=false;return true;
	}
	bool Dirty() const { return fDirty; }
private:
	IO& fIO;
	uint32_t fSaved[3]={};
	bool fAttempted=false,fActive=false,fWritable=false,fDirty=false;
};

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
