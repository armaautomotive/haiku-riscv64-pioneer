/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GGML_HOST_DEVICE_H
#define PIONEER_GGML_HOST_DEVICE_H
// OFFLINE ONLY: no device header, open, ioctl, mmap or hardware access.
#include "../PioneerResidentTensor.h"
#include "../TenstorrentGgufQuant.h"
#include "../PioneerAttentionMath.h"
#include "../PioneerGraphMath.h"
#include <cassert>
struct PioneerGgmlHostState {
	unsigned opens=0,finishes=0,uploads=0,calls=0,attentionCalls=0,ffnCalls=0,gluCalls=0;
	bool attention=false;
	bool coordinated=false;
	bool wide=false;
	unsigned fleet=0;
	bool fail=false,badMath=false,fp16=false,q8=false,compact=false,native=false,exact=false,dual=false,reuse=false;
	std::vector<uint32_t> bytes;
	PioneerGgmlHostState():bytes(1024*1024/4) {}
};
PioneerGgmlHostState& PioneerHostDeviceState();
class PioneerResidentDevice {
	bool active=false;
	std::vector<uint32_t> residentK[28],residentV[28];
public:
	unsigned peakWorkers=0;
	uint64_t activationReuses=0;
	uint64_t wideCalls=0;
	~PioneerResidentDevice() { Finish(); }
	bool Open(uint64_t capacity,bool fp16=false,bool q8=false,bool compact=false,bool native=false,bool exact=false,bool dual=false,bool reuse=false,bool attention=false,unsigned bulkUpload=0,unsigned fleet=0,bool resident=false,bool blocks=false,bool coordinated=false,bool debug=false,unsigned requireLayer=0) { assert(!debug && requireLayer<=3 && (!requireLayer || coordinated) && capacity==UINT64_C(2)*1024*1024*1024 && !(fp16 && q8) && (!compact || q8) && (!native || compact) && (!exact || native) && (!dual || exact) && (!reuse || dual) && (!attention || reuse) && bulkUpload<=2 && (!bulkUpload || reuse) && fleet<=120 && (!fleet || (reuse && bulkUpload)) && (!resident || (fleet && attention)) && (!blocks || resident) && (!coordinated || (resident && blocks && fleet && fleet<=119))); active=true; PioneerHostDeviceState().opens++; PioneerHostDeviceState().fp16=fp16; PioneerHostDeviceState().q8=q8; PioneerHostDeviceState().compact=compact; PioneerHostDeviceState().native=native; PioneerHostDeviceState().exact=exact; PioneerHostDeviceState().dual=dual; PioneerHostDeviceState().reuse=reuse; PioneerHostDeviceState().attention=attention; PioneerHostDeviceState().fleet=fleet; PioneerHostDeviceState().coordinated=coordinated; PioneerHostDeviceState().wide=requireLayer==3; return true; }
	bool LayerGlu(unsigned n,const uint32_t* gate,const uint32_t* up,uint32_t* output) {
		auto& s=PioneerHostDeviceState(); assert(active && s.coordinated && n==3072); s.gluCalls++;
		if (s.fail) return false;
		for (unsigned i=0;i<n;i++) output[i]=PioneerAttentionBits(PioneerGraphSiluFast(PioneerAttentionFloat(gate[i]))*PioneerAttentionFloat(up[i]));
		return true;
	}
	bool LayerFfn(uint64_t gate,uint64_t up,uint64_t down,const uint32_t* x,const uint32_t* d,uint32_t* output) {
		auto& s=PioneerHostDeviceState(); assert(active && s.coordinated); s.ffnCalls++;
		uint32_t a[3072],b[3072],operand[1536],scales[96];
		if (!Q8Matvec(gate,1024,3072,x,d,a) || !Q8Matvec(up,1024,3072,x,d,b)) return false;
		for (unsigned block=0;block<96;block++) {
			float values[32]; for (unsigned i=0;i<32;i++) values[i]=PioneerGraphSiluFast(PioneerAttentionFloat(a[block*32+i]))*PioneerAttentionFloat(b[block*32+i]);
			if (!PioneerGraphQuantize32(values,operand+block*16,scales[block])) return false;
		}
		return Q8Matvec(down,3072,1024,operand,scales,output);
	}
	bool UploadPages(uint64_t offset,const uint32_t* data,unsigned pages) {
		assert(pages>=1 && pages<=4);
		for (unsigned p=0;p<pages;p++) if (!Upload(offset+p*4096,data+p*1024)) return false;
		return true;
	}
	bool AttentionTile(unsigned n,const uint32_t* q,const uint32_t* k,const uint32_t* v,uint32_t* output) {
		auto& s=PioneerHostDeviceState(); assert(active && s.attention); s.attentionCalls++; if (s.fail) return false;
		float query[256],result[256]; memcpy(query,q,1024);
		std::vector<uint16_t> key(n*128),value(n*128);
		for (unsigned w=0;w<n*64;w++) { key[w*2]=uint16_t(k[w]); key[w*2+1]=uint16_t(k[w]>>16); value[w*2]=uint16_t(v[w]); value[w*2+1]=uint16_t(v[w]>>16); }
		if (!PioneerAttentionHead(query,key.data(),value.data(),n,128,result) || !PioneerAttentionHead(query+128,key.data(),value.data(),n,128,result+128)) return false;
		memcpy(output,result,1024); return true;
	}
	bool AttentionGroups(unsigned n,const uint32_t* q,const uint32_t* k,const uint32_t* v,uint32_t* output) {
		assert(PioneerHostDeviceState().fleet);
		for (unsigned g=0;g<8;g++) if (!AttentionTile(n,q+g*256,k+g*n*64,v+g*n*64,output+g*256)) return false;
		return true;
	}
	bool ResidentAttentionStep(unsigned n,unsigned layer,const uint32_t* q,const uint32_t* k,const uint32_t* v,uint32_t* output) {
		assert(layer<28 && n && n<=256 && n<=residentK[layer].size()/512+1);
		auto& key=residentK[layer]; auto& val=residentV[layer];
		key.resize(n*512); val.resize(n*512);
		memcpy(key.data()+(n-1)*512,k,2048); memcpy(val.data()+(n-1)*512,v,2048);
		std::vector<uint32_t> allK(n*512),allV(n*512);
		for (unsigned g=0;g<8;g++) for (unsigned t=0;t<n;t++) {
			memcpy(allK.data()+(g*n+t)*64,key.data()+t*512+g*64,256);
			memcpy(allV.data()+(g*n+t)*64,val.data()+t*512+g*64,256);
		}
		return AttentionGroups(n,q,allK.data(),allV.data(),output);
	}
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
		if (s.fleet) { unsigned used=s.coordinated?s.fleet:outputs<=6144?2:s.fleet; if (used>outputs/32) used=outputs/32; if (used>peakWorkers) peakWorkers=used; }
		assert(s.fleet || !s.dual || (outputs>=64 && !(outputs&63)));
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
		if (s.wide && outputs>4096) wideCalls++;
		return true;
	}
};
#endif
