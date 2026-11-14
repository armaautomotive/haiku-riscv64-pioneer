/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_QGW8_USER_SESSION_INSPECTION 1
#define PIONEER_Q38_SMALL_RECURRENT_SESSION_FIXTURE_ONLY 1
#include "pioneer_q38_small_recurrent_session_test.cpp"
#include "../PioneerQ38HybridFullSession.h"

struct FullIO:SessionIO {
 unsigned support=0,starts=0,observes=0;
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_CLIENT_INSPECTION
 unsigned holds=0,reads=0,closed=0,next=0,ended=0,bad=0,currentPrompt=0;
#endif
 explicit FullIO(SessionIO source):SessionIO(source){}
 bool Call(int fd,unsigned op,void* data,size_t size){
  namespace F=PioneerQ38FullOffload;namespace P=PioneerQ38TokenControl;
  if(op==TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT){
   assert(fd==7 && owned && controls==2 && transfer.receipt.Sealed() && size==sizeof(F::Support));support++;
   *static_cast<F::Support*>(data)={55,1,F::MaxPrompt,F::MaxOutput,F::MaxPositions,248320,F::AllCompute,0};return true;
  }
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_CLIENT_INSPECTION
  if(op==TT_PCI_PROBE_Q38_QGWD_RESIDENT_SUPPORT){
   namespace R=PioneerQ38QgwdResident;auto& s=*static_cast<R::Support*>(data);
   assert(fd==7 && owned && size==sizeof(s) && R::SupportRequest(s));s={55,1,1,1,1,1,{}};return true;
  }
  if(op==TT_PCI_PROBE_Q38_QGWD_RESIDENT_CONTROL){
   namespace R=PioneerQ38QgwdResident;auto& p=*static_cast<R::Packet*>(data);
   assert(fd==7 && owned && size==sizeof(p) && R::RequestShape(p));auto& r=p.reply;
   r.operation=p.request.operation;r.started=r.ready=1;r.hostsReleased=3;
   if(p.request.operation==R::RequestClose){assert(holds==1 && reads==1 && closed==next);closed++;r.stage=10;r.reservationRetained=1;}
   else if(p.request.operation==R::NextRequest){assert(closed==next+1);next++;observes=holds=reads=0;
    r.stage=11;r.reservationRetained=r.oldContextRetired=1;}
   else{assert(p.request.operation==R::ModelEnd && closed==next+1);ended++;owned=false;r.stage=12;r.modelEnded=1;}
   if(bad==p.request.operation)r.stage++;return true;
  }
#endif
  if(op!=TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL)return SessionIO::Call(fd,op,data,size);
  auto& p=*static_cast<P::Packet*>(data);assert(fd==7 && owned && controls==2 && transfer.receipt.Sealed()
   && size==sizeof(p) && F::RequestShape(p));
  auto& r=p.reply;r.version=P::Version;r.operation=p.request.operation;r.started=r.ready=1;
  auto* d=r.detail;d[0]=d[1]=1;
  if(p.request.operation==P::Start){assert(p.request.prompts==1 && p.request.limit==1);
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_CLIENT_INSPECTION
   assert(p.prompt[0]==(starts?18U:17U));currentPrompt=p.prompt[0];
#else
   assert(p.prompt[0]==17);
#endif
   starts++;r.stage=d[2]=8;d[3]=1;return true;}
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_CLIENT_INSPECTION
  if(p.request.operation==P::Observe){observes++;r.stage=d[2]=11;d[6]=1;d[8]=1;d[10]=64;return true;}
  if(p.request.operation==P::Hold){assert(!holds++);r.stage=d[2]=4;d[6]=1;d[7]=120;return true;}
  if(p.request.operation==P::Read){assert(!reads++ && p.request.records==1);r.stage=d[2]=4;r.records=1;
   d[6]=1;d[8]=1;d[9]=1;d[10]=256;d[11]=1;auto* v=p.records;
   v[0]=0x31435451;v[1]=1;v[3]=currentPrompt;v[4]=1000;v[5]=0x3f800000;v[6]=1;v[7]=1;
   v[10]=64;v[11]=973;for(unsigned i=12;i<36;i++)v[i]=100+i;v[44]=1;v[45]=48;v[46]=119;return true;}
#else
  if(p.request.operation==P::Observe){observes++;return false;}
#endif
  assert(false);return false;
 }
};
int main(int argc,char** argv){
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_CLIENT_INSPECTION
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--close") || !strcmp(argv[1],"--next") || !strcmp(argv[1],"--end"))));
#else
 assert(argc==1);(void)argv;
#endif
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
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_CLIENT_INSPECTION
 if(argc==2)io.bad=!strcmp(argv[1],"--close")?1:!strcmp(argv[1],"--next")?2:3;
#endif
 {PioneerQ38HybridFullSession<FakePrepared,FullIO> session(prepared,io);
  assert(session.Begin(*canonical,&prompt,1,1,[&](uint64_t bytes,uint64_t total){
   assert(bytes>progressBytes && bytes<=total && total==UINT64_C(20754878464));progressBytes=bytes;progressCalls++;
  }) && io.opens==1 && io.closes==0 && io.support==1
   && io.starts==1 && transfer.calls==21475 && io.controls==2 && io.spanEnds==1 && !io.bankEnds);
  assert(progressCalls==21475 && progressBytes==UINT64_C(20754878464));
#ifdef PIONEER_Q38_SMALL_RECURRENT_RESIDENT_CLIENT_INSPECTION
  assert(session.Poll() && session.TerminalReady());
  if(io.bad==1){assert(!session.Finish() && session.FailedState() && io.closes==1 && !io.bankEnds);return 0;}
  assert(session.Finish());uint32_t output[PioneerQ38FullOffload::MaxOutput]={};unsigned count=0;
  assert(session.CopyGeneratedTokens(output,PioneerQ38FullOffload::MaxOutput,count) && count==1 && output[0]==1000);
  const uint32_t invalid=PioneerQ38FullOffload::Vocabulary,nextPrompt=18;
  assert(!session.NextResidentRequest(&invalid,1,1) && !session.NextResidentRequest(nullptr,1,1)
   && !session.NextResidentRequest(&nextPrompt,0,1) && !io.next && !io.closes);
  if(io.bad==2){assert(!session.NextResidentRequest(&nextPrompt,1,1) && session.FailedState() && io.closes==1 && !io.bankEnds);return 0;}
  assert(session.NextResidentRequest(&nextPrompt,1,1) && session.Poll() && session.Finish());
  assert(io.opens==1 && io.starts==2 && io.closed==2 && io.next==1 && !io.closes && transfer.calls==21475);
  if(io.bad==3){assert(!session.EndResident() && session.FailedState() && io.closes==1 && !io.bankEnds);return 0;}
  assert(session.EndResident() && session.Completed() && io.ended==1 && io.closes==1 && !io.bankEnds);
#else
  assert(!session.Poll() && session.FailedState() && io.observes==1 && io.closes==1 && !io.bankEnds);
  assert(!session.Poll() && io.observes==1 && io.closes==1);
#endif
 }
 assert(io.closes==1);
 puts("PASS fake-identity recurrent full-session lifecycle; SIMULATED ONLY, NO MODEL OR SPEED");
}
