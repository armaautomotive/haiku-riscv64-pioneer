/* SPDX-License-Identifier: MIT */
// COMPILE-ONLY interface subject. No implementations of kernel IO,device
// entry,FD/mapping/PCI factory,public controls or deployment authority.
#include <Drivers.h>
#include "../PioneerNativeDramModelBandLease.h"
#include "PioneerNativeDramBandLeaseProbeImage.h" // freshly compiled RV32
struct NativeDramBandKernelProbeIO {
	bool NativeScope(const PioneerNativeDramModelBandLease*,const tt_dram_samples_result*,const uint32_t*,unsigned,const uint32_t*,unsigned);
	void NativeRetain();
	bool NativeSourceRead(unsigned,uint32_t&);
	bool NativeTlbRead(unsigned,unsigned,uint32_t&);bool NativeTlbWrite(unsigned,unsigned,uint32_t);
	bool NativeResetRead(uint32_t&);bool NativeResetWrite(uint32_t);
	bool NativeNiuRead(unsigned,uint32_t&);bool NativeNiuWrite(unsigned,uint32_t);
	bool NativeL1Read(unsigned,uint32_t&);bool NativeL1Write(unsigned,uint32_t);
	bool NativeDramRead(unsigned,uint32_t&);bool NativeDramWrite(unsigned,uint32_t);
	bool NativeControlRead(unsigned,uint32_t&);bool NativePcRead(uint32_t&);
	bool NativeClockBegin();bool NativeClockEnd();bool NativeNow(uint64_t&);bool NativePause();
};
bool PioneerNativeDramBandKernelCompileSubject(NativeDramBandKernelProbeIO& io,PioneerNativeDramModelBandLease& lease,
	const tt_dram_samples_result& baseline,unsigned sample) {
	return lease.Run<NativeDramBandKernelProbeIO,PioneerNativeDramBandLeaseProbeImage>(io,baseline,sample)
		&& lease.End<NativeDramBandKernelProbeIO,PioneerNativeDramBandLeaseProbeImage>(io);
}
