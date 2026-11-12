/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_UPLOAD_TIMING_H
#define PIONEER_Q38_UPLOAD_TIMING_H
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

// Diagnostic counters only. Clock/overflow problems invalidate measurements,
// never the underlying read or ioctl, and never grant execution authority.
struct PioneerQ38UploadTiming {
 using Clock=uint64_t (*)();
 Clock clock;uint64_t readNs=0,callNs=0,readCalls=0,callCalls=0,readBytes=0,callBytes=0;
 bool valid=true;
 explicit PioneerQ38UploadTiming(Clock value):clock(value){if(!clock)valid=false;}
 uint64_t Begin()const{return clock?clock():0;}
 void End(uint64_t start,uint64_t& elapsed,uint64_t& calls,uint64_t& bytes,uint64_t size){
  const uint64_t finish=clock?clock():0, max=std::numeric_limits<uint64_t>::max();
  if(!start || !finish || finish<start || elapsed>max-(finish-start) || calls==max || bytes>max-size){valid=false;return;}
  elapsed+=finish-start;calls++;bytes+=size;
 }
};

template<class Pages> class PioneerQ38TimedPages {
 Pages& pages;PioneerQ38UploadTiming& timing;
public:
 PioneerQ38TimedPages(Pages& source,PioneerQ38UploadTiming& measurement):pages(source),timing(measurement){}
 bool AllVerified()const{return pages.AllVerified();}
 auto Part(unsigned i)const -> decltype(pages.Part(i)){return pages.Part(i);}
 bool OutputSafe(const void* output,size_t bytes)const{
  const uintptr_t out=reinterpret_cast<uintptr_t>(output), self=reinterpret_cast<uintptr_t>(this);
  const uintptr_t meta=reinterpret_cast<uintptr_t>(&timing), max=std::numeric_limits<uintptr_t>::max();
  return output && bytes && out<=max-bytes && self<=max-sizeof(*this) && meta<=max-sizeof(timing)
   && !(out<self+sizeof(*this) && self<out+bytes) && !(out<meta+sizeof(timing) && meta<out+bytes)
   && pages.OutputSafe(output,bytes);
 }
 bool ReadPages(unsigned part,unsigned page,unsigned count,uint8_t* out,unsigned capacity,std::string& error){
  if(!count || count>capacity/4096 || !OutputSafe(out,size_t(count)*4096)){
   error="Timed page destination refused.";return false;
  }
  const uint64_t before=timing.Begin();const bool okay=pages.ReadPages(part,page,count,out,capacity,error);
  timing.End(before,timing.readNs,timing.readCalls,timing.readBytes,uint64_t(count)*4096);return okay;
 }
};
#endif
