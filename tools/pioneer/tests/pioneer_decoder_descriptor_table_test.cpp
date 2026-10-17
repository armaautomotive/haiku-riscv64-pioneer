/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <set>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderDescriptorTable.h"
int main()
{
	PioneerDecoderJob model={}; unsigned next=0x04000000;
	for (auto& l:model.layers) for (unsigned role=0;role<7;role++) {
		reinterpret_cast<uint32_t*>(&l)[role]=next; next+=0x400000;
	}
	model.vocabulary=next;
	unsigned jobs=0,matrix=0,attention=0,glu=0;
	std::set<unsigned> q,k,v,o,gate,up,down,vocab;
	for (unsigned worker=1;worker<=119;worker++) {
		unsigned count=PioneerDecoderDescriptorCount(worker); assert(count<=225);
		assert(TT_DECODER_TABLE_BASE+count*32<=0x88000);
		unsigned expected=worker<=6?225:worker<=8?197:worker<=32?169:worker<=64?57:29;
		assert(count==expected); jobs+=count;
		for (unsigned index=0;index<count;index++) {
			PioneerDecoderDescriptor e; assert(PioneerDecoderDescriptorEntry(model,worker,index,e));
			assert(e.words[3]==1 && e.words[4]==PioneerDecoderDescriptorXY(worker) && !e.words[5] && !e.words[6]);
			if (e.words[7]) { assert(worker<=8); if (e.words[7]==0x31475441) attention++; else { assert(worker<=6 && e.words[7]==0x314c4741); glu++; } continue; }
			matrix++; unsigned rows=e.words[2]*16,kdim=e.words[1]*16;
			assert(rows>=32 && rows<=1280 && (rows&31)==0 && 0x84000+rows*4<=TT_DECODER_TABLE_BASE);
			// Independent complete first-layer row coverage,including paired
			// gate/up boundaries and the wide tied-embedding classifier.
			std::set<unsigned>* coverage=nullptr; unsigned base=0;
			const auto& l=model.layers[0];
			std::set<unsigned>* sets[]={&q,&v,&k,&o,&gate,&up,&down};
			for (unsigned role=0;role<7;role++) { unsigned b=reinterpret_cast<const uint32_t*>(&l)[role];
				if (e.words[0]>=b && e.words[0]<b+0x400000) { coverage=sets[role]; base=b; } }
			if (e.words[0]>=model.vocabulary) { coverage=&vocab; base=model.vocabulary; }
			if (coverage) { unsigned row=(e.words[0]-base)/(kdim+kdim/8);
				for (unsigned r=row;r<row+rows;r++) assert(coverage->insert(r).second); }
		}
		PioneerDecoderDescriptor invalid; assert(!PioneerDecoderDescriptorEntry(model,worker,count,invalid));
	}
	assert(jobs==9219 && matrix==8827 && attention==224 && glu==168);
	const std::set<unsigned>* sets[]={&q,&v,&k,&o,&gate,&up,&down,&vocab};
	const unsigned sizes[]={2048,1024,1024,1024,3072,3072,1024,151936};
	for (unsigned i=0;i<8;i++) { assert(sets[i]->size()==sizes[i] && *sets[i]->begin()==0 && *sets[i]->rbegin()==sizes[i]-1); }
	assert(!PioneerDecoderDescriptorCount(0) && !PioneerDecoderDescriptorCount(120));
	puts("PASS OFFLINE fixed119-peer/9219-job descriptor schedule,all row coverage/paired boundaries/225-entry L1 bound;NO device exposure");
}
