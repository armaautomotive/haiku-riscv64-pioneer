/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderBatch.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
int main()
{
	// Exhaust every possible outlier,including token0 and the final token.
	for (unsigned extra=0;extra<151936;extra++) {
		unsigned ids[3]={extra,151643,151645};
		if (extra==151643 || extra==151645) continue;
		if (ids[0]>ids[1]) { unsigned swap=ids[0]; ids[0]=ids[1]; ids[1]=swap; }
		if (ids[1]>ids[2]) { unsigned swap=ids[1]; ids[1]=ids[2]; ids[2]=swap; }
		unsigned word0=0; uint32_t word1=0;
		assert(PioneerDecoderPackEog(ids,3,word0,word1) && PioneerDecoderEogSetValid(word0,word1));
		for (unsigned id:ids) assert(PioneerDecoderIsEog(id,word0,word1));
		for (unsigned token:{0U,1U,128247U,151642U,151643U,151644U,151645U,151670U,151935U,151936U,UINT32_MAX})
			assert(PioneerDecoderIsEog(token,word0,word1)==(token==ids[0] || token==ids[1] || token==ids[2]));
	}
	for (unsigned base=151909;base<151936;base++) for (unsigned bit=0;bit<28;bit++) {
		assert(PioneerDecoderEogSetValid(base,1U|(1U<<bit))==(base+bit<151936));
	}
	{ unsigned word0=42; uint32_t word1=42; const unsigned tooWide[]={0,100,200},duplicate[]={0,0},reverse[]={1,0},outside[]={151936};
		for (auto ids:{tooWide,duplicate,reverse,outside}) {
			unsigned count=ids==tooWide?3:ids==outside?1:2;
			assert(!PioneerDecoderPackEog(ids,count,word0,word1) && !word0 && !word1);
		}
		assert(!PioneerDecoderPackEog(nullptr,1,word0,word1) && !PioneerDecoderPackEog(outside,0,word0,word1));
		unsigned actual[]={128247,151643,151645,151662,151663,151664};
		assert(PioneerDecoderPackEog(actual,6,word0,word1));
		for (unsigned token=0;token<151936;token++) {
			bool expected=false; for (unsigned id:actual) expected|=token==id;
			assert(PioneerDecoderIsEog(token,word0,word1)==expected);
		}
	}
	uint32_t sequence[119]={}; PioneerDecoderBatchPlan plan;
	for (unsigned first=1;first<=256;first++) for (unsigned steps=0;steps<=3;steps++) {
		bool valid=steps && steps<=2 && first+steps<=257;
		assert(PioneerDecoderPlanBatch(first,steps,1,sequence,plan)==valid);
		if (valid) for (unsigned w=0;w<119;w++) assert(plan.firstSequence[w]==1 && plan.lastSequence[w]==PioneerDecoderSequenceAdvance(w,true,true,true)*steps && plan.lastLimit==first+steps-1);
	}
	for (unsigned steps:{1U,2U}) for (unsigned w=0;w<119;w++) {
		sequence[w]=0x7ffffffe - PioneerDecoderSequenceAdvance(w,true,true,true)*steps;
		assert(PioneerDecoderPlanBatch(1,steps,0x7ffffffe,sequence,plan) && plan.lastSequence[w]==0x7ffffffe);
		sequence[w]++; assert(!PioneerDecoderPlanBatch(1,steps,1,sequence,plan) && !plan.nonce); sequence[w]=0;
	}
	for (bool fused:{false,true}) for (unsigned completed:{1U,2U}) for (unsigned polls:{0U,2U,131070U}) {
		unsigned r=(29116-(fused?8827:0))*completed+64*(completed-1)+1+polls,rf=61558*completed+576*(completed-1)+4+polls;
		unsigned w=9611*completed+1,wf=9611*completed+4;
		uint32_t s[24]={r,rf,w,wf,119,151936,17,10},before[8],after[8]; const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		for (unsigned i=0;i<8;i++) { before[i]=s[8+i]=UINT32_MAX-7; after[i]=s[16+i]=before[i]+delta[i]; }
		assert(PioneerDecoderBatchSummary(s,completed,17,before,after,fused));
		if (fused) assert(!PioneerDecoderBatchSummary(s,completed,17,before,after));
		for (unsigned i=0;i<24;i++) { s[i]^=1; assert(!PioneerDecoderBatchSummary(s,completed,17,before,after,fused)); s[i]^=1; }
		assert(uint64_t(29116*completed+64*(completed-1)+1)+uint64_t(2)*65535*9219*completed+w<UINT32_MAX);
	}
	for (bool small:{false,true}) for (bool fused:{false,true}) {
		unsigned r=29117+(small?0:118)-(fused?8827:0),rf=61560+(small?0:9494),w=9612+(small?0:118),wf=9613+(small?0:9494);
		uint32_t s[24]={r,rf,w,wf,119,151936,17,small?9U:8U},before[8],after[8]; const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		for (unsigned i=0;i<8;i++) { before[i]=s[8+i]=UINT32_MAX-7; after[i]=s[16+i]=before[i]+delta[i]; }
		assert(PioneerDecoderSummary(s,1,17,before,after,true,true,small,true,true,false,true,false,true,fused));
		for (unsigned i=0;i<24;i++) { s[i]^=1; assert(!PioneerDecoderSummary(s,1,17,before,after,true,true,small,true,true,false,true,false,true,fused)); s[i]^=1; }
	}
	uint32_t packet[64]={TT_DECODER_BATCH_MAGIC,17,151936,2,254,255,119,10}; packet[8]=42; packet[9]=43; packet[24]=0x3f800000; packet[25]=0xbf800000; unsigned completed=0;
	assert(PioneerDecoderBatchPacket(packet,17,254,2,151643,5,completed) && completed==2);
	for (unsigned i=0;i<64;i++) if (i<8 || (i>=10 && i<24) || i>=26) { packet[i]^=1; assert(!PioneerDecoderBatchPacket(packet,17,254,2,151643,5,completed) && !completed); packet[i]^=1; }
	packet[8]=151645; assert(!PioneerDecoderBatchPacket(packet,17,254,2,151643,5,completed));
	packet[3]=1; packet[5]=254; packet[9]=packet[25]=0; assert(PioneerDecoderBatchPacket(packet,17,254,2,151643,5,completed) && completed==1);
	packet[8]=42; assert(!PioneerDecoderBatchPacket(packet,17,254,2,151643,5,completed));
	packet[3]=2; packet[5]=255; packet[9]=151645; packet[25]=0x7f800000; assert(!PioneerDecoderBatchPacket(packet,17,254,2,151643,5,completed));
	puts("PASS OFFLINE two-position autonomous plan,all context/119-peer sequence overflow bounds,wrapped exact NoC/host256B counts,all summary words,complete token/logit/EOS/unused-zero packet rules;NO native/driver operation");
}
