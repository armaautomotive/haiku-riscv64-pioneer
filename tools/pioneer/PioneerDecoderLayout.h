/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_LAYOUT_H
#define PIONEER_DECODER_LAYOUT_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// OFFLINE fixed Qwen3-0.6B decoder candidate ONLY. This is PRIVATE GPU data,
// not a public ioctl: a future controller must derive addresses/sequences,
// verify every immutable initialized extent and own all referenced leases.
// No driver/backend currently stages,loads or selects this job.
enum { TT_DECODER_JOB_BASE=0x146000,TT_DECODER_JOB_BYTES=4096,TT_DECODER_LAYERS=28,
	TT_DECODER_WORKERS=119,TT_DECODER_INPUT=0x84000,TT_DECODER_HIDDEN=0x8a000,
	TT_DECODER_RESIDUAL=0x8b000,TT_DECODER_Q=0x87000,TT_DECODER_K=0x86000,
	TT_DECODER_NORM=0x89000,TT_DECODER_VOCAB=151936 };
struct PioneerDecoderLayer {
	uint32_t q,v,k,o,gate,up,down,attentionNorm,ffnNorm,qNorm,kNorm;
};
struct PioneerDecoderJob {
	uint32_t magic,operation,version,tokens,nonce,kvBase,finalNorm,vocabulary;
	uint32_t hostLow,hostHigh,hostBytes,workers,reserved[4];
	PioneerDecoderLayer layers[TT_DECODER_LAYERS];
	uint32_t sequence[TT_DECODER_WORKERS];
	uint32_t rope[128]; // interleaved exact cos/sin table for THIS position
};
static_assert(sizeof(PioneerDecoderJob)<=TT_DECODER_JOB_BYTES,"private decoder args must not overlap NoC scratch at147000");
static inline unsigned PioneerDecoderSequenceAdvance(unsigned index)
{
	return index>=119?0:index<32?197:index<64?85:index<96?57:1;
}
static inline bool PioneerDecoderWeight(uint32_t base,unsigned k,unsigned n)
{
	unsigned bytes=k*n+k*n/8;
	return !(base&4095) && base>=0x04000000 && base<0x82400000 && bytes<=0x82400000-base;
}
static inline bool PioneerDecoderNorm(uint32_t base)
{
	return !(base&4095) && base>=0x04000000 && base<=0x823ff000;
}
static inline bool PioneerDecoderJobValid(const volatile PioneerDecoderJob& job)
{
	if (job.magic!=0x31444344 || job.operation!=8 || job.version!=1 || !job.tokens || job.tokens>256
		|| !job.nonce || job.nonce>=0x7fffffff || job.kvBase!=0x82400000 || job.workers!=119
		|| !job.hostHigh || job.hostHigh>0xffff || (job.hostLow&0xfffff) || job.hostBytes!=0x100000
		|| (job.hostHigh==0xffff && job.hostLow>0xfff00000) || !PioneerDecoderNorm(job.finalNorm)
		|| !PioneerDecoderWeight(job.vocabulary,1024,151936)) return false;
	for (unsigned i=0;i<4;i++) if (job.reserved[i]) return false;
	for (unsigned i=0;i<119;i++) if (!job.sequence[i] || job.sequence[i]>0x7fffffff-PioneerDecoderSequenceAdvance(i)) return false;
	for (unsigned i=0;i<128;i++) { uint32_t a=job.rope[i]&0x7fffffff; if (a>0x3f800000 || (a && a<0x26800000)) return false; }
	for (unsigned i=0;i<28;i++) {
		const volatile auto& l=job.layers[i];
		if (!PioneerDecoderWeight(l.q,1024,2048) || !PioneerDecoderWeight(l.v,1024,1024) || !PioneerDecoderWeight(l.k,1024,1024)
			|| !PioneerDecoderWeight(l.o,2048,1024) || !PioneerDecoderWeight(l.gate,1024,3072) || !PioneerDecoderWeight(l.up,1024,3072)
			|| !PioneerDecoderWeight(l.down,3072,1024) || !PioneerDecoderNorm(l.attentionNorm) || !PioneerDecoderNorm(l.ffnNorm)
			|| !PioneerDecoderNorm(l.qNorm) || !PioneerDecoderNorm(l.kNorm)) return false;
	}
	return true;
}
static inline bool PioneerDecoderSummary(const uint32_t* s,unsigned tokens,unsigned nonce,const uint32_t* before,const uint32_t* after)
{
	if (!s || !before || !after || !tokens || tokens>256 || !nonce || nonce>=0x7fffffff || s[4]!=119 || s[5]!=151936
		|| s[6]!=nonce || s[7]!=8 || before[2]!=before[6] || after[2]!=after[6]) return false;
	// 28*(64+32+32+32+96+96+32) ordinary matrix peers,119 final wide
	// peers,113 immutable norm reads and28 complete8-group attention ops.
	unsigned r=s[0],minimum=43716+448*((tokens+63)>>6),w=22309,wf=33030;
	if (r<minimum || r>minimum+2*65535*10871U || ((r-minimum)&1) || s[1]!=77205+1792*tokens+(r-minimum) || s[2]!=w || s[3]!=wf) return false;
	const unsigned delta[]={r,s[1],r+w,r,w,wf,r+w,w};
	for (unsigned i=0;i<8;i++) if (s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i]) return false;
	return true;
}
#endif
