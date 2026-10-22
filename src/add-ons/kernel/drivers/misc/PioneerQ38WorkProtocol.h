/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WORK_PROTOCOL_H
#define PIONEER_Q38_WORK_PROTOCOL_H
#include "PioneerQ38DispatchProof.h"
#ifdef PIONEER_Q38_ROOT_CYCLE_PROFILE
#include "PioneerQ38RootCycleData.h"
#endif
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
#ifndef PIONEER_Q38_WORK_OUTPUT_DMA
#error Compact output requires the owned Work DMA protocol
#endif
#include "PioneerQ38CompactOutput.h"
#endif
// Explicit WORK interface. Old drivers reject these operations;there is NO
// silent fallback to the exhaustive checked path. Same logical request and
// owned output ranges,NO user physical addresses/code/nonce authority.
#define TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT (B_DEVICE_OP_CODES_END + 0x60f2)
#define TT_PCI_PROBE_Q38_LEGACY_WORK_RUN (B_DEVICE_OP_CODES_END + 0x60f3)
#if defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT)
#if (defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC) && (PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC))) || (defined(PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT) && (PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT != 1 || !defined(PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT))) || (defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC) && defined(PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT))
#error Worker lifetime qualification requires its exact isolated diagnostic OR reference role
#endif
#endif
#if defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT)
#if (defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) && PIONEER_Q38_FFN_CODE_DIAGNOSTIC != 1) || (defined(PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT) && PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT != 1) || (defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) && defined(PIONEER_Q38_FFN_CODE_REFERENCE_CLIENT)) || defined(PIONEER_Q38_PEER_PHASE_QUALIFICATION) || defined(PIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_NATIVE_Q8_QUALIFICATION) || defined(PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT)
#error Held-only FFN code qualification has its own exact isolated protocol role
#endif
// Ordinary Work/model/Assistant operations cannot accidentally invoke the
// never-launched code-footprint diagnostic or treat it as a promoted target.
#if defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT)
// New workers EXECUTE boot+STOP;the older code-only contract NEVER did.
// Do not relabel namespace5 or permit its old clients to invoke this role.
#define TT_PCI_PROBE_Q38_WORK_SUPPORT (B_DEVICE_OP_CODES_END + 0x6280)
#define TT_PCI_PROBE_Q38_WORK_RUN (B_DEVICE_OP_CODES_END + 0x6281)
#define TT_Q38_WORK_SUPPORT_VERSION 6
#else
#define TT_PCI_PROBE_Q38_WORK_SUPPORT (B_DEVICE_OP_CODES_END + 0x6270)
#define TT_PCI_PROBE_Q38_WORK_RUN (B_DEVICE_OP_CODES_END + 0x6271)
#define TT_Q38_WORK_SUPPORT_VERSION 5
#endif
static_assert(TT_PCI_PROBE_Q38_WORK_SUPPORT!=TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT
	&& TT_PCI_PROBE_Q38_WORK_RUN!=TT_PCI_PROBE_Q38_LEGACY_WORK_RUN,"code qualification stays isolated");
#elif defined(PIONEER_Q38_PEER_PHASE_QUALIFICATION) || defined(PIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT)
#if defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_NATIVE_Q8_QUALIFICATION) || defined(PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT)
#error Private timing DATA ABI must never mix with SM7 SOURCE ABI
#endif
#include "PioneerQ38PeerPhaseData.h"
#define PIONEER_Q38_PEER_PHASE_PROTOCOL
// Separate private timing DATA transport. Old ordinary Work/model clients
// cannot accidentally invoke this root or accept its changed counter shape.
#define TT_PCI_PROBE_Q38_WORK_SUPPORT (B_DEVICE_OP_CODES_END + 0x6260)
#define TT_PCI_PROBE_Q38_WORK_RUN (B_DEVICE_OP_CODES_END + 0x6261)
#define TT_Q38_WORK_SUPPORT_VERSION 4
static_assert(TT_PCI_PROBE_Q38_WORK_SUPPORT!=TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT
	&& TT_PCI_PROBE_Q38_WORK_RUN!=TT_PCI_PROBE_Q38_LEGACY_WORK_RUN,"timing DATA namespace stays distinct");
#elif defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_NATIVE_Q8_QUALIFICATION) || defined(PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT)
// New lossless SM7 SOURCE encoding MUST NOT share ordinary TC Work ops.
// Private-source producer/qualifier only;NOT a fallback or ABI relabelling.
#define TT_PCI_PROBE_Q38_WORK_SUPPORT (B_DEVICE_OP_CODES_END + 0x6250)
#define TT_PCI_PROBE_Q38_WORK_RUN (B_DEVICE_OP_CODES_END + 0x6251)
#define TT_Q38_WORK_SUPPORT_VERSION 3
static_assert(TT_PCI_PROBE_Q38_WORK_SUPPORT!=TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT
	&& TT_PCI_PROBE_Q38_WORK_RUN!=TT_PCI_PROBE_Q38_LEGACY_WORK_RUN,"source-format namespaces MUST remain distinct");
