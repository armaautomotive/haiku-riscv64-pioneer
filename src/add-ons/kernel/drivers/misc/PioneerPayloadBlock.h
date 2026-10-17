/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PAYLOAD_BLOCK_H
#define PIONEER_PAYLOAD_BLOCK_H
#include <stdint.h>
// Contiguous owned SRAM DATA only, never control/NoC/reset/boot registers.
// The caller has already proved compute completion/hold and a fixed TLB.
// Aligned pairs may use one64-bit SRAM transfer; controls remain32-bit.
// Phase fences keep all accesses inside the ownership interval. No memcpy
// over-read, unaligned transaction, vector/FPU kernel state or skipped word.
template<class IO>
auto PioneerPayloadPairRead(IO& io,unsigned word,int) -> decltype(io.PayloadReadPair(word))
{ return io.PayloadReadPair(word); }
template<class IO>
uint64_t PioneerPayloadPairRead(IO& io,unsigned word,long)
{ uint64_t low=io.PayloadRead(word); return low|(uint64_t(io.PayloadRead(word+1))<<32); }
template<class IO>
auto PioneerPayloadPairWrite(IO& io,unsigned word,uint32_t low,uint32_t high,int)
	-> decltype(io.PayloadWritePair(word,low,high),void()) { io.PayloadWritePair(word,low,high); }
template<class IO>
void PioneerPayloadPairWrite(IO& io,unsigned word,uint32_t low,uint32_t high,long)
{ io.PayloadWrite(word,low); io.PayloadWrite(word+1,high); }
template<class IO>
bool PioneerPayloadReadBlock(IO& io,unsigned first,unsigned words,unsigned capacity,uint32_t* output)
{
	if (!output || !words || first>=capacity || words>capacity-first) return false;
	io.PayloadFence();
	unsigned at=0;
	if (first&1) { output[0]=io.PayloadRead(first); at=1; }
	for (;words-at>=8;at+=8) {
		uint64_t v[4];
		for (unsigned w=0;w<4;w++) v[w]=PioneerPayloadPairRead(io,first+at+w*2,0);
		for (unsigned w=0;w<4;w++) { output[at+w*2]=uint32_t(v[w]); output[at+w*2+1]=uint32_t(v[w]>>32); }
	}
	for (;words-at>=2;at+=2) {
		uint64_t value=PioneerPayloadPairRead(io,first+at,0);
		output[at]=uint32_t(value); output[at+1]=uint32_t(value>>32);
	}
	for (;at<words;at++) output[at]=io.PayloadRead(first+at);
	io.PayloadFence(); return true;
}
template<class IO>
bool PioneerPayloadStageBlock(IO& io,unsigned words,unsigned capacity,const uint32_t* input)
{
	if (!input || !words || words>capacity) return false;
	io.PayloadFence();
	unsigned at=0;
	for (;words-at>=2;at+=2) PioneerPayloadPairWrite(io,at,input[at],input[at+1],0);
	if (at<words) io.PayloadWrite(at,input[at]);
	io.PayloadFence();
	bool exact=true;
	at=0;
	for (;words-at>=2;at+=2) {
		uint64_t value=PioneerPayloadPairRead(io,at,0);
		exact &= uint32_t(value)==input[at]; exact &= uint32_t(value>>32)==input[at+1];
	}
	if (at<words) exact &= io.PayloadRead(at)==input[at];
	io.PayloadFence(); return exact;
}
template<class IO>
auto PioneerRuntimePayloadRead(IO& io,unsigned first,unsigned words,uint32_t* output,int)
	-> decltype(io.RuntimePayloadRead(first,words,output)) { return io.RuntimePayloadRead(first,words,output); }
template<class IO>
bool PioneerRuntimePayloadRead(IO& io,unsigned first,unsigned words,uint32_t* output,long)
{
	for (unsigned w=0;w<words;w++) output[w]=io.RuntimeBufferRead(first+w);
	return true;
}
template<class IO>
auto PioneerBulkPayloadStage(IO& io,const uint32_t* input,unsigned words,int)
	-> decltype(io.BulkPayloadStage(input,words)) { return io.BulkPayloadStage(input,words); }
template<class IO>
bool PioneerBulkPayloadStage(IO& io,const uint32_t* input,unsigned words,long)
{
	for (unsigned w=0;w<words;w++) io.BulkL1Write(w,input[w]);
	bool exact=true;
	for (unsigned w=0;w<words;w++) exact &= io.BulkL1Read(w)==input[w];
	return exact;
}
template<class IO>
auto PioneerBulkPayloadRead(IO& io,unsigned first,unsigned words,uint32_t* output,int)
	-> decltype(io.BulkPayloadRead(first,words,output)) { return io.BulkPayloadRead(first,words,output); }
template<class IO>
bool PioneerBulkPayloadRead(IO& io,unsigned first,unsigned words,uint32_t* output,long)
{
	for (unsigned w=0;w<words;w++) output[w]=io.BulkL1Read(first+w);
	return true;
}
#endif
