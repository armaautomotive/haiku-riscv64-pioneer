/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38OrderedParts.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchProtocol.h"
#include <array>
#include <random>
using Parts=std::array<tt_q38_cohort_part,119>;
static uint64_t Bytes(unsigned type,unsigned k,unsigned groups) {
	return uint64_t(groups)*(k/(type==8?32:256))*(type==8?576:type==12?2304:3392);
}
static bool Oracle(unsigned type,unsigned k,const Parts& p,unsigned n) {
	if (!n || n>119 || (type!=8 && type!=12 && type!=14) || (k!=5120 && k!=6144 && k!=17408)) return false;
	for (unsigned w=0;w<119;w++) {
		const auto& a=p[w];
		if (w>=n) { if (a.bank || a.groups || a.offset) return false; continue; }
		if (a.bank>=7 || !a.groups || a.groups>16 || (a.offset&63) || a.offset>=UINT64_C(3221225472)
			|| Bytes(type,k,a.groups)>UINT64_C(3221225472)-a.offset) return false;
		for (unsigned j=0;j<w;j++) if (a.bank==p[j].bank && a.offset<p[j].offset+Bytes(type,k,p[j].groups)
			&& p[j].offset<a.offset+Bytes(type,k,a.groups)) return false;
	}
	return true;
}
int main()
{
	std::mt19937 random(0x38a119); unsigned cases=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U})
	for (unsigned n:{1U,7U,119U}) for (unsigned groups:{1U,3U,16U}) {
		Parts sorted={}; uint64_t end[7]={};
		for (unsigned w=0;w<n;w++) {
			unsigned bank=w%7; sorted[w]={bank,groups,end[bank]}; end[bank]+=Bytes(type,k,groups);
		}
		unsigned comparisons=0;
		assert(Oracle(type,k,sorted,n) && PioneerQ38OrderedParts(type,k,sorted.data(),n,&comparisons) && !comparisons);
		for (unsigned trial=0;trial<384;trial++) {
			auto parts=sorted; std::shuffle(parts.begin(),parts.begin()+n,random);
			if (trial%4) {
				unsigned at=random()%119,field=random()%9;
				if (!field) parts[at].bank=7; else if (field==1) parts[at].groups=0;
				else if (field==2) parts[at].groups=17; else if (field==3) parts[at].offset++;
				else if (field==4) parts[at].offset=UINT64_MAX-63;
				else if (field==5) parts[at].offset=3221225472;
				else if (field==6) parts[at]=parts[random()%n];
				else if (field==7) parts[at].offset=(random()%2048)*64;
				else parts[at].offset=3221225472-64;
			}
			assert(PioneerQ38OrderedParts(type,k,parts.data(),n)==Oracle(type,k,parts,n)); cases++;
		}
		// An out-of-order disjoint gap is legal;overlap with ANY earlier
		// interval must still be caught,not only the latest worker's part.
		Parts back=sorted; std::reverse(back.begin(),back.begin()+n);
		assert(PioneerQ38OrderedParts(type,k,back.data(),n) && Oracle(type,k,back,n));
		if (n==119) { back[118]=back[0]; assert(!PioneerQ38OrderedParts(type,k,back.data(),n)); }
	}
	Parts empty={}; assert(!PioneerQ38OrderedParts(12,5120,nullptr,1));
	for (unsigned count:{0U,120U,UINT32_MAX}) assert(!PioneerQ38OrderedParts(12,5120,empty.data(),count));
	auto request=std::make_unique<tt_q38_dispatch_probe>(); request->version=1; request->type=12; request->inputs=17408; request->peers=119;
	for (unsigned w=0;w<request->inputs/2;w++) request->activation[w]=uint32_t(PioneerInt8Operand(0))*0x10001;
	for (unsigned w=0;w<119;w++) request->part[w]={w%7,1,uint64_t(w/7)*Bytes(12,17408,1)};
	assert(PioneerQ38DispatchValid(*request)); request->part[118]=request->part[0]; assert(!PioneerQ38DispatchValid(*request));
	printf("PASS logical interval fast path:%u independently-oracled ordered/shuffled/invalid/full7-bank cases,unsorted legal gaps,touching ends,earlier overlaps,inactive fields/overflow/bounds;119 ordered workers need0 pair scans vs7021;NO IO/source/page authority or native speed claim\n",cases);
}
