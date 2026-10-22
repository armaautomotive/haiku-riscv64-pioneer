/* SPDX-License-Identifier: MIT */
// Extracted ACTUAL output/Abort/Finish branch,ACTUAL source-bound session.
// Synthetic finite outputs and simulated native receipts,NOT GPU arithmetic.
#include "graph_fixture.inc"
static bool ClientFinish(bool okay,tt_q38_ffn_graph_run& request,GraphSession& session){
#include "client_finish.inc"
 return okay;
}
static void Fingerprints(tt_q38_ffn_graph_run& r){
 const uint32_t expected[]={0xf0e218bfU,0xbb087a1fU};
 static_assert(uint32_t(UINT64_C(16777619)*0x359c449bU)==1);
 for(unsigned pass=0;pass<2;pass++){
  for(auto& word:r.output[pass])word=0;
  uint32_t hash=2166136261U;for(unsigned i=0;i<5119;i++)hash*=16777619U;
  r.output[pass][5119]=(expected[pass]*0x359c449bU)^hash;
  assert((r.output[pass][5119]&0x7f800000U)!=0x7f800000U);
 }
}
int main(){
 (void)&Check;(void)&CohortHostTestMain;Fixture fixture;
 std::vector<tt_q38_ffn_graph_run> heap(1);auto& request=heap[0];
 for(unsigned failure=0;failure<4;failure++){
  FfnPrimeSource source(fixture.file);GraphSessionIO io(source,0);GraphSession session(io,source);
  InputRequest(request,0,2);
  assert(session.SelectFfnPrime(0) && session.Open() && session.PrimeSpan()
   && session.InitializeFfnWorkspace(0) && session.RunFfnGraph(0,request));
  Fingerprints(request);
  if(failure==1 || failure==2)request.output[failure-1][0]^=1;
  unsigned calls=io.graphs+io.spans+io.spanEnds+io.initializations;
  assert(ClientFinish(failure!=3,request,session)==!failure);
  assert(io.closes==1 && io.ends==unsigned(!failure) && !io.runs
   && calls==io.graphs+io.spans+io.spanEnds+io.initializations);
  if(failure)assert(session.Failed() && !session.Finish() && !io.ends && io.closes==1);
  else assert(session.Finish() && io.ends==1 && io.closes==1);
 }
 puts("PASS extracted actual profile-client finish branch:matching two prior-GPU fingerprints allow one END;either changed fingerprint or prior failure CLOSE ONLY/NO END/retry/extra call;ACTUAL session SIMULATED receipts/outputs NOT GPU or CPU-numeric equivalence");
}
