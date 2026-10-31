/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT) || PIONEER_Q38_TOKEN_VALIDATION_CLIENT != 1
#error Host timing belongs only to the separate USER diagnostic client
#endif
#ifndef PIONEER_Q38_DIAGNOSTIC_HOST_TIMING_H
#define PIONEER_Q38_DIAGNOSTIC_HOST_TIMING_H
#include <cstdint>
#include <cstdio>

// Observational USER clock DATA only. An ioctl duration includes scheduling
// and transport,NOT accelerator cycles. In particular three processed
// positions (two prompt positions) are NOT three decode tokens. No rate,
// completion,ownership,retry,cleanup or Work authority comes from this data.
class PioneerQ38DiagnosticHostTiming {
public:
 enum Bucket:unsigned {Upload,Startup,Observe,Cleanup,Other,Buckets};
 enum Mark:unsigned {Entry,DeviceOpen,StartReturned,TerminalObserved,CleanClosed,Returned,Marks};
 struct Sample {uint64_t calls=0,total=0,maximum=0;};
private:
 bool enabled=false,valid=true;int64_t previous=-1,marks[Marks]={};
 bool marked[Marks]={};Sample samples[Buckets];
 bool Clock(int64_t now){
  if(!valid)return false;
  if(now<0 || now<previous){valid=false;return false;}
  previous=now;return true;
 }
public:
 explicit PioneerQ38DiagnosticHostTiming(bool requested=false):enabled(requested){}
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
  std::fprintf(out,"DEBUG74_HOST_TIMING valid%u;HOST_WALL_ONLY_NOT_ACCELERATOR_CYCLES_OR_TOKEN_RATE\n",unsigned(valid));
  if(!valid)return; // never publish plausible-looking partial clock data
  const char* names[]={"upload_ioctl","startup_ioctl","observe_ioctl","cleanup_ioctl","other_ioctl"};
  for(unsigned b=0;b<Buckets;b++)std::fprintf(out,"DEBUG74_HOST_IOCTL %s calls%llu total_us%llu max_us%llu\n",names[b],
   (unsigned long long)samples[b].calls,(unsigned long long)samples[b].total,(unsigned long long)samples[b].maximum);
  const char* phases[]={"source_to_device_open","device_open_to_start_return","start_return_to_terminal_observation","terminal_observation_to_clean_close","total_diagnostic"};
  const Mark first[]={Entry,DeviceOpen,StartReturned,TerminalObserved,Entry};
  const Mark last[]={DeviceOpen,StartReturned,TerminalObserved,CleanClosed,Returned};
  for(unsigned p=0;p<5;p++)if(marked[first[p]] && marked[last[p]] && marks[last[p]]>=marks[first[p]])
   std::fprintf(out,"DEBUG74_HOST_PHASE %s elapsed_us%llu\n",phases[p],(unsigned long long)(marks[last[p]]-marks[first[p]]));
 }
};
#endif
