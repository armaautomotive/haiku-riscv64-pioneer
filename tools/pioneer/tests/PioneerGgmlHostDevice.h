/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GGML_HOST_DEVICE_H
#define PIONEER_GGML_HOST_DEVICE_H
// OFFLINE ONLY: no device header, open, ioctl, mmap or hardware access.
#include "../PioneerResidentTensor.h"
#include <cassert>
struct PioneerGgmlHostState {
	unsigned opens=0,finishes=0,uploads=0,calls=0;
	bool fail=false,badMath=false,fp16=false;
	std::vector<uint32_t> bytes;
	PioneerGgmlHostState():bytes(1024*1024/4) {}
};
PioneerGgmlHostState& PioneerHostDeviceState();
class PioneerResidentDevice {
	bool active=false;
public:
	~PioneerResidentDevice() { Finish(); }
	bool Open(uint64_t capacity,bool fp16=false) { assert(capacity==UINT64_C(2)*1024*1024*1024); active=true; PioneerHostDeviceState().opens++; PioneerHostDeviceState().fp16=fp16; return true; }
	bool Finish() { if (active) PioneerHostDeviceState().finishes++; active=false; return true; }
	bool Upload(uint64_t offset,const uint32_t* data) {
		auto& s=PioneerHostDeviceState(); assert(active && !(offset%4096) && offset+4096<=s.bytes.size()*4);
		s.uploads++; memcpy(s.bytes.data()+offset/4,data,4096); return true;
	}
	bool Matvec(uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* x,uint32_t* output) {
		auto& s=PioneerHostDeviceState(); assert(active); s.calls++; if (s.fail) return false;
		for (unsigned row=0;row<outputs;row++) {
			float sum=0;
			for (unsigned k=0;k<inputs;k++) {
				uint64_t p=offset+PioneerMatvecWeightElement(inputs,k,row);
				uint32_t w=((s.bytes[size_t(p/4)]>>(8*(p&3)))&65535)<<16,v=((x[k/2]>>(16*(k&1)))&65535)<<16;
				float a,b; memcpy(&a,&w,4); memcpy(&b,&v,4);
				if (s.fp16) {
					a=PioneerFloat16Value(uint16_t(w>>16)&~1U); b=PioneerFloat16Value(uint16_t(v>>16));
				}
				sum+=a*b;
			}
			if (s.badMath) sum+=1;
			memcpy(output+row,&sum,4);
		}
		return true;
	}
};
#endif
