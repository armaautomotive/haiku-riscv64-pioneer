/* SPDX-License-Identifier: MIT */
// Actual controller template/RV64 stack witness,NOT a native IO factory.
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_RESIDENT_DECODER_OWNER 1
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#include "pioneer_q38_projection_execution_kernel_compile.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentDecoderOwner.h"
struct DecoderCompileIO:ProjectionExecutionCompileIO {
 bool ResidentDecoderScope(const PioneerQ38ResidentDecoderOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ConvHistoryBanks&);
};
extern "C" bool pioneer_resident_decoder_compile(DecoderCompileIO& io,PioneerQ38ProjectionGraphOwner& projection,
 PioneerQ38ConvHistoryBanks& history,PioneerQ38ResidentDecoderOwner& decoder,PioneerQ38ResidentDecoderResult& result){
 return decoder.RunOnce(io,projection,history,result) && decoder.Close(io,result);
}
