/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WORK_STAGE_H
#define PIONEER_Q38_WORK_STAGE_H
// Opt-in WORK data staging ONLY. Fixed owned L1 data windows,no code,
// NoC/control/reset registers or user physical addresses. Do NOT verify
// every payload word on CPU;the checked/debug path remains unchanged.
// ALL writes are followed by one same-device nonposted read before ANY
// arm/release. CPU fences alone do NOT flush posted PCIe writes:
// https://docs.kernel.org/driver-api/device-io.html
template<class IO> auto PioneerQ38WorkPost(IO& io,uint32_t a,const uint32_t* input,unsigned words,int)
	-> decltype(io.Q38PayloadData(a),bool())
{
	if (!input || (!PioneerQ38DispatchStageRange(a,words) && !PioneerQ38DispatchCompactStageRange(a,words)) || !io.Q38PanelMappingStable()) return false;
	auto data=io.Q38PayloadData(a); data.PayloadFence();
	unsigned at=0;
	for (;words-at>=2;at+=2) PioneerPayloadPairWrite(data,at,input[at],input[at+1],0);
	if (at<words) data.PayloadWrite(at,input[at]);
	data.PayloadFence(); return true;
}
template<class IO> bool PioneerQ38WorkPost(IO& io,uint32_t a,const uint32_t* input,unsigned words,long)
{
	if (!input || (!PioneerQ38DispatchStageRange(a,words) && !PioneerQ38DispatchCompactStageRange(a,words)) || !io.Q38PanelMappingStable()) return false;
	// Scalar backend's Q38PanelWrite uses the existing ordered32-bit MMIO
	// accessor. Still drained once after the COMPLETE private job frame.
	for (unsigned w=0;w<words;w++) io.Q38PanelWrite(a+w*4,input[w]);
	return true;
}
template<class IO> auto PioneerQ38WorkDrain(IO& io,int) -> decltype(io.Q38PayloadData(0x74000),bool())
{
	if (!io.Q38PanelMappingStable()) return false;
	auto data=io.Q38PayloadData(0x74000); data.PayloadFence();
	(void)data.PayloadRead(0); // owned header word,NOT a numeric comparison
	data.PayloadFence(); return true;
}
template<class IO> bool PioneerQ38WorkDrain(IO& io,long)
{
	if (!io.Q38PanelMappingStable()) return false;
	(void)io.Q38PanelRead(0x74000); return true;
}
template<class IO,bool Work> bool PioneerQ38WorkStage(IO& io,uint32_t a,const uint32_t* input,unsigned words,int)
{
	if constexpr (Work) return PioneerQ38WorkPost(io,a,input,words,0);
	return PioneerQ38DispatchStage(io,a,input,words,0);
}
#endif
