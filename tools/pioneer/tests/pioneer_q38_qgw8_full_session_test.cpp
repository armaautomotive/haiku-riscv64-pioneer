/* SPDX-License-Identifier: MIT */
// Real QGW8 upload/session code plus synthetic driver replies. No device or model.
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#define main qgw8_upload_fixture_main
#include "pioneer_q38_qgw8_upload_session_test.cpp"
#undef main
#include "../PioneerQ38HybridFullSession.h"

struct FullIO:SimOwner {
 unsigned support=0,starts=0,polls=0,holds=0,reads=0,finishes=0;
 bool failStart=false,badRecord=false,badSupport=false;
 explicit FullIO(Fixture& fixture):SimOwner(fixture){
#ifdef PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128
  tag=UINT64_C(0x2026110700010128);
#else
  tag=UINT64_C(0x51544b4e4f445256);
#endif
 }
 bool Call(int fd,unsigned op,void* data,size_t size){
  namespace F=PioneerQ38FullOffload;namespace P=PioneerQ38TokenControl;
  if(op==TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT){
   assert(fd==42 && support<2 && (support==0?!owned:owned && seals==1 && receipt->Sealed())
    && size==sizeof(F::Support));support++;
   *static_cast<F::Support*>(data)={55,1,96,32,128,248320,F::AllCompute,0};
   if(badSupport)static_cast<F::Support*>(data)->compute=0;
   return true;
  }
  if(op!=TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL)return SimOwner::Call(fd,op,data,size);
  assert(fd==42 && owned && seals==1 && receipt->Sealed() && support==2 && size==sizeof(P::Packet));
  auto& p=*static_cast<P::Packet*>(data);assert(F::RequestShape(p));auto& r=p.reply;
  const unsigned action=p.request.operation;r.version=1;r.operation=action;r.started=r.ready=1;
  auto* d=r.detail;d[0]=d[1]=1;
  if(action==P::Start){
   assert(!starts++ && p.request.prompts==1 && p.request.limit==2 && p.prompt[0]==17);
   r.stage=d[2]=8;d[3]=1;if(failStart)return false;
  }else if(action==P::Observe){
   assert(starts==1 && !holds);polls++;
   if(polls==1){r.ready=0;r.stage=d[2]=9;}
   else {r.stage=d[2]=11;d[6]=1;d[8]=2;d[9]=1;d[10]=64;}
  }else if(action==P::Hold){
   assert(polls==2 && !holds++);r.stage=d[2]=4;d[6]=1;d[7]=120;
  }else if(action==P::Read){
   assert(holds==1 && !reads++ && p.request.records==2);r.stage=d[2]=4;r.records=2;
   d[6]=1;d[8]=2;d[9]=1;d[10]=512;d[11]=1;
   for(unsigned pos=0;pos<2;pos++){
    auto* v=p.records+pos*64;v[0]=0x31435451;v[1]=1;v[2]=pos;
    v[3]=pos?1000:17;v[4]=1000+pos;v[5]=0x3f800000;v[6]=pos+1;v[7]=(pos+1)&1;
    v[9]=unsigned(pos==0);v[10]=64;v[11]=973;
    for(unsigned i=12;i<36;i++)v[i]=100+i;
    v[44]=pos+1;v[45]=48;v[46]=119;
   }
   if(badRecord)p.records[4]=F::Vocabulary;
  }else if(action==P::Finish){
   assert(reads==1 && !finishes++);r.stage=d[2]=11;r.complete=1;d[3]=d[5]=1;owned=false;
  }else assert(!"unexpected QGW8 lifecycle operation");
  return true;
 }
};
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--fail-start") || !strcmp(argv[1],"--bad-clock") || !strcmp(argv[1],"--bad-record")
  || !strcmp(argv[1],"--bad-support") || !strcmp(argv[1],"--wrong-tag") || !strcmp(argv[1],"--fake-tag"))));
#ifdef PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128
 static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x2026110700010128),"exact candidate tag");
#else
 static_assert(PioneerQ38FullOffload::BuildTag==UINT64_C(0x51544b4e4f445256),"renamed native-context fake tag only");
#endif
 Fixture fixture;auto io=std::make_unique<FullIO>(fixture);
 io->failStart=argc==2 && !strcmp(argv[1],"--fail-start");io->badRecord=argc==2 && !strcmp(argv[1],"--bad-record");
 io->badSupport=argc==2 && !strcmp(argv[1],"--bad-support");
 const bool fakeTag=argc==2 && !strcmp(argv[1],"--fake-tag");
 const bool wrongTag=argc==2 && !strcmp(argv[1],"--wrong-tag");
 const bool badClock=argc==2 && !strcmp(argv[1],"--bad-clock");
 if(wrongTag)io->tag=UINT64_C(0x385747514f4e4452); // old upload-only fixture is not the native full owner
 if(fakeTag)io->tag=UINT64_C(0x51544b4e4f445256);
 PioneerQ38HybridFullSession<HeadPages,FullIO> session(fixture.pages,*io);
 const uint32_t input[]={17};uint32_t tokens[32]={};unsigned count=99;
 bool clockCalled=false;
 bool ok=session.Begin(*fixture.canonical,input,1,2,{},[&]{
  assert(!clockCalled && io->support==2 && io->seals==1 && !io->starts);
  clockCalled=true;return !badClock;
 });
 if(ok)ok=session.Poll();if(ok)ok=session.Poll();if(ok)ok=session.Finish();
 if(argc==1){
  assert(ok && clockCalled && session.Completed() && !io->owned && io->opens==1 && io->closes==1 && !io->bankEnd);
  assert(io->begins==1 && io->seals==1 && io->spanEnd==1 && io->starts==1 && io->polls==2
   && io->holds==1 && io->reads==1 && io->finishes==1);
  assert(session.CopyGeneratedTokens(tokens,32,count) && count==2 && tokens[0]==1000 && tokens[1]==1001);
  puts("PASS QGW8 sealed upload through same-FD synthetic START/OBSERVE/HOLD/READ/FINISH; NO MODEL OR DEVICE");
 }else if(wrongTag || fakeTag){
  assert(!ok && !clockCalled && session.FailedState() && !io->support && io->closes==1 && !io->owned && !io->begins);
  puts("PASS upload-only fake tag refused before full-session capability or upload; NO DEVICE");
 }else if(io->badSupport){
  assert(!ok && !clockCalled && session.FailedState() && io->support==1 && io->closes==1 && !io->owned && !io->retained
   && !io->begins && !io->seals && !io->spanEnd && !io->bankEnd);
  puts("PASS unsupported QGW8 full session refused before bank BEGIN or upload; NO DEVICE");
 }else{
  assert(!ok && clockCalled && session.FailedState() && io->closes==1 && io->owned && io->retained
   && io->banks.Quarantined() && !io->bankEnd && !io->finishes);
  if(badClock)assert(!io->starts && session.failure=="start_clock");
  assert(!session.CopyGeneratedTokens(tokens,32,count) && count==99);
  puts("PASS QGW8 failed token lifecycle closes only and retains owner; NO DEVICE");
 }
}
