/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerNativeMixedFpuStreamProtocol.h"
#include "../PioneerNativeMixedFpuStreamLayout.h"
#include <cassert>
#include <cstdio>
#include <set>
int main() {
	using L=PioneerNativeMixedFpuStreamLayout;using V=PioneerNativeMixedFpuStreamVariant;
	const unsigned types[17]={14,14,14,14,14,14,8,8,8,8,8,8,8,8,8,8,8};
	const unsigned tensors[17]={48,48,48,0,0,0,5,5,5,16,16,16,49,49,49,11,11};
	const unsigned widths[17]={6144,6144,6144,5120,5120,5120,5120,5120,5120,6144,6144,6144,5120,5120,5120,5120,5120};
	const unsigned first[17]={0,2560,5088,0,124160,248288,0,5120,10208,0,2560,5088,0,6144,12256,0,32};
	const unsigned outputs[17]={5120,5120,5120,248320,248320,248320,10240,10240,10240,5120,5120,5120,12288,12288,12288,48,48};
	std::set<unsigned> addresses;unsigned maximum=0,payloadWords=0;
	for(unsigned i=0;i<L::L1Words;i++){unsigned a=L::Address(i);assert(L::Contains(a) && addresses.insert(a).second);}
	assert(addresses.size()==26908 && L::Address(26908)==UINT32_MAX && !L::Contains(UINT32_MAX));
	for(unsigned s=0;s<17;s++) {
		assert(V::Type(s)==types[s] && V::Tensor(s)==tensors[s] && V::Width(s)==widths[s] && V::First(s)==first[s] && V::Outputs(s)==outputs[s]);
		assert(V::RealRows(s)==(s==16?16U:32U) && first[s]+V::RealRows(s)<=outputs[s]);
		assert(V::Count(s)==((types[s]==14?0x06000000U:0x08000000U)|widths[s]));
		assert(V::Marker(s)==(types[s]==14?0x31533651U:0x31533851U) && V::Fault(s)==(types[s]==14?0x45533651U:0x45533851U));
		assert(V::CodeBytes(s)==(types[s]==14?7848U:6812U));
		std::set<unsigned> dram;
		for(unsigned i=0;i<L::DramWords(s);i++){unsigned a=L::DramAddress(s,i);assert(L::DramContains(s,a) && dram.insert(a).second && a<0xff000000);}
		assert(!L::DramContains(s,L::DramAddress(s,L::DramWords(s))));if(dram.size()>maximum)maximum=unsigned(dram.size());
		std::set<unsigned> expected;unsigned total=0;
		for(unsigned b=0;b<2;b++)for(unsigned packet=0;packet<V::Packets(s);packet++) {
			L::Read p={};assert(L::Packet(s,widths[s]/256-2+b,b,packet,p));
			for(unsigned byte=0;byte<p.bytes;byte+=4) {
				unsigned a=p.destination+byte,source=p.source+byte;
				unsigned dense=source<L::DramInput?(source-L::DramWeight)/4:L::WeightBytes(s)/4+(source-L::DramInput)/4;
				assert(L::Contains(a) && L::Immutable(a) && L::FinalPayloadIndex(s,a)==dense && expected.insert(a).second);total++;
			}
			for(unsigned byte=0;byte<64;byte+=4){unsigned a=p.destination-64+byte;assert(L::Contains(a) && L::Immutable(a) && L::FinalPayloadIndex(s,a)==UINT32_MAX);}
		}
		assert(total==(types[s]==14?4352U:4864U));payloadWords+=total;
		for(unsigned a=L::DataBegin;a<L::DataEnd;a+=4)assert((L::FinalPayloadIndex(s,a)!=UINT32_MAX)==bool(expected.count(a)));
		for(unsigned a:{L::Raw+1,L::Metadata+2,L::Input+3,0x17ffc0U,0x6e000U,L::Output,L::Zero})assert(L::FinalPayloadIndex(s,a)==UINT32_MAX);
	}
	assert(maximum==L::MaxDramWords && maximum*4==233472 && payloadWords==79616);
	for(unsigned a:{0U,0x5ffcU,0x6004U,0x6dffcU,0x72000U,0x74020U,0x8ffbcU,0xa6400U,0x17ffbcU,0x17ffccU})assert(!L::Contains(a));
	for(unsigned s:{17U,18U,~0U}) {
		assert(!V::Type(s) && !V::Width(s) && !V::RealRows(s) && !V::Count(s) && !V::Marker(s) && !V::PreparedBytes(s) && !V::Packets(s) && !V::Flits(s));
		assert(V::Tensor(s)==UINT32_MAX && V::First(s)==UINT32_MAX && !L::DramWords(s) && !L::DramContains(s,L::DramWeight));
		L::Read r={1,2,3};assert(!L::Packet(s,0,0,0,r) && r.source==1 && r.destination==2 && r.bytes==3);
	}
	PioneerNativeMixedFpuStreamRequest r={};r.version=1;r.expected_tag=V::Tag;r.expected_generation=11;
	assert(PioneerNativeMixedFpuStreamRequestValid(r));unsigned refused=0;
	for(unsigned b=0;b<sizeof(r.result);b++){auto bad=r;reinterpret_cast<uint8_t*>(&bad.result)[b]=1;assert(!PioneerNativeMixedFpuStreamRequestValid(bad));refused++;}
	for(unsigned tag=0;tag<=0x90;tag++)if(tag!=0x87){auto bad=r;bad.expected_tag=UINT64_C(0x2026110400010000)|tag;assert(!PioneerNativeMixedFpuStreamRequestValid(bad));refused++;}
	for(unsigned bit=0;bit<32;bit++){auto bad=r;bad.version^=1U<<bit;assert(!PioneerNativeMixedFpuStreamRequestValid(bad));bad=r;bad.reserved=1U<<bit;assert(!PioneerNativeMixedFpuStreamRequestValid(bad));refused+=2;}
	auto bad=r;bad.expected_generation=0;assert(!PioneerNativeMixedFpuStreamRequestValid(bad));refused++;
	printf("PASS diagnostic87 DATA contract:17 fixed Q6/Q8 bands/explicit16-row tail,26908 unique L1 words/exact sample GDDR domains,max233472B,both final producer slots/79616 total payload words/no header-gap alias,%u dirty-result/tag/version/reserved/generation refusals;NO native IO/ownership authority\n",refused);
}
