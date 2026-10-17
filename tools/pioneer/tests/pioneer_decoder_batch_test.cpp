/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderBatch.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
int main()
{
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
	for (unsigned completed:{1U,2U}) for (unsigned polls:{0U,2U,131070U}) {
		unsigned r=29116*completed+64*(completed-1)+1+polls,rf=61558*completed+576*(completed-1)+4+polls;
		unsigned w=9611*completed+1,wf=9611*completed+4;
		uint32_t s[24]={r,rf,w,wf,119,151936,17,10},before[8],after[8]; const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		for (unsigned i=0;i<8;i++) { before[i]=s[8+i]=UINT32_MAX-7; after[i]=s[16+i]=before[i]+delta[i]; }
		assert(PioneerDecoderBatchSummary(s,completed,17,before,after));
		for (unsigned i=0;i<24;i++) { s[i]^=1; assert(!PioneerDecoderBatchSummary(s,completed,17,before,after)); s[i]^=1; }
		assert(uint64_t(29116*completed+64*(completed-1)+1)+uint64_t(2)*65535*9219*completed+w<UINT32_MAX);
	}
	uint32_t packet[64]={TT_DECODER_BATCH_MAGIC,17,151936,2,254,255,119,10}; packet[8]=42; packet[9]=43; packet[24]=0x3f800000; packet[25]=0xbf800000; unsigned completed=0;
	assert(PioneerDecoderBatchPacket(packet,17,254,2,151643,151645,completed) && completed==2);
	for (unsigned i=0;i<64;i++) if (i<8 || (i>=10 && i<24) || i>=26) { packet[i]^=1; assert(!PioneerDecoderBatchPacket(packet,17,254,2,151643,151645,completed) && !completed); packet[i]^=1; }
	packet[8]=151645; assert(!PioneerDecoderBatchPacket(packet,17,254,2,151643,151645,completed));
	packet[3]=1; packet[5]=254; packet[9]=packet[25]=0; assert(PioneerDecoderBatchPacket(packet,17,254,2,151643,151645,completed) && completed==1);
	packet[8]=42; assert(!PioneerDecoderBatchPacket(packet,17,254,2,151643,151645,completed));
	packet[3]=2; packet[5]=255; packet[9]=151645; packet[25]=0x7f800000; assert(!PioneerDecoderBatchPacket(packet,17,254,2,151643,151645,completed));
	puts("PASS OFFLINE two-position autonomous plan,all context/119-peer sequence overflow bounds,wrapped exact NoC/host256B counts,all summary words,complete token/logit/EOS/unused-zero packet rules;NO native/driver operation");
}
