/* SPDX-License-Identifier: MIT */
#include "../PioneerAttentionPrefixCache.h"
#include <assert.h>
#include <stdio.h>
struct Device {
	std::vector<unsigned> calls;
	bool fail=false;
	bool ResidentAttentionStep(unsigned n,unsigned layer,const uint32_t* q,const uint32_t* k,const uint32_t* v,uint32_t* y) {
		assert(layer<28 && q && y); calls.push_back(n); if (fail) return false;
		for (unsigned g=0;g<8;g++) for (unsigned j=0;j<64;j++) { assert(k[g*64+j]==n*100+g); assert(v[g*64+j]==n*100+g+10); }
		return true;
	}
};
int main()
{
	PioneerAttentionPrefixCache cache; Device device; uint32_t q[2048]={},y[2048]={};
	for (unsigned n:{1U,2U,3U,2U,3U,7U,1U,2U}) {
		std::vector<uint32_t> k(n*512),v(n*512);
		for (unsigned g=0;g<8;g++) for (unsigned t=0;t<n;t++) for (unsigned j=0;j<64;j++) { k[(g*n+t)*64+j]=(t+1)*100+g; v[(g*n+t)*64+j]=(t+1)*100+g+10; }
		unsigned before=unsigned(device.calls.size()); assert(cache.Run(device,0,n,q,k.data(),v.data(),y));
		assert(device.calls.size()-before==(n==7?7U:1U));
		if (n==3) { k[0]^=1; device.fail=true; assert(!cache.Run(device,0,n,q,k.data(),v.data(),y)); device.fail=false; }
	}
	assert(!cache.Run(device,28,1,q,q,q,y));
	printf("PASS: prefix append/rewind/cold restore/restart and failure have explicit device execution; no stale cache reuse or CPU attention substitution\n");
}
