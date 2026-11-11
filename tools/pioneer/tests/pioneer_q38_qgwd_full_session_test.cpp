/* SPDX-License-Identifier: MIT */
// Actual QGWD prepared page/session path through the existing full-session
// controller, with synthetic driver replies only. No model or hardware.
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_QGWD_UPLOAD_SESSION_FIXTURE_ONLY 1
#include "pioneer_q38_qgwd_upload_session_test.cpp"
#include "../PioneerQ38HybridFullSession.h"
#ifdef PIONEER_Q38_QGWE_BATCH_DENSE_PLAN
static_assert(PioneerQ38HybridHeadWorkerLayout::Magic==0x45574751,"QGWE simulated plan identity required");
#endif
#if defined(PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION) || defined(PIONEER_Q38_QGWD_TWO_CLIENT_142)
static constexpr unsigned kOutputs=2;
#else
static constexpr unsigned kOutputs=1;
#endif

struct FullIO:SessionOwner {
 unsigned support=0,starts=0,polls=0,holds=0,reads=0,finishes=0;
 bool failStart=false,badFeedback=false;
 explicit FullIO(SessionOwner owner):SessionOwner(owner){}
 bool Call(int fd,unsigned op,void* data,size_t size){
  namespace F=PioneerQ38FullOffload;namespace P=PioneerQ38TokenControl;
  if(op==TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT){
   assert(fd==42 && support<2 && (support==0?!owned:owned && seals==1 && receipt.Sealed()) && size==sizeof(F::Support));support++;
   *static_cast<F::Support*>(data)={55,1,F::MaxPrompt,F::MaxOutput,F::MaxPositions,248320,F::AllCompute,0};return true;
  }
  if(op!=TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL)return SessionOwner::Call(fd,op,data,size);
  assert(fd==42 && owned && seals==1 && receipt.Sealed() && support==2 && size==sizeof(P::Packet));
  auto& packet=*static_cast<P::Packet*>(data);assert(F::RequestShape(packet));auto& r=packet.reply;
  const unsigned action=packet.request.operation;r.version=1;r.operation=action;r.started=r.ready=1;
  auto* d=r.detail;d[0]=d[1]=1;
  if(action==P::Start){assert(!starts++ && packet.request.prompts==1 && packet.request.limit==kOutputs && packet.prompt[0]==
#ifdef PIONEER_Q38_QGWD_CLIENT_141
   248319
#else
   17
#endif
  );
   r.stage=d[2]=8;d[3]=1;if(failStart)return false;
  }else if(action==P::Observe){assert(starts==1 && !holds);polls++;
   if(polls==1){r.ready=0;r.stage=d[2]=9;d[12]=123;d[13]=456;}
   else{r.stage=d[2]=11;d[6]=1;d[8]=kOutputs;d[9]=kOutputs-1;d[10]=64;}
  }else if(action==P::Hold){assert(polls==2 && !holds++);r.stage=d[2]=4;d[6]=1;d[7]=120;
  }else if(action==P::Read){assert(holds==1 && !reads++ && packet.request.records==kOutputs);r.stage=d[2]=4;r.records=kOutputs;
   d[6]=1;d[8]=kOutputs;d[9]=1;d[10]=256*kOutputs;d[11]=1;
   for(unsigned n=0;n<kOutputs;n++){auto* v=packet.records+n*64;
   v[0]=0x31435451;v[1]=1;v[2]=n;v[3]=n?1000:
#ifdef PIONEER_Q38_QGWD_CLIENT_141
    248319;
#else
    17;
#endif
   if(n && badFeedback)v[3]++;
   v[4]=1000+n;v[5]=0x3f800000;v[6]=n+1;v[7]=(n+1)&1;v[9]=unsigned(n+1<kOutputs);
   v[10]=64;v[11]=973;for(unsigned i=12;i<36;i++)v[i]=100+i;v[44]=n+1;v[45]=48;v[46]=119;}
  }else if(action==P::Finish){assert(reads==1 && !finishes++);r.stage=d[2]=11;r.complete=1;d[3]=d[5]=1;owned=false;
  }else assert(!"unexpected synthetic full-session action");return true;
 }
};
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!std::strcmp(argv[1],"--fail-start")
  || (kOutputs==2 && !std::strcmp(argv[1],"--bad-feedback")))));
#ifdef PIONEER_Q38_QGWD_TWO_CLIENT_142
 static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026111100010142),"distinct QGWD two-token client identity");
#elif defined(PIONEER_Q38_QGWD_CLIENT_141)
 static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026111000010141),"distinct QGWD candidate identity");
#else
 static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x445747514f4e4452),"distinct synthetic QGWD owner");
