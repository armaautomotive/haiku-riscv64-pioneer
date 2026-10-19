/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38CpuValidation.h"
#include <cassert>
#include <cstdio>
struct ReferenceTensor { unsigned type,inputs,outputs; };
struct ReferenceModel { std::vector<ReferenceTensor> tensors; const auto& Tensors() const { return tensors; } };
struct ReferenceSource {
	ReferenceModel model; std::vector<uint8_t> bytes; bool ready=true,readOkay=true; unsigned reads=0;
	bool Ready() const { return ready; }
	const auto& Model() const { return model; }
	bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned count,void* out,unsigned capacity) {
		reads++; if (!readOkay || tensor || capacity<count || offset>bytes.size() || count>bytes.size()-offset) return false;
		memcpy(out,bytes.data()+offset,count); return true;
	}
};
int main()
{
	for (unsigned k:{5120U,6144U,17408U}) for (unsigned type:{8U,12U,14U}) {
		unsigned n=32,block=type==8?32:256,size=type==8?34:type==12?144:210,blocks=k/block;
		ReferenceSource s; s.model.tensors.push_back({type,k,n}); s.bytes.resize(size_t(n)*blocks*size,0);
		std::vector<float> x(k),expected(n,0);
		for (unsigned i=0;i<k;i++) x[i]=float(int(i%251)-125)/128;
		PioneerQ38CohortInput input; assert(PioneerQ38CohortQuantize(type,k,x.data(),input));
		for (unsigned r=0;r<n;r++) for (unsigned b=0;b<blocks;b++) {
			auto* raw=s.bytes.data()+(size_t(r)*blocks+b)*size;
			unsigned half=type==14?208:0; raw[half]=0; raw[half+1]=0x3c;
			if (type==12) { raw[3]=0x34; for (unsigned g=0;g<4;g++) { raw[4+g]=1; raw[8+g]=1; } for (unsigned g=4;g<8;g++) raw[8+g]=0x11; }
			if (type==14) for (unsigned p=0;p<16;p++) raw[192+p]=1;
			for (unsigned i=0;i<block;i++) {
				int weight=int((i*7+r*3+b*11)%(type==8?255:type==12?16:64))-(type==8?128:type==12?0:32);
				if (type==8) raw[2+i]=uint8_t(weight);
				else if (type==12) raw[16+(i/64)*32+i%32]|=uint8_t(weight)<<((i/32%2)*4);
				else { unsigned value=unsigned(weight+32),plane=i%128/32;
					raw[(i/128)*64+(plane%2)*32+i%32]|=uint8_t(value&15)<<((plane/2)*4);
					raw[128+(i/128)*32+i%32]|=uint8_t(value>>4)<<(plane*2); }
			}
			// Independent scalar decoder and dot order,not UnpackPanel.
			for (unsigned p=0;p<block/16;p++) {
				int dot=0,sum=0;
				for (unsigned lane=0;lane<16;lane++) {
					unsigned i=p*16+lane,index=b*block+i;
					int weight=int((i*7+r*3+b*11)%(type==8?255:type==12?16:64))-(type==8?128:type==12?0:32);
					uint16_t a=uint16_t(input.activation[index/2]>>((index%2)*16));
					int activation=int(a&1023)*((a&0x8000)?-1:1); dot+=weight*activation; sum+=activation;
				}
				float scale=PioneerAttentionFloat(input.scales[b]);
				expected[r]+=scale*float(dot)+(type==12?(-.25f*scale)*float(sum):0.0f);
			}
		}
		assert(PioneerQ38CpuValidate(s,0,x.data(),k,expected.data(),n) && s.reads==2);
		auto before=expected; expected[17]+=1;
		assert(!PioneerQ38CpuValidate(s,0,x.data(),k,expected.data(),n) && expected!=before);
		expected=before; s.bytes.back()^=1;
		assert(!PioneerQ38CpuValidate(s,0,x.data(),k,expected.data(),n)); s.bytes.back()^=1;
		s.readOkay=false; assert(!PioneerQ38CpuValidate(s,0,x.data(),k,expected.data(),n)); s.readOkay=true;
		assert(!PioneerQ38CpuValidate(s,1,x.data(),k,expected.data(),n));
		assert(!PioneerQ38CpuValidate(s,0,x.data(),k,expected.data(),n-1));
		s.ready=false; assert(!PioneerQ38CpuValidate(s,0,x.data(),k,expected.data(),n));
	}
	puts("PASS CPU validation:independent original Q8/Q4_K/Q6_K rows,all32 outputs/nonuniform K5120/6144/17408 inputs,affine minimum,bitwise comparison,source/read/shape/numeric faults rejected,caller output NEVER substituted;NO DEVICE IO");
}
