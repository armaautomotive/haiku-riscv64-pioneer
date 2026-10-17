/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_LAYER_PROOF_H
#define PIONEER_LAYER_PROOF_H
// Separate predicate: a fused graph must NEVER use the single-matrix proof.
static inline bool PioneerLayerSummary(const uint32_t* s,unsigned operation,unsigned limit,unsigned n,unsigned nonce,
	const uint32_t* before,const uint32_t* after)
{
	if (!s || !before || !after || !limit || limit>119 || !nonce || nonce>=0x7fffffff
		|| (operation!=4 && operation!=5) || !n || n>3072 || (n&31) || (operation==4 && n!=1024)) return false;
	unsigned pg=limit<96?limit:96,pd=limit<32?limit:32,p=operation==4?2*pg+pd:0;
	unsigned workers=operation==4?pg:1,minimum=4*p+1,r=s[0],rf=s[1],w=2*p+1,wf=2*p+n/16;
	if (s[4]!=workers || s[5]!=n || s[6]!=nonce || s[7]!=operation
		|| r<minimum || r>minimum+2*65535*p || ((r-minimum)&1)
		|| (!p && r!=1) || rf!=(p?r-p+511:n/16) || s[2]!=w || s[3]!=wf
		|| before[2]!=before[6] || after[2]!=after[6]) return false;
	const unsigned expected[]={r,rf,r+w,r,w,wf,r+w,w};
	for (unsigned i=0;i<8;i++) if (s[8+i]!=before[i] || s[16+i]!=after[i]
		|| uint32_t(after[i]-before[i])!=expected[i]) return false;
	return true;
}
#endif
