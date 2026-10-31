/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING) || PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING != 1
#error Exact separate tag75 USER generation timing selection required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_POSTED_CANCEL) || defined(PIONEER_Q38_POSTED_CANCEL_TEST)
#error Generation host clock DATA cannot alias old USER,kernel,native,CPU reference,Work or HOST transport TEST roles
#endif
#ifndef PIONEER_Q38_GENERATION_HOST_TIMING_H
#define PIONEER_Q38_GENERATION_HOST_TIMING_H
#include <cstdint>
#include <cstdio>

// Observational CPU wall time ONLY. Startup includes initialization/install;
// three positions include two prompts. NO accelerator cycles,decode rate,
// completion,ownership,cleanup,CPU model reference or Work capability.
class PioneerQ38GenerationHostTiming {
public:
 enum Bucket:unsigned {Upload,Startup,Observe,Cleanup,Other,Buckets};
 enum Mark:unsigned {Entry,DeviceOpen,StartReturned,TerminalObserved,CleanClosed,Returned,Marks};
 struct Sample {uint64_t calls=0,total=0,maximum=0;};
private:
 bool enabled=false,valid=true;int64_t previous=-1,marks[Marks]={};
 bool marked[Marks]={};Sample samples[Buckets];
 bool Clock(int64_t now){
  if(!valid)return false;
  if(now<0 || now<previous){valid=false;return false;}previous=now;return true;
 }
public:
 explicit PioneerQ38GenerationHostTiming(bool requested=false):enabled(requested){}
 bool Enabled()const{return enabled;}
 bool Valid()const{return valid;}
 const Sample& Get(Bucket bucket)const{return samples[bucket];}
 void At(Mark mark,int64_t now){
  if(!enabled)return;
  if(unsigned(mark)>=Marks || marked[mark] || !Clock(now)){valid=false;return;}
  marked[mark]=true;marks[mark]=now;
 }
 void Call(Bucket bucket,int64_t begin,int64_t end){
  if(!enabled)return;
  if(unsigned(bucket)>=Buckets || !Clock(begin) || !Clock(end)){valid=false;return;}
  const uint64_t elapsed=uint64_t(end-begin);auto& s=samples[bucket];
  if(s.calls==UINT64_MAX || elapsed>UINT64_MAX-s.total){valid=false;return;}
  s.calls++;s.total+=elapsed;if(elapsed>s.maximum)s.maximum=elapsed;
 }
 void Print(FILE* out)const{
  if(!enabled)return;
  std::fprintf(out,"DEBUG75_GENERATION_HOST_TIMING valid%u;HOST_WALL_ONLY_NOT_ACCELERATOR_CYCLES_OR_TOKEN_RATE\n",unsigned(valid));
  if(!valid)return;
  const char* names[]={"upload_ioctl","startup_ioctl","observe_ioctl","cleanup_ioctl","other_ioctl"};
  for(unsigned b=0;b<Buckets;b++)std::fprintf(out,"DEBUG75_GENERATION_HOST_IOCTL %s calls%llu total_us%llu max_us%llu\n",names[b],
   (unsigned long long)samples[b].calls,(unsigned long long)samples[b].total,(unsigned long long)samples[b].maximum);
  const char* phases[]={"source_to_device_open","device_open_to_start_return","start_return_to_terminal_observation","terminal_observation_to_clean_close","total_diagnostic"};
  const Mark first[]={Entry,DeviceOpen,StartReturned,TerminalObserved,Entry};
  const Mark last[]={DeviceOpen,StartReturned,TerminalObserved,CleanClosed,Returned};
  for(unsigned p=0;p<5;p++)if(marked[first[p]] && marked[last[p]] && marks[last[p]]>=marks[first[p]])
   std::fprintf(out,"DEBUG75_GENERATION_HOST_PHASE %s elapsed_us%llu\n",phases[p],(unsigned long long)(marks[last[p]]-marks[first[p]]));
 }
};
#endif
