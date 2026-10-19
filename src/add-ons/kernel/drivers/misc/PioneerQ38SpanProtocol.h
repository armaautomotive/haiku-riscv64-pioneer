/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SPAN_PROTOCOL_H
#define PIONEER_Q38_SPAN_PROTOCOL_H
#include "PioneerQ38BankProtocol.h"
#include "PioneerResidentBanks.h"
#include "PioneerRuntimeMatvec.h"
// Separate opt-in pre-cohort transport. Logical bank/offset and inline DATA
// only; no caller address, route, code, nonce or physical host capability.
// SPAN_END restores its code before cohort handoff. BANK_END still MUST be
// explicit; close alone retains the whole context even after SPAN_END.
#define TT_PCI_PROBE_Q38_SPAN_SUPPORT (B_DEVICE_OP_CODES_END + 0x60f4)
#define TT_PCI_PROBE_Q38_SPAN_UPLOAD (B_DEVICE_OP_CODES_END + 0x60f5)
#define TT_PCI_PROBE_Q38_SPAN_END (B_DEVICE_OP_CODES_END + 0x60f6)
#define TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT (B_DEVICE_OP_CODES_END + 0x60f7)
#define TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD (B_DEVICE_OP_CODES_END + 0x60f8)
enum { TT_Q38_SPAN_PUBLIC_VERSION=1,TT_Q38_SPAN_PUBLIC_BYTES=1048576 };
struct tt_q38_span_support {
	uint32_t driver,version,banks,page_bytes,max_bytes,before_cohort,explicit_end,reserved;
};
struct tt_q38_span_result {
	uint32_t version,state,bytes,pages,nonce,started,completed,held,host_exact,matched_words,recovery_required,reserved;
};
struct tt_q38_span_upload {
	uint32_t version,bank,bytes,reserved;
	uint64_t offset;
	tt_q38_span_result result;
	uint32_t data[TT_Q38_SPAN_PUBLIC_BYTES/4];
};
struct tt_q38_span_end {
	uint32_t version,reserved,closed,recovery_required;
};
static inline bool PioneerQ38SpanPublicShape(unsigned version,unsigned bank,uint64_t offset,unsigned bytes,unsigned reserved)
{
	return version==TT_Q38_SPAN_PUBLIC_VERSION && !reserved && bank<7 && bytes && bytes<=TT_Q38_SPAN_PUBLIC_BYTES
		&& !(bytes&4095) && !(offset&4095) && offset<PioneerResidentBanks::Capacity && bytes<=PioneerResidentBanks::Capacity-offset;
}
#endif
