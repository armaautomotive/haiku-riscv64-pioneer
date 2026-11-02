/* SPDX-License-Identifier: MIT */
// Immutable expected-ROM DATA only. No ISA engine,device or model products.
#ifdef PIONEER_Q38_PROGRAM_DATA_BRIDGE_TEST_TOKEN_PHASE
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS 1
#include "../PioneerQ38TokenPhaseProfilePrograms.h"
#elif defined(PIONEER_Q38_PROGRAM_DATA_BRIDGE_TEST_WIDE_GDN)
#define PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS 1
#include "../PioneerQ38TokenWideGdnPrograms.h"
#elif defined(PIONEER_Q38_PROGRAM_DATA_BRIDGE_TEST_OUTPUT_CLOSURE)
#define PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS 1
#include "../PioneerQ38TokenBatchedOutputClosurePrograms.h"
#else
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS 1
#include "../PioneerQ38TokenTwinBatchPrefetchPrograms.h"
#endif
#include "pioneer_q38_precise_beta_program_data_bridge.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
#ifdef PIONEER_Q38_PROGRAM_DATA_BRIDGE_TEST_TOKEN_PHASE
 namespace P=PioneerQ38TokenPhaseProfilePrograms;
 static_assert(P::TokenPhaseProfile==1 && P::WideGdn==1 && sizeof(P::PeerGdn)==35944,"actual new timing ROOT package ROM DATA");
#elif defined(PIONEER_Q38_PROGRAM_DATA_BRIDGE_TEST_WIDE_GDN)
 namespace P=PioneerQ38TokenWideGdnPrograms;
 static_assert(P::WideGdn==1 && sizeof(P::PeerGdn)==35944,"actual new returning-GDN ROM DATA");
#elif defined(PIONEER_Q38_PROGRAM_DATA_BRIDGE_TEST_OUTPUT_CLOSURE)
 namespace P=PioneerQ38TokenBatchedOutputClosurePrograms;
 static_assert(P::BatchedOutputClosure==1 && P::GlobalOutputReadsPerGroup==10,"corrected package ROM DATA");
#else
 namespace P=PioneerQ38TokenTwinBatchPrefetchPrograms;
#endif
 unsigned checked=0,refused=0;
 for(unsigned worker=1;worker<120;worker++){
  for(unsigned span=0;span<P::CodeSpans(worker);span++){
   const auto range=P::CodeSpan(worker,span);
   for(unsigned word=0;word<range.words;word++){
    const uint32_t at=range.address+word*4;uint32_t expected=0,actual=0;
    assert(P::ProgramWord(worker,at,expected) && Q38PreciseBetaTestProgramWord(worker,at,actual) && actual==expected);checked++;
   }
  }
  for(uint32_t at:{0U,0x6001U,0x6004U,0x98000U,0x9a000U,UINT32_MAX}){
   uint32_t output=0xa9e73c15;assert(!Q38PreciseBetaTestProgramWord(worker,at,output) && output==0xa9e73c15);refused++;
  }
 }
 for(unsigned worker:{0U,120U,UINT32_MAX})for(uint32_t at:{0x6000U,0x20000U,0x70000U}){
  uint32_t output=0xa9e73c15;assert(!Q38PreciseBetaTestProgramWord(worker,at,output) && output==0xa9e73c15);refused++;
 }
 assert(checked==119*P::CodeWords(1));
 printf("PASS twin-batch immutable expected-ROM bridge/ALL119 workers/%u code words/%u worker,alignment,gap+runtime-buffer refusals/unchanged output;NO ISA engine,native owner,hardware,model arithmetic,Work or TPS proof\n",checked,refused);
}
