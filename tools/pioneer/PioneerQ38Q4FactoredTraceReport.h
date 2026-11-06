/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_Q4_FACTORED_TRACE_REPORT) || PIONEER_Q38_Q4_FACTORED_TRACE_REPORT!=1 || defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Separate TRACE data reporter only;never inference or cleanup authority
#endif
#ifndef PIONEER_Q38_Q4_FACTORED_TRACE_REPORT_H
#define PIONEER_Q38_Q4_FACTORED_TRACE_REPORT_H
#include <cstdio>
#include <stdint.h>
// Pure formatting of ALREADY returned words. No device,END,retry,fallback
// or model speed capability. Always print ALL64 before returning a mismatch.
static inline bool PioneerQ38FactoredTraceReport(FILE* stream,const uint32_t* actual,const uint32_t* expected,bool& allExact){
 allExact=false;if(!stream || !actual || !expected)return false;
 unsigned mismatches=0,first=64;
 if(fputs("FACTORED_TRACE_DATA_ONLY words64 final32_exact_required hashes_diagnostic_only;NOT_END_OR_SPEED_AUTHORITY\n",stream)<0)return false;
 for(unsigned i=0;i<64;i++){
  const char* kind=i<32?"final":i<52?"block":i<56?"stage":"first";
  const unsigned slot=i<32?i:i<52?i-32:i<56?i-52:i-56;
  const bool same=actual[i]==expected[i];if(!same){mismatches++;if(first==64)first=i;}
  if(fprintf(stream,"FACTORED_TRACE_WORD index%u kind%s slot%u got%08x expected%08x exact%u\n",i,kind,slot,actual[i],expected[i],unsigned(same))<0)return false;
 }
 if(fprintf(stream,"FACTORED_TRACE_DATA_DONE mismatches%u first%u;DATA_ONLY_NOT_END_OR_SPEED_AUTHORITY\n",mismatches,first)<0 || fflush(stream) || ferror(stream))return false;
 allExact=mismatches==0;return true;
}
#endif
