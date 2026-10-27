/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_ORIGINAL_WEIGHT_BYTES_H
#define PIONEER_Q38_RECURRENT_ORIGINAL_WEIGHT_BYTES_H
#include "PioneerQ38OriginalWeightBytes.h"
// Neutral immutable learned BYTES only. No native scenario,model/source,
// expected numeric vectors,execution,initialization or release authority.
struct RecurrentBlockRawLearnedBytes:ProjectionExecutionLearnedBytes {
 virtual bool Contains(unsigned,uint32_t,unsigned)const=0;
 virtual bool BeginRawPacket()=0;virtual bool EndRawPacket()=0;
 virtual bool Word(unsigned,uint32_t,uint32_t&)const=0;
};
#endif
