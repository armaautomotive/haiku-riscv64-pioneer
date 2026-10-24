/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DECODER_CONV_BRIDGE_H
#define PIONEER_Q38_DECODER_CONV_BRIDGE_H
#include "pioneer_q38_projection_execution_machine.h"
class PioneerQ38CanonicalModelPlan;
struct DecoderHistoryBytes {
 virtual ~DecoderHistoryBytes()=default;
 virtual bool ReadHistory(unsigned bank,uint32_t address,uint32_t* out,unsigned words)=0;
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 virtual bool ReadGdnState(unsigned bank,uint32_t address,uint32_t* out,unsigned words)=0;
 virtual bool WriteGdnState(unsigned bank,uint32_t address,const uint32_t* data,unsigned words)=0;
#endif
}; // TEST device byte transport ONLY,never a current-state/native capability
// TEST emitted-instruction bridge ONLY. Reads SAME actual producer's GDDR
// bytes and the actual observed history-zero output;no numerical input or
// fabricated completion/producer capability accepted.
unsigned ExecuteDecoderConv(const char* image,const PioneerQ38CanonicalModelPlan&,unsigned worker,unsigned bank,
 ProjectionExecutionFabric&,DecoderHistoryBytes&);
#endif
