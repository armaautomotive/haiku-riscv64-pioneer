/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_STATE_H
#define PIONEER_Q38_NEXT_GDN_STATE_H
#include "PioneerQ38NextGdnProducer.h"
#include "PioneerQ38NextGdnRecurrentPlan.h"
#include "PioneerQ38GdnRendezvousPlan.h"
#include "PioneerQ38HostDeadline.h"
class PioneerQ38NextConvState;
// HEAP metadata only;SAME original snapshot and ALL native leases survive.
class PioneerQ38NextGdnState {
 friend class PioneerQ38ResidentDecoderOwner;
 PioneerQ38NextGdnRecurrentPlan jobs[96];PioneerQ38GdnRendezvousPlan gated[96];
 const PioneerQ38NextConvState* parent=nullptr;const PioneerQ38NextConvProducer* input=nullptr;
 uint32_t before[96][8]={},accepted[96][96]={},clock[96][16]={},polls[96]={};
 unsigned epoch=0,layer=64;bool image[120]={},staged[120]={},receipt[120]={},built=false,done=false;
 PioneerQ38NextGdnState()=default;
public:
 PioneerQ38NextGdnState(const PioneerQ38NextGdnState&)=delete;
 PioneerQ38NextGdnState& operator=(const PioneerQ38NextGdnState&)=delete;
};
static_assert(sizeof(PioneerQ38NextGdnState)>98304 && sizeof(PioneerQ38NextGdnState)<262144,"96complete recipes/receipts on HEAP,not numeric products or duplicate snapshots");
#endif
