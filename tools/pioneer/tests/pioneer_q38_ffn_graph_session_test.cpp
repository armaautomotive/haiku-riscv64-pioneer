/* SPDX-License-Identifier: MIT */
// ACTUAL source/session/namespace9 policy;SIMULATED native receipts only.
#define PIONEER_Q38_FFN_MODEL_SESSION_FIXTURE_ONLY
#include "pioneer_q38_ffn_model_session_test.cpp"
struct GraphSessionIO:ModelIO {
 using ModelIO::ModelIO;
 unsigned graphs=0,graphFailure=0;
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  if(op!=TT_PCI_PROBE_Q38_FFN_GRAPH_RUN)return ModelIO::Call(fd,op,data,bytes);
  auto& r=*static_cast<tt_q38_ffn_graph_run*>(data);
  assert(fd==73 && bytes==sizeof(r) && !stopped && spanEnds==1 && initializations==1 && !ends && !runs && !graphs++
   && r.layer==selected && PioneerQ38FfnGraphRequest(r));
  r.state=TT_RUNTIME_PASSED;r.closed=1;r.session=11;r.driver_generation=7;
  r.installed_words=r.restored_words=867448;r.peer_boots=119;r.root_starts=r.runs+1;
  for(unsigned pass=0;pass<r.runs;pass++){r.elapsed_us[pass]=100+pass;for(unsigned i=0;i<5120;i++)r.output[pass][i]=0x3f000000+i+pass;}
  if(graphFailure==1){stopped=true;return false;}
  if(graphFailure==2)r.version++;if(graphFailure==3)r.layer^=1;if(graphFailure==4)r.runs^=1;
  if(graphFailure==5)r.reserved=1;if(graphFailure==6)r.reserved_reply=1;if(graphFailure==7)r.state=TT_RUNTIME_COMPLETION_FAILED;
  if(graphFailure==8)r.closed=0;if(graphFailure==9)r.recovery_required=1;if(graphFailure==10)r.session=12;
  if(graphFailure==11)r.driver_generation=8;if(graphFailure==12)r.installed_words--;if(graphFailure==13)r.restored_words--;
  if(graphFailure==14)r.peer_boots--;if(graphFailure==15)r.root_starts--;if(graphFailure==16)r.elapsed_us[0]=0;
  if(graphFailure==17)r.elapsed_us[1]=0;if(graphFailure==18)r.output[0][5119]=0x7fc00001;
  if(graphFailure==19)r.output[1][5119]=0xff800000;if(graphFailure==20)actual.valid=false;
  if(graphFailure==21){assert(actual.model.Open(actual.original));actual.epoch=actual.model.Epoch();}
  if(graphFailure)stopped=true;
  return true;
 }
};
using GraphSession=PioneerQ38CohortSession<GraphSessionIO,FfnPrimeSource,ModelMode>;
static void InputRequest(tt_q38_ffn_graph_run& r,unsigned layer,unsigned runs){
 r={};r.version=1;r.layer=layer;r.runs=runs;
 for(unsigned pass=0;pass<runs;pass++)for(unsigned i=0;i<5120;i++)r.input[pass][i]=0x3f800000+pass+i;
 assert(PioneerQ38FfnGraphRequest(r));
}
int main(){
 (void)&Check;(void)&CohortHostTestMain;Fixture fixture;unsigned positive=0,faults=0,inert=0;
 std::vector<tt_q38_ffn_graph_run> heap(1);auto& r=heap[0];
 for(unsigned count:{1U,2U}){
  FfnPrimeSource source(fixture.file);GraphSessionIO io(source,0);GraphSession s(io,source);InputRequest(r,0,count);
  assert(s.SelectFfnPrime(0) && s.Open() && s.PrimeSpan() && s.InitializeFfnWorkspace(0)
   && s.RunFfnGraph(0,r) && io.graphs==1 && !io.runs && s.Finish() && io.ends==1);positive++;
 }
 for(unsigned failure=1;failure<=21;failure++){
  FfnPrimeSource source(fixture.file);GraphSessionIO io(source,0);GraphSession s(io,source);InputRequest(r,0,2);
  assert(s.SelectFfnPrime(0) && s.Open() && s.PrimeSpan() && s.InitializeFfnWorkspace(0));io.graphFailure=failure;
  assert(!s.RunFfnGraph(0,r));unsigned calls=io.graphs+io.spans+io.initializations+io.spanEnds;
  assert(s.Failed() && !s.RunFfnGraph(0,r) && !s.Finish() && io.closes==1 && !io.ends && !io.runs
   && calls==io.graphs+io.spans+io.initializations+io.spanEnds);faults++;
 }
 for(unsigned which=0;which<23;which++){
  FfnPrimeSource source(fixture.file);GraphSessionIO io(source,0);GraphSession s(io,source);InputRequest(r,0,2);
  assert(s.SelectFfnPrime(0) && s.Open() && s.PrimeSpan() && s.InitializeFfnWorkspace(0));
  if(which==0)r.version=0;if(which==1)r.layer=64;if(which==2)r.runs=0;if(which==3)r.runs=3;
  if(which==4)r.reserved=1;if(which==5)r.state=1;if(which==6)r.closed=1;if(which==7)r.recovery_required=1;
  if(which==8)r.reserved_reply=1;if(which==9)r.session=11;if(which==10)r.driver_generation=7;
  if(which==11)r.elapsed_us[0]=1;if(which==12)r.elapsed_us[1]=1;if(which==13)r.installed_words=1;
  if(which==14)r.restored_words=1;if(which==15)r.peer_boots=1;if(which==16)r.root_starts=1;
  if(which==17)r.input[0][0]=0x7f800000;if(which==18)r.input[1][5119]=0x7fc00001;
  if(which==19)r.output[0][0]=1;if(which==20)r.output[1][5119]=1;
  if(which==21){r.runs=1;}if(which==22)source.valid=false;
  assert(!s.RunFfnGraph(0,r) && !io.graphs && !s.Finish() && io.closes==1 && !io.ends && !io.runs);inert++;
 }
 printf("PASS source-bound persistent FFN graph session %u positive/%u lost-or-invalid native/source receipts/%u before-launch refusals:SAME FD/source/epoch/zero session/generation,full finite F32 boundary,one isolated namespace9 call then explicit END;first uncertainty CLOSE ONLY/no retry/END/fallback;SIMULATED_RECEIPTS_NOT_NATIVE_ARITHMETIC_OR_SPEED\n",positive,faults,inert);
}
