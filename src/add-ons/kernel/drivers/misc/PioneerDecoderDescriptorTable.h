/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_DESCRIPTOR_TABLE_H
#define PIONEER_DECODER_DESCRIPTOR_TABLE_H
#include "PioneerDecoderLayout.h"
#include "PioneerPairedFfnPlan.h"
// OFFLINE prerequisite ONLY. One fixed Qwen job schedule in an existing
// peer bulk-L1 owner. Maximum225 entries end87c20,before raw weights88000.
// Largest classifier result ends85400,before table86000. No user routes.
enum { TT_DECODER_TABLE_BASE=0x86000,TT_DECODER_TABLE_ENTRIES=225,
	TT_DECODER_TABLE_WORDS=225*8,TT_DECODER_TABLE_COUNT=0x713c0,
	TT_DECODER_TABLE_CURSOR=0x713c4 };
struct PioneerDecoderDescriptor { uint32_t words[8]; };
static inline unsigned PioneerDecoderDescriptorCount(unsigned worker)
{
	return worker && worker<=119?PioneerDecoderSequenceAdvance(worker-1,true,true,true):0;
}
static inline unsigned PioneerDecoderDescriptorXY(unsigned worker)
{
	const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
	return worker && worker<=119?columns[worker/10]|((2+worker%10)<<6):0;
}
static inline bool PioneerDecoderDescriptorMatrix(uint32_t base,unsigned k,unsigned n,unsigned worker,PioneerDecoderDescriptor& entry)
{
	if (!worker || worker>119 || !PioneerDecoderWeight(base,k,n)) return false;
	unsigned blocks=n/32,workers=blocks<119?blocks:119;
	if (worker>workers) return false;
	unsigned per=blocks/workers,extra=blocks%workers,ordinal=worker-1;
	unsigned first=(ordinal*per+(ordinal<extra?ordinal:extra))*32;
	unsigned rows=(per+(ordinal<extra))*32;
	entry={}; entry.words[0]=base+first*(k+k/8); entry.words[1]=k/16;
	entry.words[2]=rows/16; entry.words[3]=1; entry.words[4]=PioneerDecoderDescriptorXY(worker);
	return true;
}
static inline bool PioneerDecoderDescriptorEntry(const PioneerDecoderJob& model,unsigned worker,unsigned index,PioneerDecoderDescriptor& entry)
{
	entry={}; if (index>=PioneerDecoderDescriptorCount(worker)) return false;
	unsigned at=0;
	for (unsigned layer=0;layer<28;layer++) {
		const auto& l=model.layers[layer];
		const unsigned base[]={l.q,l.v,l.k,l.o,l.gate,l.down};
		const unsigned inputs[]={1024,1024,1024,2048,1024,3072};
		const unsigned outputs[]={2048,1024,1024,1024,6144,1024};
		for (unsigned phase=0;phase<8;phase++) {
			if (phase==3 || phase==6) {
				if (worker>(phase==3?8U:6U)) continue;
				if (at++!=index) continue;
				entry.words[3]=1; entry.words[4]=PioneerDecoderDescriptorXY(worker);
				entry.words[7]=phase==3?0x31475441:0x314c4741;
				return true;
			}
			unsigned role=phase<3?phase:phase<6?phase-1:5;
			unsigned limit=outputs[role]/32; if (limit>119) limit=119;
			if (worker>limit) continue;
			if (at++!=index) continue;
			if (role!=4) return PioneerDecoderDescriptorMatrix(base[role],inputs[role],outputs[role],worker,entry);
			PioneerPairedFfnSpan span; if (!PioneerPairedFfnWorker(worker,span)
				|| !PioneerDecoderWeight(l.gate,1024,3072) || !PioneerDecoderWeight(l.up,1024,3072)) return false;
			entry.words[0]=(span.tensor?l.up:l.gate)+span.first*1152;
			entry.words[1]=64; entry.words[2]=span.rows/16; entry.words[3]=1;
			entry.words[4]=PioneerDecoderDescriptorXY(worker); return true;
		}
	}
	return at==index && PioneerDecoderDescriptorMatrix(model.vocabulary,1024,151936,worker,entry);
}
#endif
