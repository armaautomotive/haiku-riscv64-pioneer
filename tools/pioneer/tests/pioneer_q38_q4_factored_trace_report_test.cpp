/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_Q4_FACTORED_TRACE_REPORT 1
#include "../PioneerQ38Q4FactoredTraceReport.h"
#include <cassert>
#include <string>
int main(){
 unsigned expected[64],actual[64];for(unsigned i=0;i<64;i++)expected[i]=actual[i]=0x3f000000+i;
 for(unsigned mismatch:{0U,31U,32U,52U,56U,63U,64U}){
  for(unsigned i=0;i<64;i++)actual[i]=expected[i];if(mismatch<64)actual[mismatch]^=1;
  FILE* file=tmpfile();assert(file);bool same=true;
  assert(PioneerQ38FactoredTraceReport(file,actual,expected,same) && same==(mismatch==64));
  rewind(file);std::string output;char line[256];unsigned words=0;
  while(fgets(line,sizeof(line),file)){
   output+=line;unsigned index;if(sscanf(line,"FACTORED_TRACE_WORD index%u",&index)==1){assert(index==words);words++;}
  }
  assert(words==64 && output.find("index63 kindfirst slot7")!=std::string::npos);
  assert(output.find("DATA_ONLY_NOT_END_OR_SPEED_AUTHORITY")!=std::string::npos && !fclose(file));
 }
 bool same=true;assert(!PioneerQ38FactoredTraceReport(nullptr,actual,expected,same) && !same);
 puts("PASS_TRACE_REPORT first_last_final_hash_intermediate_mismatch_and_equal7cases_ALL64_written_before_decision nullStreamFalse;NO_DEVICE_END_FALLBACK_OR_SPEED");
}
