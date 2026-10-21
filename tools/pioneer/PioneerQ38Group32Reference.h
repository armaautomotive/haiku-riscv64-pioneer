/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GROUP32_REFERENCE_H
#define PIONEER_Q38_GROUP32_REFERENCE_H
#ifndef PIONEER_Q38_GROUP32_REFERENCE_CLIENT
#error Group32 CPU reference belongs ONLY in the independent qualification client
#endif
#include "PioneerQwen38Panel.h"
#include <vector>
// Read-only DEBUG oracle:original packed tiles and activations,not shader
// instructions/results. No device reads/IO/CPU fallback or model selector.
template<class R> static bool PioneerQ38Group32Reference(const R& r,const std::vector<uint8_t>& packed,unsigned worker,float* output) {
	if(!output || r.type!=12 || worker>=r.peers || (r.inputs!=5120 && r.inputs!=6144 && r.inputs!=17408))return false;
	unsigned groups=r.part[worker].groups,blocks=r.inputs/256;
	if(!groups || groups>16 || packed.size()!=size_t(groups)*blocks*2304)return false;
	float stage[256]={};
	for(unsigned g=0;g<groups;g++)for(unsigned b=0;b<blocks;b++)for(unsigned p=0;p<16;p+=2) {
		PioneerQ38Panel first,second;
		const uint8_t* tile=packed.data()+(g*blocks+b)*2304;
		if(!PioneerQ38UnpackPanel(12,tile,2304,p,&first) || !PioneerQ38UnpackPanel(12,tile,2304,p+1,&second))return false;
		int sum=0,dot[16]={};
		for(unsigned k=0;k<32;k++) {
			unsigned at=b*256+p*16+k;uint16_t encoded=uint16_t(r.activation[at/2]>>((at%2)*16));
			if(!PioneerInt8OperandValid(encoded))return false;
			int x=int(encoded&1023)*((encoded&0x8000)?-1:1);sum+=x;
			const auto& panel=k<16?first:second;
			for(unsigned row=0;row<16;row++) {unsigned e=(k%16)*16+row;uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16));
				dot[row]+=int(w&1023)*((w&0x8000)?-1:1)*x;}
		}
		float scale;uint32_t raw=r.scales[b];__builtin_memcpy(&scale,&raw,4);
		unsigned magnitude=raw&0x7fffffff;if(magnitude && (magnitude<0x2f800000 || magnitude>0x47800000))return false;
		for(unsigned row=0;row<16;row++) {
			if(first.coefficient[row]!=second.coefficient[row] || first.minimum[row]!=second.minimum[row])return false;
			float coefficient,minimum;__builtin_memcpy(&coefficient,&first.coefficient[row],4);__builtin_memcpy(&minimum,&first.minimum[row],4);
			volatile float c=coefficient*scale,m=minimum*scale,product=c*float(dot[row]),correction=m*float(sum),value=product+correction;
			stage[g*16+row]+=value;
		}
	}
	for(unsigned row=0;row<256;row++)output[row]=stage[row];
	return true;
}
#endif
