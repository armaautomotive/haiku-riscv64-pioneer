/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GGML_HOST_DEVICE_H
#define PIONEER_GGML_HOST_DEVICE_H
// OFFLINE ONLY: no device header, open, ioctl, mmap or hardware access.
#include "../PioneerResidentTensor.h"
#include "../TenstorrentGgufQuant.h"
#include <cassert>
struct PioneerGgmlHostState {
	unsigned opens=0,finishes=0,uploads=0,calls=0;
	bool fail=false,badMath=false,fp16=false,q8=false,compact=false,native=false,exact=false,dual=false;
	std::vector<uint32_t> bytes;
	PioneerGgmlHostState():bytes(1024*1024/4) {}
};
PioneerGgmlHostState& PioneerHostDeviceState();
class PioneerResidentDevice {
	bool active=false;
public:
	~PioneerResidentDevice() { Finish(); }
	bool Open(uint64_t capacity,bool fp16=false,bool q8=false,bool compact=false,bool native=false,bool exact=false,bool dual=false) { assert(capacity==UINT64_C(2)*1024*1024*1024 && !(fp16 && q8) && (!compact || q8) && (!native || compact) && (!exact || native) && (!dual || exact)); active=true; PioneerHostDeviceState().opens++; PioneerHostDeviceState().fp16=fp16; PioneerHostDeviceState().q8=q8; PioneerHostDeviceState().compact=compact; PioneerHostDeviceState().native=native; PioneerHostDeviceState().exact=exact; PioneerHostDeviceState().dual=dual; return true; }
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
	bool Q8Matvec(uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* x,const uint32_t* scales,uint32_t* output) {
		auto& s=PioneerHostDeviceState(); assert(active && s.q8 && !s.fp16); s.calls++; if (s.fail) return false;
		assert(!s.dual || (outputs>=64 && !(outputs&63)));
		for (unsigned row=0;row<outputs;row++) {
			float sum=0;
			for (unsigned block=0;block<inputs/32;block++) {
				int dot=0;
				for (unsigned i=0;i<32;i++) {
					unsigned k=block*32+i; uint64_t p=offset+(s.exact?PioneerQ8NativeF32ValueByte(inputs,k,row):s.compact?((uint64_t(row/16)*(inputs/32)+block)*576+32+(k%32)*16+row%16):((uint64_t(row/16)*(inputs/16)+k/16)*576+16+(k%16)*32+(row%16)*2));
					uint16_t a=uint16_t(s.bytes[size_t(p/4)]>>(8*(p&3))),b=uint16_t(x[k/2]>>(16*(k&1)));
					if (s.compact) { int q=s.native?int(a&127)*((a&128)?-1:1):int(a&255); if (!s.native && q>=128) q-=256; a=PioneerInt8Operand(q); }
					assert(PioneerInt8OperandValid(a) && PioneerInt8OperandValid(b));
					dot+=int(a&1023)*((a&0x8000)?-1:1)*int(b&1023)*((b&0x8000)?-1:1);
				}
				uint64_t p=offset+(s.exact?PioneerQ8NativeF32ScaleByte(inputs,block,row):s.compact?((uint64_t(row/16)*(inputs/32)+block)*576+(row%16)*2):((uint64_t(row/16)*(inputs/16)+block*2+1)*576+544+(row%16)*2));
				float dw=PioneerHalfFloat(uint16_t(s.bytes[size_t(p/4)]>>(8*(p&3)))),dx; memcpy(&dx,scales+block,4);
				if (s.exact) memcpy(&dw,s.bytes.data()+p/4,4);
				if (s.exact) sum=fmaf(float(dot),dw*dx,sum); else sum+=float(dot)*(dw*dx);
			}
			if (s.badMath) sum+=1;
			memcpy(output+row,&sum,4);
		}
		return true;
	}
};
#endif
