/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerNativeQ4FpuProtocol.h"
#include "../PioneerNativeQ4FpuLayout.h"
#include "../PioneerQ38Q4FpuBlock.h"
#include <set>
#include <cassert>
#include <cstdio>
int main()
{
	using L=PioneerNativeQ4FpuLayout;using B=PioneerQ38Q4FpuBlock;
	static_assert(L::Raw==B::Raw && L::RawStride==B::RawStride && L::Metadata==B::Metadata && L::MetadataStride==B::MetadataStride
		&& L::Input==B::Input && L::InputStride==B::InputStride && L::Zero==B::Zero && L::Output==B::Output,"same actual pipeline geometry");
	std::set<unsigned> addresses;bool payload[6144]={},input[512]={};unsigned p=0,b=0;
	for(unsigned i=0;i<L::L1Words;i++){unsigned a=L::Address(i);assert(L::Contains(a) && addresses.insert(a).second);}
	assert(addresses.size()==12372 && L::Address(12372)==UINT32_MAX && !L::Contains(UINT32_MAX));
	for(unsigned a=L::DataBegin;a<L::DataEnd;a++) {
		unsigned at=L::PreparedIndex(a);if(at!=UINT32_MAX){assert(at<6144 && !payload[at]);payload[at]=true;p++;}
		at=L::InputIndex(a);if(at!=UINT32_MAX){assert(at<512 && !input[at]);input[at]=true;b++;}
	}
	assert(p==6144 && b==512);for(bool v:payload)assert(v);for(bool v:input)assert(v);
	for(unsigned a:{0U,0x5ffcU,0x6004U,0x6dffcU,0x72000U,0x8ffbcU,0x98100U,0x17ffbcU,0x17ffccU})assert(!L::Contains(a));
	PioneerNativeQ4FpuRequest r={};r.version=1;r.expected_tag=PioneerNativeQ4FpuVariant::Tag;r.expected_generation=6;
	assert(PioneerNativeQ4FpuRequestValid(r));unsigned refusals=0;
	for(unsigned byte=0;byte<sizeof(r.result);byte++) {
		auto bad=r;reinterpret_cast<uint8_t*>(&bad.result)[byte]=1;assert(!PioneerNativeQ4FpuRequestValid(bad));refusals++;
	}
	for(unsigned role:{0x00U,0x78U,0x79U,0x80U,0x81U,0x82U,0x83U,0x84U,0x86U}){auto bad=r;bad.expected_tag=UINT64_C(0x2026110400010000)|role;assert(!PioneerNativeQ4FpuRequestValid(bad));refusals++;}
	for(unsigned bit=0;bit<32;bit++){auto bad=r;bad.version^=1U<<bit;assert(!PioneerNativeQ4FpuRequestValid(bad));bad=r;bad.reserved=1U<<bit;assert(!PioneerNativeQ4FpuRequestValid(bad));refusals+=2;}
	auto bad=r;bad.expected_generation=0;assert(!PioneerNativeQ4FpuRequestValid(bad));refusals++;
	printf("PASS diagnostic85 DATA contract:12372 unique journal words,all6144 prepared+512 input bytes exactly once,no header/gap aliases,%u dirty-result/tag/version/reserved/generation refusals;NO IO/native ownership authority\n",refusals);
}
