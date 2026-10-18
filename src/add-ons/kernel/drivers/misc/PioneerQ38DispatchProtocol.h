/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_PROTOCOL_H
#define PIONEER_Q38_DISPATCH_PROTOCOL_H
#include "PioneerQ38CohortProtocol.h"
// Experimental54 ABI. Native launch requires separate lifecycle/card
// qualification;the default and51..53 drivers do not handle these operations.
// Persistent WAIT is NOT the old per-job stopped=1 proof. Caller supplies
// logical DATA only;kernel alone derives all routes,addresses and nonces.
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_TRANSFER_DIAGNOSTIC
// Isolated58 transfer diagnostics;NO model-inference capability.
#define TT_PCI_PROBE_Q38_DISPATCH_SUPPORT (B_DEVICE_OP_CODES_END + 0x58f2)
#define TT_PCI_PROBE_Q38_DISPATCH_RUN (B_DEVICE_OP_CODES_END + 0x58f3)
enum { TT_Q38_DISPATCH_DRIVER=58,TT_Q38_DISPATCH_ABI=1,TT_Q38_DISPATCH_PEERS=119 };
#else
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_FAULT_RETURN
// Isolated57 diagnostics,NEVER a55/56 code substitution/model capability.
#define TT_PCI_PROBE_Q38_DISPATCH_SUPPORT (B_DEVICE_OP_CODES_END + 0x57f2)
#define TT_PCI_PROBE_Q38_DISPATCH_RUN (B_DEVICE_OP_CODES_END + 0x57f3)
enum { TT_Q38_DISPATCH_DRIVER=57,TT_Q38_DISPATCH_ABI=1,TT_Q38_DISPATCH_PEERS=119 };
#else
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_BFLOAT
// Separate56 lossless BF16 operand namespace,NEVER a54/55 code substitution.
#define TT_PCI_PROBE_Q38_DISPATCH_SUPPORT (B_DEVICE_OP_CODES_END + 0x56f2)
#define TT_PCI_PROBE_Q38_DISPATCH_RUN (B_DEVICE_OP_CODES_END + 0x56f3)
enum { TT_Q38_DISPATCH_DRIVER=56,TT_Q38_DISPATCH_ABI=1,TT_Q38_DISPATCH_PEERS=119 };
#else
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR
// Separate55 ABI namespace;NEVER selects new code under a54 caller/driver.
#define TT_PCI_PROBE_Q38_DISPATCH_SUPPORT (B_DEVICE_OP_CODES_END + 0x55f2)
#define TT_PCI_PROBE_Q38_DISPATCH_RUN (B_DEVICE_OP_CODES_END + 0x55f3)
enum { TT_Q38_DISPATCH_DRIVER=55,TT_Q38_DISPATCH_ABI=1,TT_Q38_DISPATCH_PEERS=119 };
#else
#define TT_PCI_PROBE_Q38_DISPATCH_SUPPORT (B_DEVICE_OP_CODES_END + 0x54f2)
#define TT_PCI_PROBE_Q38_DISPATCH_RUN (B_DEVICE_OP_CODES_END + 0x54f3)
enum { TT_Q38_DISPATCH_DRIVER=54,TT_Q38_DISPATCH_ABI=1,TT_Q38_DISPATCH_PEERS=119 };
#endif
#endif
#endif
#endif
struct tt_q38_dispatch_support { uint32_t driver,version,peers,rows,max_inputs,banks,offset_alignment,persistent,model_inference; };
struct tt_q38_dispatch_result {
	uint32_t version,state,peers,started,completed,waiting,recovery_required,root_sources_verified;
	uint32_t nonce,root_started,root_completed,root_held,root_phase,root_count,root_marker,root_pc[2];
	uint32_t before[8],after[8],stats[64],peer[TT_Q38_DISPATCH_PEERS][64];
};
struct tt_q38_dispatch_probe {
	uint32_t version,type,inputs,peers,reserved[4];
	tt_q38_cohort_part part[TT_Q38_DISPATCH_PEERS];
	uint32_t activation[TT_Q38_COHORT_INPUT_WORDS],scales[TT_Q38_COHORT_SCALE_WORDS];
	tt_q38_dispatch_result result;
	uint32_t output[TT_Q38_DISPATCH_PEERS][256];
};
static inline bool PioneerQ38DispatchValid(const tt_q38_dispatch_probe& r)
{
	if (r.version!=1 || !r.peers || r.peers>119 || !PioneerQ38PanelProbeBytes(r.type)
		|| (r.inputs!=5120 && r.inputs!=6144 && r.inputs!=17408)) return false;
	for (unsigned value:r.reserved) if (value) return false;
	for (unsigned w=0;w<119;w++) {
		const auto& p=r.part[w];
		if (w>=r.peers) { if (p.bank || p.groups || p.offset) return false; continue; }
		if (!PioneerQ38CohortPartValid(r.type,r.inputs,p)) return false;
		for (unsigned other=0;other<w;other++) {
			const auto& q=r.part[other];
			if (p.bank==q.bank && p.offset<q.offset+PioneerQ38CohortBytes(r.type,r.inputs,q.groups)
				&& q.offset<p.offset+PioneerQ38CohortBytes(r.type,r.inputs,p.groups)) return false;
		}
	}
	for (unsigned w=0;w<r.inputs/2;w++) if (!PioneerInt8OperandValid(uint16_t(r.activation[w]))
		|| !PioneerInt8OperandValid(uint16_t(r.activation[w]>>16))) return false;
	for (unsigned w=r.inputs/2;w<TT_Q38_COHORT_INPUT_WORDS;w++) if (r.activation[w]) return false;
	unsigned blocks=r.inputs/PioneerQ38CohortBlockInputs(r.type);
	for (unsigned b=0;b<blocks;b++) { unsigned m=r.scales[b]&0x7fffffff; if (m && (m<0x2f800000 || m>0x47800000)) return false; }
	for (unsigned b=blocks;b<TT_Q38_COHORT_SCALE_WORDS;b++) if (r.scales[b]) return false;
	return true;
}
#endif
