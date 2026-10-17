/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderModel.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderBatchModel.h"
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
	uint64_t ropeOffset=next;
	for (uint64_t page=ropeOffset/4096;page<ropeOffset/4096+32;page++) initialized[page]=1;
	assert(PioneerDecoderBatchModelRegister(offsets,ropeOffset,151643,151645,initialized.data(),0x80000000ULL,job)
		&& job.reserved[1]==0x04000000+ropeOffset && job.reserved[2]==151643 && job.reserved[3]==151645 && !job.reserved[0] && !job.magic && !job.hostHigh);
	for (unsigned page=0;page<32;page++) {
		initialized[ropeOffset/4096+page]=0;
		assert(!PioneerDecoderBatchModelRegister(offsets,ropeOffset,151643,151645,initialized.data(),0x80000000ULL,job) && !job.vocabulary);
		initialized[ropeOffset/4096+page]=1;
	}
	for (unsigned i=0;i<sizeof(offsets)/8;i++) {
		uint64_t collision=reinterpret_cast<const uint64_t*>(&offsets)[i];
		assert(!PioneerDecoderBatchModelRegister(offsets,collision,151643,151645,initialized.data(),0x80000000ULL,job) && !job.vocabulary);
	}
	for (uint64_t invalid:{ropeOffset+1,UINT64_MAX,UINT64_C(0x7e3ff000),UINT64_C(0x7e400000)})
		assert(!PioneerDecoderBatchModelRegister(offsets,invalid,151643,151645,initialized.data(),0x80000000ULL,job) && !job.vocabulary);
	assert(!PioneerDecoderBatchModelRegister(offsets,ropeOffset,151643,151643,initialized.data(),0x80000000ULL,job));
	assert(!PioneerDecoderBatchModelRegister(offsets,ropeOffset,151936,151645,initialized.data(),0x80000000ULL,job));
	assert(!PioneerDecoderBatchModelRegister(offsets,ropeOffset,151643,151936,initialized.data(),0x80000000ULL,job));
	assert(PioneerDecoderBatchModelRegister(offsets,ropeOffset,151643,151645,initialized.data(),0x80000000ULL,job));
	puts("PASS OFFLINE autonomous-model prerequisite:all32 immutable RoPE pages,disjoint fromALL310 weight/norm extents and KV tail,EOG bounds,failed derivation clears control data;NOT driver/native");
	printf("PASS OFFLINE fixed decoder model registration,310 immutable extents/all first+last pages,KV-tail separation,64-bit overflow/alignment/uninitialized refusal,private address derivation;NO driver exposure\n");
}
