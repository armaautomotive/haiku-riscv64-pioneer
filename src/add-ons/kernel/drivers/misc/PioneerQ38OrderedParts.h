/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ORDERED_PARTS_H
#define PIONEER_Q38_ORDERED_PARTS_H
#include "PioneerQ38CohortProtocol.h"
// Logical bounds ONLY,no file/device/initialized-page authority. Exact
// acceptance parity with pairwise overlap checking,including unsorted input.
// Normal private schedules advance monotonically within each bank. A new
// interval beyond EVERY previous end cannot overlap one;otherwise retain
// the complete original pairwise check. Seven high-water marks use56B.
static inline bool PioneerQ38OrderedParts(unsigned type,unsigned inputs,const tt_q38_cohort_part* parts,
	unsigned peers,unsigned* compared=nullptr)
{
	if (!parts || !peers || peers>119) return false;
	uint64_t high[7]={};
	for (unsigned w=0;w<119;w++) {
		const auto& p=parts[w];
		if (w>=peers) { if (p.bank || p.groups || p.offset) return false; continue; }
		if (!PioneerQ38CohortPartValid(type,inputs,p)) return false;
		uint64_t end=p.offset+PioneerQ38CohortBytes(type,inputs,p.groups);
		if (p.offset<high[p.bank]) for (unsigned other=0;other<w;other++) {
			if (compared) ++*compared; // offline diagnostics;constant-null in production
			const auto& q=parts[other];
			if (p.bank==q.bank && p.offset<q.offset+PioneerQ38CohortBytes(type,inputs,q.groups) && q.offset<end) return false;
		}
		if (end>high[p.bank]) high[p.bank]=end;
	}
	return true;
}
#endif
