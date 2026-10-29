/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_HOST_LEASE_H
#define PIONEER_Q38_TOKEN_HOST_LEASE_H
#include "PioneerQ38TokenViewLease.h"
#include "PioneerWideOutput.h"
// SAME private wired-host journal,not a clean GPU or caller-supplied safe
// hint. Capture before launch;native closure derives actual PCI/VM authority.
class PioneerQ38TokenHostLease {
 tt_host_page_result* input=nullptr;PioneerOutputBufferLease* bulk=nullptr;PioneerWideOutputLease* wide=nullptr;
 const PioneerQ38TokenViewLease* views=nullptr;
 int32_t area[3]={-1,-1,-1};uintptr_t pointer[3]={};uint64_t physical[3]={};
 unsigned cursor=0;bool captured=false,begun=false,closed=false,unsafe=false;
 static unsigned Bytes(unsigned kind){return kind==0?unsigned(TT_HOST_PAGE_BYTES):kind==1?unsigned(TT_OUTPUT_BUFFER_BYTES):unsigned(TT_WIDE_OUTPUT_BYTES);}
 bool Released(unsigned kind)const{return kind<3 && kind>=3-cursor;}
 bool Fail(){unsafe=true;return false;}
public:
 PioneerQ38TokenHostLease()=default;
 PioneerQ38TokenHostLease(const PioneerQ38TokenHostLease&)=delete;
 PioneerQ38TokenHostLease& operator=(const PioneerQ38TokenHostLease&)=delete;
 bool Pristine()const{return !input && !bulk && !wide && !views && !captured && !begun && !closed && !unsafe && !cursor;}
 bool Prepared()const{return captured && !begun && !closed && !unsafe && !cursor;}
 bool Closing()const{return captured && begun && !closed && !unsafe && cursor<=3;}
 bool Complete()const{return captured && begun && closed && !unsafe && cursor==3;}
 bool Unsafe()const{return unsafe;}
 unsigned ReleasedCount()const{return cursor;}
 void Fault(){unsafe=true;} // MEMORY ONLY;never retry or release another host
 bool PermitsRelease(unsigned kind)const{return captured && begun && !closed && !unsafe && cursor<3 && kind==2-cursor;}
 template<class IO> bool Matches(IO& io,const tt_host_page_result& h,const PioneerOutputBufferLease& b,
  const PioneerWideOutputLease& w,const PioneerQ38TokenViewLease& v)const{
  if(!captured || unsafe || cursor>3 || input!=&h || bulk!=&b || wide!=&w || views!=&v || !io.TokenHostLeaseIdentity(*this)
   || !io.TokenViewLeaseIdentity(v) || h.version!=1 || h.state!=TT_HOST_PAGE_READY || h.allocated!=1 || h.memory_type_set!=1
   || h.map_stable!=1 || h.patterns!=2 || h.mismatches || h.bytes!=TT_HOST_PAGE_BYTES || h.release_status
   || h.allocation_status || h.map_status || h.cache_status || h.released!=unsigned(Released(0)) || h.physical!=physical[0])return false;
  const PioneerOutputBufferLease* records[]={&b,&w};
  for(unsigned i=1;i<3;i++){
   const auto& r=*records[i-1];
   if(!r.allocated || !r.memory_type_set || r.pending || r.quarantined || r.state!=TT_OUTPUT_BUFFER_READY
    || r.patterns!=4 || r.mismatches || r.physical!=physical[i] || r.released!=Released(i) || r.ready==Released(i))return false;
  }
  for(unsigned i=0;i<3;i++){
   if(area[i]<0 || v.OriginalHost(i)!=area[i] || io.TokenHostView(i)!=(Released(i)?-1:area[i])
    || io.TokenHostPointer(i)!=(Released(i)?uintptr_t(0):pointer[i]) || io.TokenHostPhysical(i)!=physical[i]
    || io.TokenHostAttempted(i)!=Released(i) || io.TokenHostQuarantined(i))return false;
  }
  return true; // CPU metadata only,no HostPageMap/get_memory_map/page access
 }
 template<class IO> __attribute__((noinline)) bool Capture(IO& io,tt_host_page_result& h,PioneerOutputBufferLease& b,
  PioneerWideOutputLease& w,const PioneerQ38TokenViewLease& v){
  if(!Pristine() || !io.TokenHostLeaseIdentity(*this) || !io.TokenHostCaptureScope() || !v.Prepared() || !v.Matches(io)
   || !PioneerWideOutputDistinct(w.physical,h,b) || !h.allocated || !h.memory_type_set || h.released || h.version!=1
   || h.state!=TT_HOST_PAGE_READY || !h.map_stable || h.patterns!=2 || h.mismatches || h.bytes!=TT_HOST_PAGE_BYTES
   || h.release_status || h.allocation_status || h.map_status || h.cache_status
   || !w.allocated || !w.ready || !w.memory_type_set || w.released || w.pending || w.quarantined
   || w.state!=TT_OUTPUT_BUFFER_READY || w.patterns!=4 || w.mismatches
   || b.state!=TT_OUTPUT_BUFFER_READY || b.patterns!=4 || b.mismatches)return false;
  const uint64_t expected[]={h.physical,b.physical,w.physical};
  for(unsigned i=0;i<3;i++){
   area[i]=v.OriginalHost(i);pointer[i]=io.TokenHostPointer(i);physical[i]=io.TokenHostPhysical(i);
   if(area[i]<0 || io.TokenHostView(i)!=area[i] || !pointer[i] || pointer[i]>UINTPTR_MAX-Bytes(i)
    || physical[i]!=expected[i] || io.TokenHostAttempted(i) || io.TokenHostQuarantined(i))return Fail();
   for(unsigned j=0;j<i;j++)if(pointer[i]<pointer[j]+Bytes(j) && pointer[j]<pointer[i]+Bytes(i))return Fail();
  }
  if(!io.TokenHostCaptureScope())return Fail();
  input=&h;bulk=&b;wide=&w;views=&v;captured=true;return Matches(io,h,b,w,v) || Fail();
 } // original VM handle,virtual+physical identity and lease metadata BEFORE image installation/launch
 template<class IO> __attribute__((noinline)) bool Close(IO& io){
  if(!Prepared() || !io.TokenHostReleaseScope() || !views || !views->DeviceClosureMatches(io)
   || !Matches(io,*input,*bulk,*wide,*views))return false;
  begun=true;
  while(cursor<3){
   const unsigned kind=2-cursor;
   if(!io.TokenHostReleaseScope() || !views->DeviceClosureMatches(io) || !Matches(io,*input,*bulk,*wide,*views))return Fail();
   const int32_t status=io.TokenHostReleaseChecked(kind);
   if(status){
    if(!kind)input->release_status=status;
    else (kind==1?*bulk:*wide).state=TT_OUTPUT_BUFFER_RELEASE_FAILED;
    return Fail();
   }
   // Successful OS delete MUST be recorded even after immediate lifetime
   // loss;recording is CPU bookkeeping,never a second release or repair.
   if(!kind){input->release_status=0;input->released=1;}
   else{auto& r=kind==1?*bulk:*wide;r.ready=false;r.released=true;}
   cursor++;
  }
  if(!io.TokenHostReleaseScope() || !Matches(io,*input,*bulk,*wide,*views) || !views->DeviceClosureMatches(io))return Fail();
  closed=true;return true;
 } // wide->bulk->input,one attempt;first failure stops all following OS/device IO
};
#endif
