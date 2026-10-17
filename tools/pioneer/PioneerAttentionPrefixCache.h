/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_ATTENTION_PREFIX_CACHE_H
#define PIONEER_ATTENTION_PREFIX_CACHE_H
#include <stdint.h>
#include <string.h>
#include <vector>
enum { PIONEER_RESIDENT_KV_BYTES=28*1024*1024 };
// Host cache-coherence bookkeeping, NOT a CPU attention/output validation.
// A changed/restored llama prefix is replayed explicitly on the device.
// Never assume an old device KV prefix belongs to a new conversation.
class PioneerAttentionPrefixCache {
	std::vector<uint32_t> keys[28],values[28];
public:
	template<class Device>
	bool Run(Device& device,unsigned layer,unsigned tokens,const uint32_t* query,
		const uint32_t* key,const uint32_t* value,uint32_t* output)
	{
		if (layer>=28 || !tokens || tokens>256 || !query || !key || !value || !output) return false;
		auto& oldK=keys[layer]; auto& oldV=values[layer];
		unsigned oldTokens=unsigned(oldK.size()/512);
		bool append=oldTokens>=tokens-1;
		for (unsigned g=0;g<8 && append && tokens>1;g++) {
			append=!memcmp(key+g*tokens*64,oldK.data()+g*oldTokens*64,(tokens-1)*256)
				&& !memcmp(value+g*tokens*64,oldV.data()+g*oldTokens*64,(tokens-1)*256);
		}
		uint32_t deltaK[512],deltaV[512];
		for (unsigned n=append?tokens:1;n<=tokens;n++) {
			for (unsigned g=0;g<8;g++) {
				memcpy(deltaK+g*64,key+(g*tokens+n-1)*64,256);
				memcpy(deltaV+g*64,value+(g*tokens+n-1)*64,256);
			}
			if (!device.ResidentAttentionStep(n,layer,query,deltaK,deltaV,output)) return false;
		}
		oldK.assign(key,key+tokens*512); oldV.assign(value,value+tokens*512);
		return true;
	}
};
#endif
