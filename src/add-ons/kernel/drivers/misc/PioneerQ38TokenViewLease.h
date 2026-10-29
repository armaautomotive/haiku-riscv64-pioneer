/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_VIEW_LEASE_H
#define PIONEER_Q38_TOKEN_VIEW_LEASE_H
#include <stdint.h>
// CPU VM-handle journal ONLY. The native factory supplies the genuine
// same-owner lifetime scope; this object cannot establish a clean card.
class PioneerQ38TokenViewLease {
 int32_t original[29]={},host[3]={};
 unsigned cursor=0,deleted=0;
 bool bound=false,attempted=false,complete=false,failed=false;
public:
 static constexpr unsigned Count=29;
 PioneerQ38TokenViewLease()=default;
 PioneerQ38TokenViewLease(const PioneerQ38TokenViewLease&)=delete;
 PioneerQ38TokenViewLease& operator=(const PioneerQ38TokenViewLease&)=delete;
 bool Pristine()const{return !bound && !attempted && !complete && !failed && !cursor && !deleted;}
 bool Prepared()const{return bound && !attempted && !complete && !failed && !cursor && !deleted;}
 bool Started()const{return attempted;}
 bool Complete()const{return attempted && complete && !failed && cursor==Count;}
 bool Failed()const{return failed;}
 unsigned Deleted()const{return deleted;}
 template<class IO> bool Matches(IO& io)const{
  if(!bound || failed || cursor>Count || !io.TokenViewLeaseIdentity(*this))return false;
  for(unsigned i=0;i<3;i++)if(host[i]<0 || io.TokenHostView(i)!=host[i])return false;
  for(unsigned i=0;i<Count;i++){
   const auto* view=io.TokenViewHandle(i);
   if(!view || *view!=(i<cursor?-1:original[i]))return false;
  }
  return true; // no syscall,device access or disposal permission
 }
 template<class IO> __attribute__((noinline)) bool Bind(IO& io){
  if(!Pristine() || !io.TokenViewLeaseIdentity(*this) || !io.TokenViewCaptureScope())return false;
  // Capture the ENTIRE set BEFORE native launch. Never adopt a replaced,
  // lost or newly absent area handle later during cleanup.
  for(unsigned i=0;i<3;i++){
   host[i]=io.TokenHostView(i);if(host[i]<0){failed=true;return false;}
   for(unsigned j=0;j<i;j++)if(host[j]==host[i]){failed=true;return false;}
  }
  for(unsigned i=0;i<Count;i++){
   const auto* view=io.TokenViewHandle(i);if(!view || *view< -1){failed=true;return false;}
   original[i]=*view;if(original[i]<0)continue;
   for(auto h:host)if(original[i]==h){failed=true;return false;}
   for(unsigned j=0;j<i;j++)if(original[j]==original[i]){failed=true;return false;}
  }
  if(!io.TokenViewCaptureScope()){failed=true;return false;}
  bound=true;return Matches(io);
 }
 template<class IO> __attribute__((noinline)) bool Close(IO& io){
  if(!Prepared() || !io.TokenViewLeaseIdentity(*this) || !io.TokenViewsScope() || !Matches(io))return false;
  attempted=true;
  for(;cursor<Count;cursor++){
   if(!io.TokenViewsScope() || !Matches(io)){failed=true;return false;}
   if(original[cursor]<0)continue;
   auto* view=io.TokenViewHandle(cursor);
   if(!io.TokenViewDelete(original[cursor])){failed=true;return false;}
   // Record the successful OS deletion before checking for lifetime loss.
   // This is bookkeeping,not a second syscall or an attempted repair.
   *view=-1;deleted++;
  }
  if(!io.TokenViewsScope() || !Matches(io)){failed=true;return false;}
  complete=true;return true;
 }
};
#endif
