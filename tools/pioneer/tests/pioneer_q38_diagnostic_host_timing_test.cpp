/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TOKEN_VALIDATION_CLIENT 1
#include "../PioneerQ38DiagnosticHostTiming.h"
#include <cassert>
#include <string>

using T=PioneerQ38DiagnosticHostTiming;
static std::string Printed(const T& timing){
 FILE* out=std::tmpfile();assert(out);timing.Print(out);assert(!std::ferror(out));
 assert(!std::fseek(out,0,SEEK_END));const long size=std::ftell(out);assert(size>=0 && size<8192);
 assert(!std::fseek(out,0,SEEK_SET));std::string result(size,'\0');
 assert(std::fread(result.data(),1,result.size(),out)==result.size());assert(!std::fclose(out));return result;
}
int main(){
 T disabled;disabled.At(T::Entry,-10);disabled.Call(T::Upload,7,-3);
 assert(!disabled.Enabled() && disabled.Valid() && Printed(disabled).empty());
 T normal(true);normal.At(T::Entry,10);normal.At(T::DeviceOpen,20);
 normal.Call(T::Other,20,21);normal.Call(T::Upload,21,40);normal.Call(T::Upload,41,43);
 normal.Call(T::Startup,44,50);normal.At(T::StartReturned,51);
 normal.Call(T::Observe,60,65);normal.Call(T::Observe,70,70);normal.At(T::TerminalObserved,75);
 normal.Call(T::Cleanup,76,80);normal.Call(T::Cleanup,80,82);normal.Call(T::Cleanup,83,90);
 normal.At(T::CleanClosed,91);normal.At(T::Returned,95);
 assert(normal.Valid() && normal.Get(T::Upload).calls==2 && normal.Get(T::Upload).total==21 && normal.Get(T::Upload).maximum==19);
 assert(normal.Get(T::Observe).calls==2 && normal.Get(T::Observe).total==5 && normal.Get(T::Observe).maximum==5);
 assert(normal.Get(T::Cleanup).calls==3 && normal.Get(T::Cleanup).total==13);
 const auto text=Printed(normal);
 for(const char* line:{"valid1;HOST_WALL_ONLY_NOT_ACCELERATOR_CYCLES_OR_TOKEN_RATE",
   "upload_ioctl calls2 total_us21 max_us19","source_to_device_open elapsed_us10",
   "device_open_to_start_return elapsed_us31","start_return_to_terminal_observation elapsed_us24",
   "terminal_observation_to_clean_close elapsed_us16","total_diagnostic elapsed_us85"})
  assert(text.find(line)!=std::string::npos);
 for(const char* forbidden:{"tokens_per_second","PASS_DEBUG74","clean_end1","accelerator_hz"})assert(text.find(forbidden)==std::string::npos);
 T incomplete(true);incomplete.At(T::Entry,0);incomplete.At(T::DeviceOpen,1);incomplete.At(T::Returned,2);
 assert(Printed(incomplete).find("total_diagnostic elapsed_us2")!=std::string::npos);
 assert(Printed(incomplete).find("terminal_observation_to_clean_close")==std::string::npos);
 unsigned refusals=0;
 for(unsigned failure=0;failure<9;failure++){
  T bad(true);bad.At(T::Entry,10);bad.Call(T::Upload,10,20);
  switch(failure){
   case 0:bad.Call(T::Upload,19,21);break; // regression across calls
   case 1:bad.Call(T::Upload,21,20);break; // regression within a call
   case 2:bad.At(T::DeviceOpen,-1);break;
   case 3:bad.At(T::Entry,21);break; // duplicate milestone
   case 4:bad.Call(T::Bucket(-1),21,22);break;
   case 5:bad.Call(T::Buckets,21,22);break;
   case 6:bad.At(T::Mark(-1),21);break;
   case 7:bad.At(T::Marks,21);break;
   case 8:bad.At(T::DeviceOpen,19);break;
  }
  assert(!bad.Valid());bad.Call(T::Upload,30,40);bad.At(T::Returned,50);assert(!bad.Valid());
  assert(Printed(bad)=="DEBUG74_HOST_TIMING valid0;HOST_WALL_ONLY_NOT_ACCELERATOR_CYCLES_OR_TOKEN_RATE\n");refusals++;
 }
 T large(true);large.At(T::Entry,0);large.Call(T::Upload,0,INT64_MAX);large.At(T::Returned,INT64_MAX);
 assert(large.Valid() && large.Get(T::Upload).total==uint64_t(INT64_MAX));
 std::printf("PASS USER host timing/disabled+zero+large clocks/5 ioctl buckets/5 wall phases/%u invalid clocks,indices,duplicate milestones suppressed;NO device,model,inference,completion,Work or token-rate proof\n",refusals);
}
