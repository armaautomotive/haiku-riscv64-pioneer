/* SPDX-License-Identifier: MIT */
// Actual authenticated-source strategy/session,simulated receipts ONLY.
#define PIONEER_Q38_FFN_PRIME_FIXTURE_ONLY
#include "pioneer_q38_ffn_prime_test.cpp"
struct ModelMode:PioneerQ38WorkSessionMode{uint64_t FfnModelGeneration()const{return 7;}};
struct ModelIO:FfnPrimeIO {
 using FfnPrimeIO::FfnPrimeIO;
 unsigned initializations=0,modelFailure=0;
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  if(op!=TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE)return FfnPrimeIO::Call(fd,op,data,bytes);
  assert(fd==73 && !stopped && spanEnds==1 && touched.size()==21 && !ends && !runs && !initializations++);
  auto& r=*static_cast<tt_q38_ffn_model_initialize*>(data);
  assert(bytes==sizeof(r) && r.version==1 && r.layer==selected && !r.reserved[0] && !r.reserved[1]
   && !r.state && !r.pages && !r.closed && !r.initialization_published && !r.recovery_required && !r.session && !r.driver_generation);
  r.state=TT_RUNTIME_PASSED;r.pages=74;r.closed=r.initialization_published=1;r.session=11;r.driver_generation=7;
  if(modelFailure==1){stopped=true;return false;}
  if(modelFailure==2)r.version++;if(modelFailure==3)r.layer^=1;if(modelFailure==4)r.reserved[0]=1;
  if(modelFailure==5)r.reserved[1]=1;if(modelFailure==6)r.state=TT_RUNTIME_COMPLETION_FAILED;
  if(modelFailure==7)r.pages--;if(modelFailure==8)r.closed=0;if(modelFailure==9)r.initialization_published=0;
  if(modelFailure==10)r.recovery_required=1;if(modelFailure==11)r.reserved_reply=1;if(modelFailure==12)r.session=0;
  if(modelFailure==13)r.driver_generation=0;if(modelFailure==14)r.driver_generation=8;
  if(modelFailure==15)actual.valid=false;
  if(modelFailure==16){assert(actual.model.Open(actual.original));actual.epoch=actual.model.Epoch();}
  if(modelFailure)stopped=true;
  return true;
 }
};
using ModelSession=PioneerQ38CohortSession<ModelIO,FfnPrimeSource,ModelMode>;
#ifndef PIONEER_Q38_FFN_MODEL_SESSION_FIXTURE_ONLY
int main(){
 (void)&Check;(void)&CohortHostTestMain;Fixture fixture;unsigned positive=0,faults=0;
 for(unsigned layer:{0U,31U,63U}){
  FfnPrimeSource source(fixture.file);ModelIO io(source,layer);ModelSession s(io,source);
  assert(s.SelectFfnPrime(layer) && s.Open() && s.PrimeSpan() && !s.FfnWorkspaceComplete(layer)
   && s.InitializeFfnWorkspace(layer) && s.FfnWorkspaceComplete(layer) && io.initializations==1
   && !io.runs && s.Finish() && io.ends==1 && !s.FfnWorkspaceComplete(layer));positive++;
 }
 for(unsigned fault=1;fault<=16;fault++){
  FfnPrimeSource source(fixture.file);ModelIO io(source,0);ModelSession s(io,source);io.modelFailure=fault;
  assert(s.SelectFfnPrime(0) && s.Open() && s.PrimeSpan() && !s.InitializeFfnWorkspace(0));
  unsigned calls=io.spans+io.spanEnds+io.initializations;
  assert(s.Failed() && !s.Finish() && !s.FfnWorkspaceComplete(0) && !s.InitializeFfnWorkspace(0)
   && io.closes==1 && !io.ends && !io.runs && calls==io.spans+io.spanEnds+io.initializations);faults++;
 }
 for(unsigned fault=0;fault<5;fault++){
  FfnPrimeSource source(fixture.file);ModelIO io(source,0);ModelSession s(io,source);
  if(fault){assert(s.SelectFfnPrime(0) && s.Open());if(fault!=1)assert(s.PrimeSpan());}
  if(fault==2)source.valid=false;
  if(fault==4){assert(s.InitializeFfnWorkspace(0));assert(!s.InitializeFfnWorkspace(0));}
  else assert(!s.InitializeFfnWorkspace(fault==3?1:0));
  assert(s.Failed() && !s.Finish() && !io.ends && !io.runs && io.initializations==(fault==4?1U:0U));faults++;
 }
 printf("PASS source-bound FFN model session %u positive/%u faults:SAME source/epoch/FD,one span-END before native init,exact generation/zero-CLOSE receipt,source checked AFTER native call,first loss CLOSE ONLY/no retry/END/legacy fallback;SIMULATED_NATIVE_RECEIPTS_NOT_COMPUTE_OR_SPEED\n",positive,faults);
}
#endif
