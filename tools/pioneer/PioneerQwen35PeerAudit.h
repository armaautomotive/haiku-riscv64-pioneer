/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PEER_AUDIT_H
#define PIONEER_QWEN35_PEER_AUDIT_H

// A resident Qwen35 Work session proves each projection's peer status,
// sequence, source and counters. Its physical PC/reset audit is retained at
// session admission and required again while closing with STOP. Checked work
// retains the full audit after every dispatch.
static inline bool PioneerQ35PeerPhysicalAuditRequired(bool work,bool closing)
{
	return !work || closing;
}

#endif
