/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_HOST_STAGE_H
#define PIONEER_RUNTIME_HOST_STAGE_H
#include <stdint.h>
// PRIVATE ordinary noncacheable HOST RAM only. Never MMIO or card L1.
// Full readback is retained. Independent words need boundary ordering,
// not two global I/O fences around every ordinary host-RAM access.
template<class IO>
bool PioneerRuntimeHostStageBlock(IO& io,const uint32_t* data) {
	if (!data) return false;
	io.RuntimeHostStageFence();
	for (unsigned w=0;w<1024;w++) io.RuntimeHostStageStore(w,data[w]);
	io.RuntimeHostStageFence(); bool valid=true;
	for (unsigned w=0;w<1024;w++) valid &= io.RuntimeHostStageLoad(w)==data[w];
	io.RuntimeHostStageFence(); return valid;
}
template<class IO>
auto PioneerRuntimeHostStage(IO& io,const uint32_t* data,int)
	-> decltype(io.RuntimeHostFillVerify(data),bool()) {
	return io.RuntimeHostFillVerify(data);
}
template<class IO>
bool PioneerRuntimeHostStage(IO& io,const uint32_t* data,long) {
	if (!data) return false;
	for (unsigned w=0;w<1024;w++) io.HostPageWrite(w,data[w]);
	for (unsigned w=0;w<1024;w++) if (io.HostPageRead(w)!=data[w]) return false;
	return true;
}
#endif
