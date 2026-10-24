/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_OUTPUT_PRODUCER_H
#define PIONEER_Q38_GDN_OUTPUT_PRODUCER_H
#if !defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE != 1 || !defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || PIONEER_Q38_GDN_RENDEZVOUS_NATIVE != 1 || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Separate native53 SAME-owner gated producer,not a DEBUG/reference/Work capability
#endif
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;
class PioneerQ38GdnOutputProjectionPlan;
class PioneerQ38CanonicalModelPlan;
// This object lives INSIDE the real decoder. Only that exact owner can
// construct/bind it,after ALL96 real receipts and ALL120 physical holds.
// Returned by const reference only;no caller ready flag,input epoch,vector
// or copied result can manufacture a producer. Current() is a CPU relation,
// NOT permission to read/launch/restore;the eventual consumer still owes
// the actual native call/context/source/clock/maps and physical guards.
class PioneerQ38GdnOutputProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 friend class PioneerQ38GdnOutputProjectionPlan;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,phase=0;
 uint32_t inputAddress=0,source[16]={};
 PioneerQ38GdnOutputProducer()=default;
public:
 PioneerQ38GdnOutputProducer(const PioneerQ38GdnOutputProducer&)=delete;
 PioneerQ38GdnOutputProducer& operator=(const PioneerQ38GdnOutputProducer&)=delete;
 bool Current()const;
};
#endif
