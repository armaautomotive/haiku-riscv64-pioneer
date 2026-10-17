/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQkvDecoderProof.h"
static PioneerDecoderJob Job(unsigned requested)
{
	PioneerDecoderJob j={}; j.magic=0x31444344; j.operation=10; j.version=11; j.tokens=j.nonce=1;
	j.kvBase=0x82400000; j.finalNorm=0x30000000; j.vocabulary=0x40000000;
	j.hostLow=0x80000000; j.hostHigh=1; j.hostBytes=0x100000; j.workers=119;
	j.reserved[0]=requested; j.reserved[1]=0x31000000; j.reserved[2]=151643; j.reserved[3]=5;
	for (auto& l:j.layers) { auto* words=reinterpret_cast<uint32_t*>(&l); for (unsigned i=0;i<7;i++) words[i]=0x04000000+i*0x1000000; for (unsigned i=7;i<11;i++) words[i]=0x30000000; }
	for (auto& s:j.sequence) s=1;
	for (unsigned i=0;i<64;i++) j.rope[2*i]=0x3f800000;
	return j;
}
int main()
{
	for (unsigned requested:{1U,2U}) {
		auto j=Job(requested),original=j; assert(PioneerQkvDecoderJobValid(j) && !memcmp(&j,&original,sizeof(j)));
		for (unsigned old=1;old<=10;old++) assert(!PioneerDecoderJobValid(j,old));
		for (unsigned version=0;version<=13;version++) { auto invalid=j; invalid.version=version; assert(PioneerQkvDecoderJobValid(invalid)==(version==11)); }
		for (unsigned w=0;w<119;w++) {
			unsigned advance=PioneerPairedQkvSequenceAdvance(w)*requested; auto edge=j;
			edge.sequence[w]=0x7fffffff-advance; assert(PioneerQkvDecoderJobValid(edge));
			edge.sequence[w]++; assert(!PioneerQkvDecoderJobValid(edge));
			edge.sequence[w]=0; assert(!PioneerQkvDecoderJobValid(edge));
		}
		for (unsigned layer=0;layer<28;layer++) for (unsigned role=0;role<11;role++) {
			auto invalid=j; reinterpret_cast<uint32_t*>(&invalid.layers[layer])[role]=0; assert(!PioneerQkvDecoderJobValid(invalid));
		}
		for (unsigned operation:{0U,8U,9U,10U,11U,12U}) { auto invalid=j; invalid.operation=operation;
			assert(PioneerQkvDecoderJobValid(invalid)==(operation==10 || (requested==1 && (operation==8 || operation==9)))); }
	}
	for (unsigned mode:{8U,9U,10U}) for (unsigned count:{1U,2U}) {
		if (mode!=10 && count!=1) continue;
		unsigned minimum=mode==8?19904:mode==9?19786:19785*count+64*(count-1)+1;
		unsigned rf=mode==8?70550:mode==9?61056:61054*count+576*(count-1)+4;
		unsigned w=mode==8?9478:mode==9?9360:9359*count+1,wf=mode==8?18855:mode==9?9361:9359*count+4;
		for (unsigned polls:{0U,1U,65535U*8967*count}) {
			unsigned before[8],after[8],s[24]={},r=minimum+2*polls; unsigned delta[]={r,rf+2*polls,r+w,r,w,wf,r+w,w};
			s[0]=r; s[1]=rf+2*polls; s[2]=w; s[3]=wf; s[4]=119; s[5]=151936; s[6]=1; s[7]=mode;
			for (unsigned i=0;i<8;i++) { before[i]=s[8+i]=0xfffffff8; after[i]=s[16+i]=before[i]+delta[i]; }
			assert(PioneerQkvDecoderSummary(s,count,1,before,after,mode));
			for (unsigned word=0;word<24;word++) for (unsigned bit=0;bit<32;bit++) { auto old=s[word]; s[word]^=1U<<bit;
				assert(!PioneerQkvDecoderSummary(s,count,1,before,after,mode)); s[word]=old; }
			assert(!PioneerQkvDecoderSummary(s,3,1,before,after,mode) && !PioneerQkvDecoderSummary(s,count,2,before,after,mode));
		}
		uint64_t maximumRead=uint64_t(minimum)+uint64_t(2)*65535*8967*count;
		assert(maximumRead+w<UINT32_MAX && maximumRead-minimum+rf<UINT32_MAX);
	}
	for (unsigned positions:{1U,2U}) {
		tt_decoder_stage_profile p={}; p.magic=0x31525044; p.nonce=1; p.tokens=positions; p.version=6;
		unsigned calls[]={112*positions,57*positions,85*positions,0,28*positions,28*positions,positions,1};
		for (unsigned i=0;i<8;i++) { p.calls[i]=calls[i]; p.cycles[i]=calls[i]?100:0; } p.cycles[7]=1000; p.reserved[0]=positions;
		assert(PioneerQkvDecoderStageValid(p,1,positions,positions) && !PioneerDecoderStageValid(p,1,positions,5,positions));
		for (unsigned word=0;word<32;word++) { auto invalid=p; auto* field=reinterpret_cast<uint32_t*>(&invalid)+word;
			if (word>=4 && word<12) *field=*field?0:1; else *field^=1;
			assert(!PioneerQkvDecoderStageValid(invalid,1,positions,positions)); }
	}
	puts("PASS OFFLINE private11 strict job/ALL119 sequence boundaries/ALL308 tensor+norm extent refusals/legacy-version refusal/full+small+batch exact counters/max polling/wrapped ALL32 bits of24 words/profile6;NO driver/native authority");
}
