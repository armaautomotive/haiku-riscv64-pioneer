/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderModel.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
int main()
{
	PioneerDecoderModelOffsets offsets={}; uint64_t next=0;
	const unsigned k[]={1024,1024,1024,2048,1024,1024,3072},n[]={2048,1024,1024,1024,3072,3072,1024};
	std::vector<uint8_t> initialized(0x80000000ULL/4096,0); std::vector<unsigned> first,last;
	auto add=[&](uint64_t& offset,uint64_t bytes) { offset=next; first.push_back(unsigned(next/4096)); next+=(bytes+4095)&~UINT64_C(4095);
		last.push_back(unsigned(next/4096)-1); for (uint64_t p=offset/4096;p<next/4096;p++) initialized[p]=1; };
	for (auto& layer:offsets.layers) { auto* words=reinterpret_cast<uint64_t*>(&layer);
		for (unsigned i=0;i<11;i++) add(words[i],i<7?uint64_t(k[i])*n[i]*9/8:4096); }
	add(offsets.finalNorm,4096); add(offsets.vocabulary,UINT64_C(1024)*151936*9/8);
	PioneerDecoderJob job={}; assert(PioneerDecoderModelRegister(offsets,initialized.data(),0x80000000ULL,job));
	assert(job.layers[0].q==0x04000000 && job.finalNorm==0x04000000+offsets.finalNorm && job.vocabulary==0x04000000+offsets.vocabulary
		&& !job.magic && !job.tokens && !job.nonce && !job.hostLow && !job.hostHigh);
	for (unsigned p:first) { initialized[p]=0; assert(!PioneerDecoderModelRegister(offsets,initialized.data(),0x80000000ULL,job)); initialized[p]=1; }
	for (unsigned p:last) { initialized[p]=0; assert(!PioneerDecoderModelRegister(offsets,initialized.data(),0x80000000ULL,job)); initialized[p]=1; }
	for (unsigned i=0;i<sizeof(offsets)/8;i++) { auto changed=offsets; auto* words=reinterpret_cast<uint64_t*>(&changed);
		words[i]=0x7e400000; assert(!PioneerDecoderModelRegister(changed,initialized.data(),0x80000000ULL,job));
		words[i]=UINT64_MAX; assert(!PioneerDecoderModelRegister(changed,initialized.data(),0x80000000ULL,job));
		words[i]=1; assert(!PioneerDecoderModelRegister(changed,initialized.data(),0x80000000ULL,job)); }
	initialized[0x7e400000/4096]=1; assert(!PioneerDecoderModelRegister(offsets,initialized.data(),0x80000000ULL,job)); initialized[0x7e400000/4096]=0;
	assert(!PioneerDecoderModelRegister(offsets,initialized.data(),0x40000000ULL,job));
	assert(PioneerDecoderModelRegister(offsets,initialized.data(),0x80000000ULL,job));
	printf("PASS OFFLINE fixed decoder model registration,310 immutable extents/all first+last pages,KV-tail separation,64-bit overflow/alignment/uninitialized refusal,private address derivation;NO driver exposure\n");
}
