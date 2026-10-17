/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQkvDescriptorPlan.h"
struct Tensor { uint32_t base; unsigned k,n; std::vector<bool> seen; };
int main() {
	PioneerDecoderJob model={}; std::vector<Tensor> tensors; uint32_t cursor=0x04000000;
	const unsigned k[]={1024,1024,1024,2048,1024,1024,3072},n[]={2048,1024,1024,1024,3072,3072,1024};
	for (unsigned layer=0;layer<28;layer++) for (unsigned role=0;role<7;role++) {
		reinterpret_cast<uint32_t*>(&model.layers[layer])[role]=cursor;
		tensors.push_back({cursor,k[role],n[role],std::vector<bool>(n[role],false)});
		cursor+=k[role]*n[role]*9/8; assert(!(cursor&4095));
	}
	model.vocabulary=cursor; tensors.push_back({cursor,1024,151936,std::vector<bool>(151936,false)});
	assert(PioneerDecoderWeight(cursor,1024,151936));
	unsigned matrixJobs=0,attention=0,glu=0,all=0;
	for (unsigned worker=1;worker<120;worker++) {
		unsigned count=PioneerQkvDescriptorCount(worker); assert(count && count<=169 && count<=TT_DECODER_TABLE_ENTRIES);
		assert(TT_DECODER_TABLE_BASE+count*32<=0x88000);
		assert(count==(worker<=6?169U:worker<=8?141U:worker<=32?113U:57U));
		for (unsigned index=0;index<count;index++) {
			PioneerDecoderDescriptor entry; assert(PioneerQkvDescriptorEntry(model,worker,index,entry)); all++;
			const auto* w=entry.words;
			assert(w[3]==1 && w[4]==PioneerDecoderDescriptorXY(worker) && !w[5] && !w[6]);
			if (w[7]) {
				assert(!w[0] && !w[1] && !w[2]);
				if (w[7]==0x31475441) { assert(worker<=8); attention++; }
				else { assert(w[7]==0x314c4741 && worker<=6); glu++; }
				continue;
			}
			matrixJobs++; unsigned matches=0;
			for (auto& t:tensors) {
				uint64_t bytes=uint64_t(t.k)*t.n*9/8;
				if (w[0]<t.base || uint64_t(w[0])-t.base>=bytes) continue;
				matches++; unsigned rowBytes=t.k*9/8,offset=w[0]-t.base;
				assert(!(offset%rowBytes) && w[1]*16==t.k && w[2] && w[2]<=82);
				unsigned first=offset/rowBytes,rows=w[2]*16;
				assert(!(first&31) && !(rows&31) && first+rows<=t.n && uint64_t(offset)+rows*rowBytes<=bytes);
				for (unsigned row=first;row<first+rows;row++) { assert(!t.seen[row]); t.seen[row]=true; }
			}
			assert(matches==1);
		}
		PioneerDecoderDescriptor bad; assert(!PioneerQkvDescriptorEntry(model,worker,count,bad));
		for (auto value:bad.words) assert(!value);
		// Every first dispatch binds ALL THREE QKV extents, not just its own.
		for (unsigned role=0;role<3;role++) { auto invalid=model; reinterpret_cast<uint32_t*>(&invalid.layers[0])[role]=0;
			assert(!PioneerQkvDescriptorEntry(invalid,worker,0,bad)); }
	}
	for (const auto& t:tensors) for (bool seen:t.seen) assert(seen);
	assert(tensors.size()==197 && matrixJobs==8575 && attention==224 && glu==168 && all==8967);
	for (unsigned worker:{0U,120U,UINT32_MAX}) { PioneerDecoderDescriptor bad;
		assert(!PioneerQkvDescriptorCount(worker) && !PioneerQkvDescriptorEntry(model,worker,0,bad)); }
	printf("PASS OFFLINE QKV descriptor schedule,197 immutable tensors/ALL rows once/119 bounded table cursors,8575 math+224 attention+168 GLU,ALL3 extent refusals/unchanged routes/table owner;NOT driver-selected or native performance\n");
}
