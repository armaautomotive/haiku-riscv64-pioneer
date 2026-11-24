/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_VERIFIED_SPAN_H
#define PIONEER_QWEN35_VERIFIED_SPAN_H

#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"

namespace PioneerQwen35 {
// Model loading may use host memory for byte verification; it does not run
// model arithmetic. A Work upload omits the device/host echo and must never
// be accepted as a verified span.
inline bool VerifiedSpanSupport(const tt_q38_span_support& span) {
    return span.driver==55 && span.version==TT_Q38_SPAN_PUBLIC_VERSION && span.banks==7
        && span.page_bytes==4096 && span.max_bytes==TT_Q38_SPAN_PUBLIC_BYTES
        && span.before_cohort==1 && span.explicit_end==1 && !span.reserved;
}
inline bool VerifiedSpanReply(const tt_q38_span_upload& packet,unsigned bank,uint64_t offset,
    unsigned bytes,unsigned nonce) {
    const auto& result=packet.result;
    return nonce && PioneerQ38SpanPublicShape(packet.version,bank,offset,bytes,packet.reserved)
        && packet.bank==bank && packet.offset==offset && packet.bytes==bytes
        && result.version==1 && result.state==TT_RUNTIME_PASSED
        && result.bytes==bytes && result.pages==bytes/4096 && result.nonce==nonce
        && result.started==1 && result.completed==1 && result.held==1
        && result.host_exact==1 && result.matched_words==bytes/4
        && !result.recovery_required && !result.reserved;
}
}
#endif
