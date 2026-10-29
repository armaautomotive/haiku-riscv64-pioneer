/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenViewLease.h"
// VM endpoint DATA tests,not native readiness/STOP/END authority. Full native
// history must separately reach the actual same-owner closure factory.
struct IO {
 PioneerQ38TokenViewLease lease;
 std::array<int32_t,29> views;
 std::array<int32_t,3> hosts={100,101,102};
 std::vector<int32_t> calls;
 unsigned fail=0,lose=0,mutate=0;
 bool eligible=true;
 IO(){for(unsigned i=0;i<views.size();i++)views[i]=int32_t(200+i);}
 bool TokenViewsScope(){return eligible;}
 bool TokenViewLeaseIdentity(const PioneerQ38TokenViewLease& views)const{return &views==&lease;}
 bool TokenViewCaptureScope(){return eligible;}
 int32_t* TokenViewHandle(unsigned i){return i<29?&views[i]:nullptr;}
 int32_t TokenHostView(unsigned i){return i<3?hosts[i]:-1;}
 bool TokenViewDelete(int32_t handle){
  calls.push_back(handle);const auto n=calls.size();
  if(lose==n)eligible=false;
  if(mutate==n)hosts[1]++;
  return fail!=n;
 }
};
int main(){
 unsigned cases=0;
 {
  IO io;PioneerQ38TokenViewLease foreign;
  assert(!foreign.Bind(io) && foreign.Pristine() && io.lease.Bind(io) && !foreign.Close(io)
   && io.lease.Prepared() && !foreign.Matches(io) && io.calls.empty());cases++;
 }
 {
  IO io;assert(io.lease.Pristine() && !io.lease.Matches(io) && !io.lease.Close(io));
  assert(io.lease.Bind(io) && io.lease.Prepared() && !io.lease.Bind(io));
  assert(io.lease.Close(io) && io.lease.Complete() && io.lease.Deleted()==29 && io.calls.size()==29 && io.lease.Matches(io));
  for(auto view:io.views)assert(view==-1);
  for(unsigned i=0;i<29;i++)assert(io.calls[i]==int32_t(200+i));
  assert(!io.lease.Close(io) && io.calls.size()==29);cases++;
 }
 {
  IO io;for(unsigned i=4;i<29;i+=2)io.views[i]=-1;
  assert(io.lease.Bind(io));
  assert(io.lease.Close(io) && io.lease.Complete() && io.lease.Deleted()==16 && io.calls.size()==16);cases++;
 }
 for(unsigned kind=0;kind<3;kind++)for(unsigned nth=1;nth<=29;nth++){
  IO io;if(kind==0)io.fail=nth;if(kind==1)io.lose=nth;if(kind==2)io.mutate=nth;
  assert(io.lease.Bind(io));
  assert(!io.lease.Close(io) && io.lease.Failed() && !io.lease.Complete() && io.calls.size()==nth);
  const unsigned successes=nth-(kind==0);
  assert(io.lease.Deleted()==successes);
  for(unsigned i=0;i<29;i++)assert(io.views[i]==(i<successes?-1:int32_t(200+i)));
  io.eligible=true;io.fail=io.lose=io.mutate=0;
  assert(!io.lease.Close(io) && io.calls.size()==nth);cases++;
 }
 for(unsigned kind=0;kind<6;kind++){
  IO io;
  if(kind==0)io.views[28]=io.hosts[2];if(kind==1)io.views[28]=io.views[1];
  if(kind==2)io.views[28]=-2;if(kind==3)io.hosts[2]=io.hosts[1];if(kind==4)io.hosts[2]=-1;if(kind==5)io.eligible=false;
  assert(!io.lease.Bind(io) && !io.lease.Close(io) && io.calls.empty() && !io.lease.Complete());
  if(kind<5)assert(io.lease.Failed() && !io.lease.Bind(io));cases++;
 }
 for(unsigned index=0;index<29;index++)for(int32_t bad:{-1,555}){
  IO io;assert(io.lease.Bind(io));io.views[index]=bad;
  assert(!io.lease.Matches(io) && !io.lease.Close(io) && io.calls.empty());cases++;
 }
 printf("PASS VM-handle journal %u cases:SAME private journal identity/prelaunch captured manifest/all29+sparse aliases/58 replaced-or-lost-handle refusals/each29 delete failures+post-delete eligibility loss+host mutation/whole-set duplicate+host+invalid preflight/NO following syscall or retry;OS endpoint DATA only,not native END or hardware qualification\n",cases);
}
