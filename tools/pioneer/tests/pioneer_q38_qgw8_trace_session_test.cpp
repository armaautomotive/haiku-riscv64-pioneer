/* SPDX-License-Identifier: MIT */
// Actual tag131 user session over synthetic pages/transport, no card.
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128 1
#define PIONEER_Q38_QGW8_TRACE_CLIENT_131 1
#define main qgw8_upload_fixture_main
#include "pioneer_q38_qgw8_upload_session_test.cpp"
#undef main
#include "../PioneerQ38HybridFullSession.h"

struct TraceIO:SimOwner {
 unsigned support=0,starts=0,polls=0,holds=0,reads=0,traceCalls=0,finishes=0;
 int failAt=-1;bool badReply=false,failFinish=false,failClose=false;
 explicit TraceIO(Fixture& f):SimOwner(f){tag=PioneerQ38FullOffload::BuildTag;}
 int Close(int fd){const int result=SimOwner::Close(fd);return failClose?-1:result;}
 bool Call(int fd,unsigned op,void* data,size_t size){
  namespace F=PioneerQ38FullOffload;namespace C=PioneerQ38TokenControl;namespace T=PioneerQ38Qgw8TraceNative;namespace P=PioneerQ38Qgw8LayerTracePacket;
  if(op==TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT){
   assert(fd==42 && support<2 && (support==0?!owned:owned && seals==1 && receipt->Sealed()) && size==sizeof(F::Support));
   support++;*static_cast<F::Support*>(data)={55,1,1,1,1,248320,F::AllCompute,0};return true;
  }
  if(op==TT_PCI_PROBE_Q38_QGW8_TRACE_READ){
   assert(fd==42 && owned && reads==1 && !finishes && size==sizeof(P::Packet));
   auto& p=*static_cast<P::Packet*>(data);assert(P::RequestShape(p) && p.request.ordinal==traceCalls);
   const unsigned n=traceCalls++;if(int(n)==failAt)return false;
   auto& r=p.reply;r.version=P::Version;r.ordinal=n;r.started=r.ready=1;r.words=p.request.words;
   const uint32_t d[]={1,1,4,0,0,0,1,n,(n/2)*5120+(n%2)*4096,p.request.words,0,64,1,1,1,p.request.words*4};
   std::copy(d,d+16,r.detail);for(unsigned i=0;i<p.request.words;i++)p.words[i]=d[8]+i+1;
   if(badReply && n==63)r.detail[11]--;return true;
  }
  if(op!=TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL)return SimOwner::Call(fd,op,data,size);
  assert(fd==42 && owned && seals==1 && receipt->Sealed() && support==2 && size==sizeof(C::Packet));
  auto& p=*static_cast<C::Packet*>(data);assert(F::RequestShape(p) && T::ControlRequestShape(p));
  auto& r=p.reply;const unsigned action=p.request.operation;r.version=1;r.operation=action;r.started=r.ready=1;
  auto* d=r.detail;d[0]=d[1]=1;
  if(action==C::Start){assert(!starts++ && p.request.prompts==1 && p.request.limit==1 && !p.request.stopEos && p.prompt[0]==248319);r.stage=d[2]=8;d[3]=1;}
  else if(action==C::Observe){assert(starts==1 && !holds);polls++;if(polls==1){r.ready=0;r.stage=d[2]=9;}
   else{r.stage=d[2]=11;d[6]=1;d[8]=1;d[9]=0;d[10]=64;}}
  else if(action==C::Hold){assert(polls==2 && !holds++);r.stage=d[2]=4;d[6]=1;d[7]=120;}
  else if(action==C::Read){assert(holds==1 && !reads++ && p.request.records==1);r.stage=d[2]=4;r.records=1;
   d[6]=1;d[8]=1;d[9]=1;d[10]=256;d[11]=1;auto* v=p.records;
   v[0]=0x31435451;v[1]=1;v[2]=0;v[3]=248319;v[4]=108133;v[5]=0x3f800000;
   v[6]=1;v[7]=1;v[8]=v[9]=0;v[10]=64;v[11]=973;for(unsigned i=12;i<36;i++)v[i]=100+i;
   v[44]=1;v[45]=48;v[46]=119;}
  else if(action==C::Finish){assert(traceCalls==128 && !finishes++);r.stage=d[2]=11;r.complete=1;d[3]=d[5]=1;
   if(failFinish)return false;owned=false;}
  else assert(!"unexpected trace lifecycle operation");return true;
 }
};
int main(int argc,char** argv){
 assert(argc==1 || argc==2);const char* mode=argc==2?argv[1]:"";
 Fixture fixture;auto io=std::make_unique<TraceIO>(fixture);
 unsigned workspaceCount=0;
 for(unsigned i=0;i<fixture.canonical->Count();i++){
  const auto* original=fixture.canonical->Part(i);
  if(original && original->kind==PioneerQ38CanonicalModelPlan::WorkspaceKind && original->bank==0){
   const auto* converted=fixture.pages.Part(i);assert(converted && converted->bank==0 && converted->offset!=original->offset);workspaceCount++;
  }
 }
 assert(workspaceCount==1); // deliberately different original vs converted geometry
 if(!strcmp(mode,"--fail-first"))io->failAt=0;else if(!strcmp(mode,"--fail-middle"))io->failAt=63;
 else if(!strcmp(mode,"--fail-last"))io->failAt=127;else if(!strcmp(mode,"--bad-reply"))io->badReply=true;
 else if(!strcmp(mode,"--fail-finish"))io->failFinish=true;else if(!strcmp(mode,"--fail-close"))io->failClose=true;
 else if(!strcmp(mode,"--wrong-tag"))io->tag=
  PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026110700010132)?UINT64_C(0x2026110700010131):UINT64_C(0x2026110700010132);
 else assert(!*mode || !strcmp(mode,"--wrong-input"));
 PioneerQ38HybridFullSession<HeadPages,TraceIO> session(fixture.pages,*io);
 std::vector<uint32_t> result(64U*5120U,0xdeadbeef);assert(!session.TraceGeneration() && !session.CopyTrace(result.data(),result.size()));
 const uint32_t input[]={248319U-unsigned(!strcmp(mode,"--wrong-input"))};
 bool okay=session.Begin(*fixture.canonical,input,1,1);
 if(!strcmp(mode,"--wrong-input")){assert(!okay && !io->opens);puts("PASS trace wrong input before open; synthetic");return 0;}
 if(okay)assert(!session.Cancel() && io->starts==1 && !io->polls);
 if(okay)okay=session.Poll();if(okay)okay=session.Poll();if(okay)okay=session.Finish();
 if(!*mode){
  assert(okay && session.Completed() && session.TraceGeneration()==20 && io->traceCalls==128 && io->finishes==1 && !io->owned && !io->retained && io->closes==1 && !io->bankEnd);
  assert(!session.CopyTrace(result.data(),result.size()-1) && result[0]==0xdeadbeef);
  assert(!session.CopyTrace(nullptr,result.size()));
  assert(!session.CopyTrace(reinterpret_cast<uint32_t*>(&session),result.size()));
  assert(!session.CopyTrace(reinterpret_cast<uint32_t*>(io.get()),result.size()));
  assert(!session.CopyTrace(reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(result.data())+1),result.size()) && result[0]==0xdeadbeef);
  assert(session.CopyTrace(result.data(),result.size()) && result[0]==1 && result.back()==result.size());
  assert(!session.CopyTrace(result.data(),result.size()));
  puts("PASS trace 128 ordered reads then FINISH/close and one publication; synthetic");
 }else{
  assert(!okay && session.FailedState() && !session.TraceGeneration() && io->closes==1 && !session.CopyTrace(result.data(),result.size()) && result[0]==0xdeadbeef);
  if(!strcmp(mode,"--wrong-tag"))assert(!io->owned && !io->begins && !io->traceCalls);
  else if(!strcmp(mode,"--fail-close"))assert(!io->owned && io->finishes==1 && io->traceCalls==128);
  else assert(io->owned && io->retained && io->banks.Quarantined() && !io->bankEnd);
  puts("PASS trace failure refuses output and does not retry/END; synthetic");
 }
}
