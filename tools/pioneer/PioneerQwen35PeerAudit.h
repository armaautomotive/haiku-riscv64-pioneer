/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PEER_AUDIT_H
#define PIONEER_QWEN35_PEER_AUDIT_H

// The Work-only combined job/doorbell occupies the slot the session already
// clears before boot. Its trailing sequence word is the peer wait signal;
// status starts immediately after the 64-byte slot.
enum {
	PioneerQ35WorkJobAddress=0x74100,
	PioneerQ35WorkJobBytes=64,
	PioneerQ35WorkJobSequenceAddress=0x7413c,
	PioneerQ35WorkPeerStatusAddress=0x74140
};

static inline bool PioneerQ35WorkSequenceValid(unsigned previous,unsigned sequence)
{
	return previous<0x7ffffffeU && sequence==previous+1 && sequence<0x7fffffffU;
}

static inline bool PioneerQ35WorkDescriptorSequenceValid(unsigned previous,
	unsigned sequence,const unsigned* descriptor)
{
	return descriptor && PioneerQ35WorkSequenceValid(previous,sequence)
		&& descriptor[12]==sequence && descriptor[15]==sequence;
}

// A resident Qwen35 Work session proves each projection's peer status,
// sequence, source and counters. Its physical PC/reset audit is retained at
// session admission and required again while closing with STOP. Checked work
// retains the full audit after every dispatch.
static inline bool PioneerQ35PeerPhysicalAuditRequired(bool work,bool closing)
{
	return !work || closing;
}

#endif
