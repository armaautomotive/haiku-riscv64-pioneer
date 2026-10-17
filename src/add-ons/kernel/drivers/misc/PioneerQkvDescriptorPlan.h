/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QKV_DESCRIPTOR_PLAN_H
#define PIONEER_QKV_DESCRIPTOR_PLAN_H
#include "PioneerDecoderDescriptorTable.h"
#include "PioneerPairedQkvPlan.h"
// OFFLINE prerequisite ONLY. SAME descriptor/worker/table owners. NOT selected
// by the live driver, registry, root image or client. No new route authority.
static inline unsigned PioneerQkvDescriptorCount(unsigned worker)
{
	return worker && worker<=119?PioneerPairedQkvSequenceAdvance(worker-1):0;
}
static inline bool PioneerQkvDescriptorEntry(const PioneerDecoderJob& model,unsigned worker,unsigned index,PioneerDecoderDescriptor& entry)
{
	entry={}; if (index>=PioneerQkvDescriptorCount(worker)) return false;
	unsigned at=0;
	for (unsigned layer=0;layer<28;layer++) {
		const auto& l=model.layers[layer];
		for (unsigned phase=0;phase<6;phase++) {
			if ((phase==1 && worker>8) || (phase==4 && worker>6)
				|| ((phase==2 || phase==5) && worker>32)) continue;
			if (at++!=index) continue;
			if (phase==1 || phase==4) {
				entry.words[3]=1; entry.words[4]=PioneerDecoderDescriptorXY(worker);
				entry.words[7]=phase==1?0x31475441:0x314c4741; return true;
			}
			if (phase==2 || phase==5) return PioneerDecoderDescriptorMatrix(phase==2?l.o:l.down,phase==2?2048:3072,1024,worker,entry);
			if (phase==0) {
				PioneerPairedQkvSpan span; if (!PioneerPairedQkvWorker(worker,span)
					|| !PioneerDecoderWeight(l.q,1024,2048) || !PioneerDecoderWeight(l.v,1024,1024) || !PioneerDecoderWeight(l.k,1024,1024)) return false;
				entry.words[0]=(span.tensor==0?l.q:span.tensor==1?l.v:l.k)+span.first*1152;
				entry.words[1]=64; entry.words[2]=span.rows/16;
			} else {
				PioneerPairedFfnSpan span; if (!PioneerPairedFfnWorker(worker,span)
					|| !PioneerDecoderWeight(l.gate,1024,3072) || !PioneerDecoderWeight(l.up,1024,3072)) return false;
				entry.words[0]=(span.tensor?l.up:l.gate)+span.first*1152;
				entry.words[1]=64; entry.words[2]=span.rows/16;
			}
			entry.words[3]=1; entry.words[4]=PioneerDecoderDescriptorXY(worker); return true;
		}
	}
	return at==index && PioneerDecoderDescriptorMatrix(model.vocabulary,1024,151936,worker,entry);
}
#endif
