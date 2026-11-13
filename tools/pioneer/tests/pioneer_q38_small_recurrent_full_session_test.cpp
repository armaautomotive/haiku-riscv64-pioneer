/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_QGW8_USER_SESSION_INSPECTION 1
#define PIONEER_Q38_SMALL_RECURRENT_SESSION_FIXTURE_ONLY 1
#include "pioneer_q38_small_recurrent_session_test.cpp"
#include "../PioneerQ38HybridFullSession.h"

struct FullIO:SessionIO {
 unsigned support=0,starts=0,observes=0;
 explicit FullIO(SessionIO source):SessionIO(source){}
 bool Call(int fd,unsigned op,void* data,size_t size){
  namespace F=PioneerQ38FullOffload;namespace P=PioneerQ38TokenControl;
  if(op==TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT){
   assert(fd==7 && owned && controls==2 && transfer.receipt.Sealed() && size==sizeof(F::Support));support++;
   *static_cast<F::Support*>(data)={55,1,F::MaxPrompt,F::MaxOutput,F::MaxPositions,248320,F::AllCompute,0};return true;
  }
  if(op!=TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL)return SessionIO::Call(fd,op,data,size);
  auto& p=*static_cast<P::Packet*>(data);assert(fd==7 && owned && controls==2 && transfer.receipt.Sealed()
   && size==sizeof(p) && F::RequestShape(p));
  if(p.request.operation==P::Observe){observes++;return false;}
  assert(p.request.operation==P::Start && p.request.prompts==1 && p.request.limit==1 && p.prompt[0]==17);
  starts++;auto& r=p.reply;r.version=P::Version;r.operation=P::Start;r.started=r.ready=1;r.stage=8;
  r.detail[0]=r.detail[1]=1;r.detail[2]=8;r.detail[3]=1;return true;
 }
};
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 FakePrepared prepared;assert(FakePrepared::Plan::Build(*canonical,prepared.proposal));
 std::vector<unsigned> order;for(unsigned i=0;i<canonical->Count();i++)if(canonical->Part(i)->kind==PioneerQ38CanonicalModelPlan::WeightKind)order.push_back(i);
 std::sort(order.begin(),order.end(),[&](unsigned a,unsigned b){const auto& x=prepared.proposal.parts[a];const auto& y=prepared.proposal.parts[b];
  return std::make_pair(x.bank,x.offset)<std::make_pair(y.bank,y.offset);});
 BanksMock backend;auto proof=BankProof(backend);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 auto receipt=std::make_unique<PioneerQ38SmallRecurrentUploadReceiptInspection>();int context=0,cookie=0;
 PioneerQ38SpanLease span;UploadIO transfer{*receipt,span,prepared,order,&context,hashes.data(),ledger.data(),0,0};
 FullIO io(SessionIO{transfer,banks,proof,&cookie});uint32_t prompt=17;unsigned progressCalls=0;uint64_t progressBytes=0;
 {PioneerQ38HybridFullSession<FakePrepared,FullIO> session(prepared,io);
  assert(session.Begin(*canonical,&prompt,1,1,[&](uint64_t bytes,uint64_t total){
   assert(bytes>progressBytes && bytes<=total && total==UINT64_C(20754878464));progressBytes=bytes;progressCalls++;
  }) && io.opens==1 && io.closes==0 && io.support==1
   && io.starts==1 && transfer.calls==21475 && io.controls==2 && io.spanEnds==1 && !io.bankEnds);
  assert(progressCalls==21475 && progressBytes==UINT64_C(20754878464));
  assert(!session.Poll() && session.FailedState() && io.observes==1 && io.closes==1 && !io.bankEnds);
  assert(!session.Poll() && io.observes==1 && io.closes==1);
 }
 assert(io.closes==1);
 puts("PASS full token START through same fake-identity sealed recurrent upload owner; uncertain OBSERVE closes once, no bank END; SIMULATED ONLY, NO TOKEN OR SPEED");
}