#endif
 static_assert(PioneerQ38FullOffload::MaxPrompt==1 && PioneerQ38FullOffload::MaxOutput==kOutputs,"bounded simulation");
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());FakeSource source;FakePlan packed;packed.source=&source;
 std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;
   ids[{layer,role}]=unsigned(source.tensors.size());source.tensors.push_back(t);}}
 for(unsigned i=0;i<canonical->Count();i++){const auto* p=canonical->Part(i);assert(p);
  const unsigned tensor=p->kind==canonical->WeightKind?ids.at({p->layer,p->role}):851;
  packed.parts.push_back({p->kind,p->layer,p->role,tensor,p->bank,p->firstRow,p->rows,p->rowBytes,canonical->Base+p->offset,p->bytes,p->allocated});}
 const auto digest=PioneerModelWeightCache::Hash("authenticated-fixture",21);
 const auto four=PioneerModelWeightCache::Hash("four-fingerprint",16),head=PioneerModelWeightCache::Hash("head-fingerprint",16);
 FakeBfp8 eight{&source};eight.digest=digest;std::string error;HeadPages parent;
 assert(parent.Bind(source,packed,*canonical,eight,digest,four,head,"/private/tmp/four","/private/tmp/head",[]{return true;},error));
 assert(parent.VerifyAll(error));DensePages pages;assert(pages.Bind(source,packed,*canonical,eight,parent,digest,error) && pages.VerifyAll(error));
 BanksMock mock;auto proof=BankProof(mock);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 auto receipt=std::make_unique<PioneerQ38DenseQ8UploadReceiptInspection>();PioneerQ38SpanLease lease;int initial=0;
 FakeDenseIO spans{*canonical,pages,*receipt,lease,&initial,ledger,hashes};
 FullIO io(SessionOwner{spans,*receipt,lease,banks,proof,ledger,hashes});
 io.failStart=argc==2 && !std::strcmp(argv[1],"--fail-start");
 io.badFeedback=argc==2 && !std::strcmp(argv[1],"--bad-feedback");
 PioneerQ38HybridFullSession<DensePages,FullIO> session(pages,io);const uint32_t input[]={
#ifdef PIONEER_Q38_QGWD_CLIENT_141
  248319
#else
  17
#endif
 };
#if defined(PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION) || defined(PIONEER_Q38_QGWD_TWO_CLIENT_142)
 const uint32_t twoPrompts[]={input[0],18};
 assert(!session.Begin(*canonical,input,1,3) && !session.Begin(*canonical,twoPrompts,2,1)
  && !session.FailedState() && !io.opens && !io.starts);
#endif
#ifdef PIONEER_Q38_QGWD_CLIENT_141
 const uint32_t wrongInput[]={17};
 assert(!session.Begin(*canonical,wrongInput,1,kOutputs) && !io.opens && !session.FailedState());
 FullIO wrongDriver(SessionOwner{spans,*receipt,lease,banks,proof,ledger,hashes});
 wrongDriver.tag=
#ifdef PIONEER_Q38_QGWD_TWO_CLIENT_142
  UINT64_C(0x2026111000010141); // the earlier driver cannot satisfy tag142
#else
  UINT64_C(0x2026102100010012);
#endif
 PioneerQ38HybridFullSession<DensePages,FullIO> denied(pages,wrongDriver);
 assert(!denied.Begin(*canonical,input,1,kOutputs) && denied.FailedState() && wrongDriver.opens==1
  && wrongDriver.closes==1 && !wrongDriver.owned && !wrongDriver.starts && !banks.Quarantined());
#endif
 bool clockCalled=false;const bool started=session.Begin(*canonical,input,1,kOutputs,{},[&]{clockCalled=true;return true;});
 if(io.failStart){
  assert(!started && clockCalled && session.FailedState() && io.owned && io.opens==1 && io.closes==1
   && io.starts==1 && !io.polls && !io.finishes && banks.Quarantined());
  puts("PASS QGWD synthetic full-session failed START retains/quarantines owner; NO DEVICE OR MODEL");return 0;
 }
 assert(started);
 assert(clockCalled && session.Poll() && session.Poll() && session.TerminalReady());
 if(io.badFeedback){
  assert(!session.Finish() && session.FailedState() && io.owned && io.reads==1 && !io.finishes
   && io.closes==1 && banks.Quarantined());
  uint32_t rejected[kOutputs]={};unsigned rejectedCount=0;
  assert(!session.CopyGeneratedTokens(rejected,kOutputs,rejectedCount) && rejectedCount==0 && rejected[0]==0);
  puts("PASS QGWD synthetic two-token feedback mismatch refuses output and quarantines; NO DEVICE OR MODEL");return 0;
 }
 assert(session.Finish() && session.Completed());
 uint32_t output[kOutputs]={};unsigned count=0;assert(session.CopyGeneratedTokens(output,kOutputs,count) && count==kOutputs && output[0]==1000);
 if(kOutputs==2)assert(output[1]==1001);
 assert(io.support==2 && io.starts==1 && io.polls==2 && io.holds==1 && io.reads==1 && io.finishes==1
  && io.opens==1 && io.closes==1 && !io.owned && !banks.Quarantined());
 puts(kOutputs==2?"PASS QGWD synthetic two-token feedback and same-FD full-session lifecycle; NO DEVICE OR MODEL":
  "PASS QGWD actual prepared upload and same-FD full-session lifecycle with synthetic token; NO DEVICE OR MODEL");
}
