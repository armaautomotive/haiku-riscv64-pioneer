/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_STATE_H
#define PIONEER_Q38_NEXT_GDN_OUTPUT_STATE_H
#include "PioneerQ38NextGdnOutputBootstrapPlan.h"
#include "PioneerQ38NextGdnOutputGraphProof.h"
#include "PioneerQ38NextGdnOutputImages.h"
#include "PioneerQ38NextGdnOutputProducer.h"
#include "PioneerQ38HostDeadline.h"
class PioneerQ38NextGdnState;
// SAME actual decoder retains recipes/receipts;no new snapshots/products.
class PioneerQ38NextGdnOutputState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38NextGdnOutputProjectionPlan jobs;PioneerQ38NextGdnOutputBootstrapPlan bootstrap;
 const PioneerQ38NextGdnState* parent=nullptr;const PioneerQ38NextGdnProducer* input=nullptr;
 uint32_t sequence[119]={},before[8]={},accepted[64]={};
 bool image[120]={},held[120]={},stopReceipt[120]={};
 bool bootPrepared=false,rootProof=false,rootHeld=false,stopping=false,done=false,stopped=false;
 PioneerQ38HostDeadline bootDeadline,runDeadline,stopDeadline;PioneerQ38HostDeadline* active=nullptr;
 PioneerQ38NextGdnOutputState()=default;
public:
 PioneerQ38NextGdnOutputState(const PioneerQ38NextGdnOutputState&)=delete;
 PioneerQ38NextGdnOutputState& operator=(const PioneerQ38NextGdnOutputState&)=delete;
};
static_assert(sizeof(PioneerQ38NextGdnOutputState)>65536 && sizeof(PioneerQ38NextGdnOutputState)<81920,"canonical recipes/receipts HEAP ONLY");
#endif
