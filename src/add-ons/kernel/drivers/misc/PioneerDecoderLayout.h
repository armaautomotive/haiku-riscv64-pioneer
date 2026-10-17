/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_LAYOUT_H
#define PIONEER_DECODER_LAYOUT_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// Fixed experimental Qwen3-0.6B decoder. This is PRIVATE GPU data,
// not a public ioctl: a future controller must derive addresses/sequences,
// verify every immutable initialized extent and own all referenced leases.
// Only the separate explicit model/run ABI may construct this private job.
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
static inline unsigned PioneerDecoderSequenceAdvance(unsigned index,bool parallel=false,bool glu=false,bool paired=false,bool smallRows=false)
{
	if (paired && smallRows) return index>=119?0:(index<64?169:57)+(parallel && index<8?28:0)+(glu && index<6?28:0);
	if (!paired) return index>=119?0:(index<32?197:index<64?85:index<96?57:1)+(parallel && index<8?28:0)+(glu && index<6?28:0);
	return index>=119?0:(index<32?169:index<64?57:29)+(parallel && index<8?28:0)+(glu && index<6?28:0);
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
// Exact bounded EOG set:one28-token window plus ONE arbitrary outlier.
// word0[17:0]=window base,word0[31:18]=low14(outlier+1),
// word1[27:0]=window mask,word1[31:28]=high4(outlier+1).
// The zero outlier code means absent. This preserves the complete set even
// when a tokenizer also declares a distant legacy </s> token as EOG.
static inline bool PioneerDecoderEogSetValid(unsigned word0,uint32_t word1)
{
	unsigned base=word0&0x3ffff,extra=(word0>>18)|((word1>>28)<<14);
	uint32_t mask=word1&0x0fffffff;
	if (base>=151936 || !(mask&1) || extra>151936) return false;
	unsigned available=151936-base;
	if (available<28 && (mask>>available)) return false;
	return !extra || extra-1<base || extra-1-base>=28;
}
static inline bool PioneerDecoderIsEog(unsigned token,unsigned word0,uint32_t word1)
{
	unsigned base=word0&0x3ffff,extra=(word0>>18)|((word1>>28)<<14);
	return (extra && token==extra-1) || (token>=base && token-base<28 && ((word1>>(token-base))&1));
}
// CPU preflight only:sorted unique COMPLETE pinned-vocabulary EOG set.
// Refuse unrepresentable sets;never silently drop an EOG token.
static inline bool PioneerDecoderPackEog(const unsigned* ids,unsigned count,unsigned& word0,uint32_t& word1)
{
	word0=word1=0;
	if (!ids || !count || count>29) return false;
	for (unsigned i=0;i<count;i++) if (ids[i]>=151936 || (i && ids[i]<=ids[i-1])) return false;
	for (unsigned candidate=0;candidate<count;candidate++) {
		unsigned base=ids[candidate],extra=0; uint32_t mask=0; bool valid=true;
		for (unsigned i=0;i<count;i++) {
			if (ids[i]>=base && ids[i]-base<28) mask|=1U<<(ids[i]-base);
			else if (!extra) extra=ids[i]+1;
			else { valid=false; break; }
		}
		if (valid) { word0=base|((extra&0x3fff)<<18); word1=mask|((extra>>14)<<28); return PioneerDecoderEogSetValid(word0,word1); }
	}
	return false;
}
static inline bool PioneerDecoderJobValid(const volatile PioneerDecoderJob& job,unsigned requiredVersion=1)
{
	if ((requiredVersion<1 || requiredVersion>10) || job.magic!=0x31444344 || (job.operation!=8 && !(requiredVersion>=4 && job.operation==9) && !(requiredVersion==10 && job.operation==10)) || job.version!=requiredVersion || !job.tokens || job.tokens>256
		|| !job.nonce || job.nonce>=0x7fffffff || job.kvBase!=0x82400000 || job.workers!=119
		|| !job.hostHigh || job.hostHigh>0xffff || (job.hostLow&0xfffff) || job.hostBytes!=0x100000
		|| (job.hostHigh==0xffff && job.hostLow>0xfff00000) || !PioneerDecoderNorm(job.finalNorm)
		|| !PioneerDecoderWeight(job.vocabulary,1024,151936)) return false;
	if (requiredVersion==10) {
		if (!job.reserved[0] || job.reserved[0]>2 || (job.operation!=10 && job.reserved[0]!=1) || job.reserved[0]>257-job.tokens || (job.reserved[1]&4095)
			|| job.reserved[1]<0x04000000 || job.reserved[1]>0x823e0000 || !PioneerDecoderEogSetValid(job.reserved[2],job.reserved[3])) return false;
	} else for (unsigned i=0;i<4;i++) if (job.reserved[i]) return false;
	for (unsigned i=0;i<119;i++) if (!job.sequence[i] || job.sequence[i]>0x7fffffff-PioneerDecoderSequenceAdvance(i,requiredVersion>=2,requiredVersion>=3,requiredVersion>=5,requiredVersion==8)*(requiredVersion==10?job.reserved[0]:1)) return false;
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
static inline bool PioneerDecoderSummary(const uint32_t* s,unsigned tokens,unsigned nonce,const uint32_t* before,const uint32_t* after,bool parallel=false,bool glu=false,bool greedy=false,bool paired=false,bool pull=false,bool diagnostic=false,bool descriptorTable=false,bool smallRows=false,bool ropeTable=false,bool fusedGather=false)
{
	if (fusedGather && (!ropeTable || !descriptorTable || smallRows)) return false;
	if (ropeTable && (!descriptorTable || smallRows)) return false;
	if ((smallRows && !descriptorTable) || (descriptorTable && (!pull || diagnostic)) || (diagnostic && (!pull || greedy)) || (pull && !paired) || (paired && !glu) || (greedy && !glu) || (glu && !parallel) || !s || !before || !after || !tokens || tokens>256 || !nonce || nonce>=0x7fffffff || s[4]!=119 || s[5]!=151936
		|| s[6]!=nonce || s[7]!=(greedy?9U:8U) || before[2]!=before[6] || after[2]!=after[6]) return false;
	// 28*(64+32+32+32+96+96+32) ordinary matrix peers,119 final wide
	// peers,113 immutable norm reads and28 complete8-group attention ops.
	unsigned r=s[0],minimum=glu?47412:parallel?45732:43716+448*((tokens+63)>>6),w=glu?23821:parallel?22981:22309,wf=glu?48542:parallel?37286:33030;
	if (greedy) { minimum-=118; w-=118; wf-=9494; }
	if (paired) { minimum-=8176; w-=4088; wf-=4088; }
	if (pull) { minimum-=784; w-=784; wf-=16128; }
	if (diagnostic) minimum+=112; // four explicit32B root/peer diagnostic readbacks per layer
	if (descriptorTable) { minimum-=9219; w-=9219; wf-=9219; }
	// +5124 real matrix jobs;each adds3 read requests,2 fixed read flits,
	// one doorbell write/flit. Gathered output flits stay EXACTLY constant.
	if (smallRows) { minimum+=15372; w+=5124; wf+=5124; }
	if (ropeTable) minimum+=2;
	if (fusedGather) minimum-=8827; // matrix jobs ONLY;attention/GLU unchanged,SAME flits
	if (r<minimum || r>minimum+2*65535*(paired?(smallRows?14343U:9219U):glu?11263U:parallel?11095U:10871U) || ((r-minimum)&1)
		|| s[1]!=(glu?102517:parallel?87733:77205+1792*tokens)-(greedy?9494:0)-(paired?6132:0)-(pull?16128:0)+(diagnostic?112:0)-(descriptorTable?9219:0)+(smallRows?10248:0)+(ropeTable?16:0)+(r-minimum) || s[2]!=w || s[3]!=wf) return false;
	const unsigned delta[]={r,s[1],r+w,r,w,wf,r+w,w};
	for (unsigned i=0;i<8;i++) if (s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i]) return false;
	return true;
}
#endif
