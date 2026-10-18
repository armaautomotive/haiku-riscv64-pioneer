/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PAYLOAD_H
#define PIONEER_Q38_PAYLOAD_H
#include "PioneerPayloadBlock.h"
// OFFLINE follow-on DATA transport only. No reset/NoC/control/code access.
// Caller first proves exclusive mapping and the relevant held/frozen/stopped
// phase. Full fill/readback and every output/input bit remain checked.
static inline bool PioneerQ38PayloadInputs(uint32_t a,unsigned words)
{
	return (a==0xa0000 && (words==2560 || words==3072 || words==8704))
		|| (a==0x94000 && (words==32 || words==80 || words==160 || words==192 || words==544));
}
static inline bool PioneerQ38PayloadOutput(unsigned words) { return words && words<=256 && !(words&15); }
template<class IO> bool PioneerQ38PayloadStage(IO& io,uint32_t a,const uint32_t* input,unsigned words)
{
	if (!input || !(PioneerQ38PayloadInputs(a,words) || (a==0x74000 && words==10)
		|| (a==0x84000 && PioneerQ38PayloadOutput(words))) || !io.Q38PanelMappingStable()) return false;
	auto view=io.Q38PayloadData(a);
	return PioneerPayloadStageBlock(view,words,words,input);
}
template<class IO> bool PioneerQ38PayloadCompare(IO& io,uint32_t a,const uint32_t* input,unsigned words)
{
	if (!input || !PioneerQ38PayloadInputs(a,words) || !io.Q38PanelMappingStable()) return false;
	auto view=io.Q38PayloadData(a); bool okay=true;
	view.PayloadFence();
	for (unsigned w=0;w<words;w+=2) {
		uint64_t value=PioneerPayloadPairRead(view,w,0);
		okay &= uint32_t(value)==input[w]; okay &= uint32_t(value>>32)==input[w+1];
	}
	view.PayloadFence(); return okay;
}
template<class IO> bool PioneerQ38PayloadReadOutput(IO& io,unsigned words,uint32_t* output)
{
	if (!output || !PioneerQ38PayloadOutput(words) || !io.Q38PanelMappingStable()) return false;
	auto view=io.Q38PayloadData(0x84000);
	return PioneerPayloadReadBlock(view,0,words,256,output);
}
#endif
