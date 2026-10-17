/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_COORDINATED_ATTENTION_PROOF_H
#define PIONEER_COORDINATED_ATTENTION_PROOF_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// OFFLINE protocol prerequisite,not a public ABI. Group0..7 maps exactly
// to peers1..8. No addresses,code,host pointers or route selection here.
static inline bool PioneerCoordinatedAttentionProof(const volatile uint32_t* p,unsigned tokens,unsigned layer,unsigned group,bool pull=false,bool parallelRope=false)
{
	if (parallelRope && !pull) return false;
	if (!p || !tokens || tokens>256 || layer>=28 || group>=8 || p[16]!=tokens || p[17]!=layer || p[18]!=group || p[19]!=0x31475441) return false;
	for (unsigned i=20;i<24;i++) if (p[i]!=unsigned((pull && i==20) || (parallelRope && i==21))) return false;
	unsigned reads=2*((tokens+63)>>6)+(parallelRope?12:pull?4:0),writes=2;
	const unsigned delta[]={reads,8*tokens+(parallelRope?104:pull?48:0),reads+writes,reads,writes,8,reads+writes,writes};
	if (p[2]!=p[6] || p[10]!=p[14]) return false;
	for (unsigned i=0;i<8;i++) if (uint32_t(p[8+i]-p[i])!=delta[i]) return false;
	return true;
}
#endif
