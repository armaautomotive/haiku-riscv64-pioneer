/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamProtocol.h"
#include "../PioneerNativeCompactQ4StreamLayout.h"
#include <cassert>
#include <cstdio>
#include <set>
#include <vector>
int main() {
	using L=PioneerNativeCompactQ4StreamLayout;using V=PioneerNativeCompactQ4StreamVariant;
	std::set<unsigned> addresses;
	for(unsigned i=0;i<L::L1Words;i++){unsigned a=L::Address(i);assert(L::Contains(a) && addresses.insert(a).second);}
	assert(addresses.size()==26908 && L::Address(26908)==UINT32_MAX && !L::Contains(UINT32_MAX));
	for(unsigned k:{5120U,17408U}) {
		std::set<unsigned> dram;for(unsigned i=0;i<L::DramWords(k);i++){unsigned a=L::DramAddress(k,i);assert(L::DramContains(k,a) && dram.insert(a).second && a<0xff000000);}
		assert(!L::DramContains(k,L::DramAddress(k,L::DramWords(k))));
		std::set<unsigned> expected;unsigned total=0;
		for(unsigned b=0;b<2;b++)for(unsigned packet=0;packet<L::Packets;packet++) {
			L::Read p={};assert(L::Packet(k,k/256-2+b,b,packet,p));
			for(unsigned byte=0;byte<p.bytes;byte+=4) {
				unsigned a=p.destination+byte,source=p.source+byte;
				unsigned dense=source<L::DramInput?(source-L::DramWeight)/4:L::WeightBytes(k)/4+(source-L::DramInput)/4;
				assert(L::Contains(a) && L::Immutable(a) && L::FinalPayloadIndex(k,a)==dense && expected.insert(a).second);total++;
			}
			for(unsigned byte=0;byte<64;byte+=4){unsigned a=p.destination-64+byte;assert(L::Contains(a) && L::Immutable(a) && L::FinalPayloadIndex(k,a)==UINT32_MAX);}
		}
		assert(total==2560);
		for(unsigned a=L::DataBegin;a<L::DataEnd;a+=4)assert((L::FinalPayloadIndex(k,a)!=UINT32_MAX)==bool(expected.count(a)));
	}
	for(unsigned a:{0U,0x5ffcU,0x6004U,0x6dffcU,0x72000U,0x74020U,0x8ffbcU,0xa6400U,0x17ffbcU,0x17ffccU})assert(!L::Contains(a));
	PioneerNativeCompactQ4StreamRequest r={};r.version=1;r.expected_tag=V::Tag;r.expected_generation=9;
	assert(PioneerNativeCompactQ4StreamRequestValid(r));unsigned refusals=0;
	for(unsigned byte=0;byte<sizeof(r.result);byte++){auto b=r;reinterpret_cast<uint8_t*>(&b.result)[byte]=1;assert(!PioneerNativeCompactQ4StreamRequestValid(b));refusals++;}
	for(unsigned tag=0;tag<=0x110;tag++)if(tag!=0x104){auto b=r;b.expected_tag=UINT64_C(0x2026110500010000)|tag;assert(!PioneerNativeCompactQ4StreamRequestValid(b));refusals++;}
	for(uint64_t tag:{UINT64_C(0x2026110400010086),UINT64_C(0x2026110400010087),UINT64_C(0x2026110500010103)}){auto b=r;b.expected_tag=tag;assert(!PioneerNativeCompactQ4StreamRequestValid(b));refusals++;}
	for(unsigned bit=0;bit<32;bit++){auto b=r;b.version^=1U<<bit;assert(!PioneerNativeCompactQ4StreamRequestValid(b));b=r;b.reserved=1U<<bit;assert(!PioneerNativeCompactQ4StreamRequestValid(b));refusals+=2;}
	auto b=r;b.expected_generation=0;assert(!PioneerNativeCompactQ4StreamRequestValid(b));refusals++;
	for(unsigned s=0;s<9;s++)assert(V::Width(s)==(s<6?5120U:17408U) && V::Tensor(s)==(s<3?7U:s<6?8U:6U) && V::Count(s)==(0x04000000|V::Width(s)));
	assert(!V::Width(9) && !V::Count(9) && V::Tensor(9)==UINT32_MAX && V::First(9)==UINT32_MAX);
	printf("PASS diagnostic104 DATA contract:26908 unique L1 words,both widths/exact bounded GDDR sets,2560 final payload words/two slots with no header/gap alias,%u dirty-result/tag/version/reserved/generation refusals;NO native IO/ownership authority\n",refusals);
}
