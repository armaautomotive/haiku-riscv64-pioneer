/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_COORDINATED_GLU_PROOF_H
#define PIONEER_COORDINATED_GLU_PROOF_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// OFFLINE fixed 512-element GLU/Q8 protocol;no addresses or supplied code.
static inline bool PioneerCoordinatedGluProof(const volatile uint32_t* p,unsigned layer,unsigned group,bool pull=false)
{
	if (!p || layer>=28 || group>=6 || p[16]!=512 || p[17]!=layer || p[18]!=group || p[19]!=0x314c4741) return false;
	for (unsigned i=20;i<32;i++) if (p[i]!=unsigned(pull && i==20)) return false;
	if (p[2]!=p[6] || p[10]!=p[14]) return false;
	if (!pull) { for (unsigned i=0;i<8;i++) if (p[8+i]!=p[i]) return false; }
	else for (unsigned i=0;i<8;i++) if (uint32_t(p[8+i]-p[i])!=(i==1?128U:i<4 || i==6?4U:0U)) return false;
	return true;
}
#endif
