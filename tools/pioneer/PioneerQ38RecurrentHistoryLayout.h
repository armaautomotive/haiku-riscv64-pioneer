/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_HISTORY_LAYOUT_H
#define PIONEER_Q38_RECURRENT_HISTORY_LAYOUT_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// Immutable geometry ONLY. A future native full-token owner must reserve
// these workspace bytes and prove each preceding Conv producer. Staging
// history is NOT publication of current history, a slot flip or token commit.
namespace PioneerQ38RecurrentHistoryLayout {
enum {Marker=0x31485351,Offset=0x800000,Stride=0x20000,Bytes=10240*3*4,Layers=48};
static inline bool Address(uint32_t workspace,unsigned layer,uint32_t& output){
 if((workspace&4095) || workspace<0x04000000 || workspace>0xc0000000 || layer>=64 || layer%4==3)return false;
 const unsigned ordinal=layer-layer/4;
 output=workspace+Offset+ordinal*Stride;return true;
}
static_assert(Bytes<=Stride && Offset+Layers*Stride<=67108864,"all48 next histories fit disjoint private workspace slots");
}
#endif
