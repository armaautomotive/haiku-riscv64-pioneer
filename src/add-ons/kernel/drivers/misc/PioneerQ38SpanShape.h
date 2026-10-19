/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SPAN_SHAPE_H
#define PIONEER_Q38_SPAN_SHAPE_H
#ifdef PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#else
#include <stdint.h>
#endif
// OFFLINE research ONLY. No ioctl/driver selects this transport. Shape is
// NOT ownership:only a future kernel-private bank/host lease may construct
// this descriptor,including the sixteen independently sealed NoC sources.
enum {
	TT_Q38_SPAN_MARKER=0x31553851,TT_Q38_SPAN_FAULT=0x46553851,
	TT_Q38_SPAN_HOST_BYTES=0x100000,TT_Q38_SPAN_JOB_WORDS=64,
	TT_Q38_SPAN_STATS_WORDS=32
};
static inline uint32_t PioneerQ38SpanRoute(unsigned bank)
{
	// SAME seven translated routes audited for this Pioneer/P100A.
	// Physical bank3 is absent;logical0..6 use physical0,1,2,4,5,6,7.
	const unsigned x[]={18,18,18,17,17,17,17},y[]={12,15,18,12,15,18,21};
	return bank<7?x[bank]|(y[bank]<<6):~uint32_t(0);
}
static inline bool PioneerQ38SpanShape(const uint32_t* d)
{
	if (!d || d[0]!=TT_Q38_SPAN_MARKER || d[1]!=1 || d[2]!=1 || d[3]
		|| !d[4] || d[4]>TT_Q38_SPAN_HOST_BYTES || (d[4]&4095)
		|| (d[5]&4095) || d[5]<0x04000000 || d[5]>=0xc4000000 || d[4]>0xc4000000-d[5]
		|| !d[6] || d[6]>=0x7fffffff || (d[7]&(TT_Q38_SPAN_HOST_BYTES-1))
		|| !d[8] || d[8]>0xffff || (d[8]==0xffff && d[7]>0xfff00000)
		|| d[9]!=TT_Q38_SPAN_HOST_BYTES || d[10]>=7 || d[11]!=PioneerQ38SpanRoute(d[10])) return false;
	for (unsigned i=12;i<16;i++) if (d[i]) return false;
	for (unsigned i=32;i<TT_Q38_SPAN_JOB_WORDS;i++) if (d[i]) return false;
	return true;
}
// CPU-side independent transport summary observer. No IO/allocation/code,
// learned arithmetic or authority comes from this DATA-only predicate.
static inline bool PioneerQ38SpanSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after)
{
	if (!s || !PioneerQ38SpanShape(d) || !before || !after) return false;
	unsigned bytes=d[4],chunks=(bytes+16383)/16384,reads=4*chunks,writes=3*chunks;
	if (s[0]!=reads || s[1]!=bytes/16 || s[2]!=writes || s[3]!=3*(bytes/64)
		|| s[4]!=bytes/4096 || s[5]!=bytes || s[6]!=d[6] || s[7]!=TT_Q38_SPAN_MARKER
		|| s[24]!=d[10] || s[25]!=d[11] || s[26]!=d[5] || s[27]!=d[7]
		|| s[28]!=d[8] || s[29]!=d[9] || s[30]!=1 || s[31]
		|| before[2]!=before[6] || after[2]!=after[6]) return false;
	const unsigned delta[]={reads,bytes/16,reads+writes,reads,writes,3*(bytes/64),reads+writes,writes};
	for (unsigned i=0;i<8;i++) if (s[8+i]!=before[i] || s[16+i]!=after[i]
		|| uint32_t(after[i]-before[i])!=delta[i]) return false;
	return true;
}
#endif