#else
#define TT_PCI_PROBE_Q38_WORK_SUPPORT TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT
#define TT_PCI_PROBE_Q38_WORK_RUN TT_PCI_PROBE_Q38_LEGACY_WORK_RUN
#define TT_Q38_WORK_SUPPORT_VERSION 2 // explicit END;close alone NEVER runs device cleanup
#endif
// Work trusts the immutable root's ON-CARD peer/input/numeric checks. CPU
// keeps only root completion/source/counter/held-state SAFETY bookkeeping.
// It does NOT read/validate peer proof records,expanded input or output
// numbers. This deliberately cannot detect an otherwise well-formed wrong
// numeric result;use the separate CPU-validated/debug mode for that.
// Optional WORK_SKIP_DOT_CHECK also lets peers rely on the proven fixed
// integer range instead of reading every numeric dot back to their RISC.
// The root derives that private selector;checked/STOP always keep it off.
static inline bool PioneerQ38WorkSafety(const tt_q38_dispatch_probe& job,
	const PioneerQ38DispatchAuthority& a,bool finalHeld=true)
{
	if (!job.peers || job.peers>119 || !a.nextRoot || a.nextRoot>=0x7fffffff) return false;
	const auto& r=job.result;
	if (r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.peers!=job.peers || r.started!=job.peers || r.completed!=job.peers
		|| r.waiting!=job.peers || r.recovery_required || r.root_sources_verified!=1 || r.nonce!=a.nextRoot
		|| r.root_started!=1 || r.root_completed!=1 || r.root_held!=unsigned(finalHeld) || r.root_phase!=0x7200c || r.root_count!=job.peers
		|| r.root_marker!=0x314d3851 || !PioneerQ38RootTerminalPc(r.root_pc[0]) || !PioneerQ38RootTerminalPc(r.root_pc[1])) return false;
	const auto* s=r.stats;
	if (s[0]!=0x314d3851 || s[1]!=1 || s[2]!=1 || s[3]!=job.type || s[4]!=job.inputs || s[5]!=job.peers
		|| s[12]!=a.nextRoot || s[13] || s[14] || s[15]
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
		|| s[10]<job.peers || s[11]<job.peers
#else
		|| s[10]<2*job.peers || s[11]<2*job.peers
#endif
		|| s[11]>s[10]
		|| s[10]>2*65536*job.peers) return false;
	unsigned groups=0;
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
	uint32_t packedWords=0;
#endif
	for (unsigned w=0;w<job.peers;w++) {
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
		if (!PioneerQ38CompactOutputAppend(job.part[w].groups,packedWords)) return false;
#endif
		groups+=job.part[w].groups;
	}
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
	uint32_t reads=s[10]+3*s[11]+job.peers,flits=s[10]+3*s[11]+4*job.peers+groups,writes=2*job.peers;
#else
	uint32_t reads=s[10]+3*s[11]+2*job.peers,flits=s[10]+3*s[11]+5*job.peers+groups,writes=2*job.peers;
#endif
#ifdef PIONEER_Q38_PEER_PHASE_PROTOCOL
	// One completed fixed64B private DATA read AFTER every peer proof/output.
	reads++;flits++;
#endif
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
	uint32_t writeFlits=writes;
	uint32_t bytes=
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
		PioneerQ38CompactOutputBytes(job.peers,packedWords),magic=PIONEER_Q38_COMPACT_OUTPUT_MAGIC,
#else
		job.peers*1024,magic=0x314f3851,
#endif
		chunks=(bytes+4095)/4096;
	if (s[49]!=magic || s[50]!=bytes || s[51]!=a.nextRoot || s[52]!=chunks || s[53]!=chunks
		|| s[54]!=bytes/64 || s[55]!=bytes/64 || s[56]!=1) return false;
	reads+=chunks; flits+=bytes/64; writes+=chunks; writeFlits+=bytes/64;
	if (s[6]!=reads || s[7]!=writes || s[8]!=flits || s[9]!=writeFlits) return false;
#else
	if (s[6]!=reads || s[7]!=writes || s[8]!=flits || s[9]!=writes) return false;
#endif
	for (unsigned c=0;c<8;c++) {
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
		uint32_t expected=c==0 || c==3?reads:c==1?flits:c==2 || c==6?reads+writes:c==5?writeFlits:writes;
#else
		uint32_t expected=c==0 || c==3?reads:c==1?flits:c==2 || c==6?reads+writes:writes;
#endif
		if (r.before[c]!=a.before[c] || r.after[c]!=a.after[c] || s[16+c]!=a.before[c] || s[24+c]!=a.after[c]
			|| uint32_t(a.after[c]-a.before[c])!=expected) return false;
	}
	for (unsigned i=0;i<16;i++) if (s[32+i]!=a.source[0][i]) return false;
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
	if (s[48]!=1) return false;
#ifdef PIONEER_Q38_PEER_PHASE_PROTOCOL
	if (!PioneerQ38PeerPhaseEnvelopeValid(s)) return false;
#elif defined(PIONEER_Q38_ROOT_CYCLE_PROFILE)
	// Separate exact root image writes ONLY this previously zero padding.
	// ALL source/counter/nonce/PC/DMA/held proofs above remain unchanged.
	if (!PioneerQ38RootCycleDataValid(s)) return false;
#else
	for (unsigned i=
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
		57
#else
		49
#endif
		;i<64;i++) if (s[i]) return false;
#endif
#else
	for (unsigned i=48;i<64;i++) if (s[i]) return false;
#endif
	return true;
}
#endif
