/* SPDX-License-Identifier: MIT */
// Actual virgin ROOT startup instructions on the SAME installed L1/CSR
// bytes. Deliberately bounded at its genuine startup-marker store: this is
// PARTIAL execution evidence,not model arithmetic,terminal or STOP proof.
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
#include "../PioneerQ38GlobalFeedbackLayout.h"
#endif
struct TokenStartupRoot:BlockMachine {
 TokenHeldEndpoints& io;unsigned deviceReads=0,deviceWrites=0,markerWrites=0;
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
 bool traffic=false;unsigned submitted=0,completed=0,busyReads=0;
 uint32_t request[10]={},before[8]={};
 void Submit(){
  namespace T=PioneerQ38TokenLayout;namespace C=PioneerQ38GlobalChainLayout;
  auto& c=io.Core(0);assert(traffic && markerWrites==1 && submitted<4 && !c.pending && c.coreReset==0x46800);
  for(unsigned i=0;i<10;i++)request[i]=c.command[i];
  const unsigned workspace=io.runtimeWorkspace,route=PioneerQ38ProjectionRoute(0);
  unsigned remote=workspace+(submitted==0?unsigned(T::SessionOffset):submitted==1?unsigned(T::PromptOffset):unsigned(C::HeadOffset));
  unsigned length=submitted==1?64U:512U,source=route;
  if(submitted==3){uint32_t plan[8];
   const unsigned token=io.runtimeDram.at(T::SessionOffset/4+7);
   assert(PioneerQ38GlobalFeedbackLayout::Describe(token,0,0,workspace,plan));
   source=PioneerQ38ProjectionRoute(plan[4]);remote=plan[5];length=2304;
  }
  const uint32_t expected[]={remote,0,source,C::R::Scratch+(remote&31),0,129,0,0,length,0};
  for(unsigned i=0;i<10;i++)assert(request[i]==expected[i]);
  // These are actual emulated hardware counters,not a native journal update.
  // Accepted advances at request;read completion advances ONLY on response.
  c.doorbells++;c.doorbellRead=false;c.requests++;c.pending=true;c.accepted++;submitted++;busyReads=0;
 }
 void Respond(){
  namespace C=PioneerQ38GlobalChainLayout;auto& c=io.Core(0);
  assert(traffic && c.pending && submitted==completed+1 && completed<3 && request[2]==PioneerQ38ProjectionRoute(0));
  const unsigned remote=request[0],local=request[3],bytes=request[8];
  assert(remote>=io.runtimeWorkspace && bytes<=67108864 && remote-io.runtimeWorkspace<=67108864-bytes);
  for(unsigned i=0;i<bytes/4;i++)io.Raw(0,local+i*4)=io.runtimeDram.at((remote-io.runtimeWorkspace)/4+i);
  c.counters[0]++;c.counters[1]+=(bytes+63)/64;c.counters[3]++;c.pending=false;c.done=true;completed++;
 } // raw immutable bytes from actual native staging ONLY;NO model products/completion fabricated
#endif
 TokenStartupRoot(const char* image,TokenHeldEndpoints& native):BlockMachine(image,8,0,.01f,nullptr,true),io(native){assert(!steps && pc==0x6000);}
 bool CodeAddress(uint32_t address)const override{uint32_t word;return PioneerQ38TokenPrograms::ProgramWord(0,address,word);}
 unsigned Index(uint32_t address)override{
  assert(CodeAddress(address) && !(address&3));l1[8191]=io.Raw(0,address);return 8191;
 }
 unsigned StepLimit()const override{return 2000000;}
 uint32_t Load(uint32_t address)override{
  assert(!(address&3));deviceReads++;
  if(address<0x180000)return io.Raw(0,address);
  assert(address>=0xffb20000 && address<0xffb22000);
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
  if(traffic && address==0xffb20040 && io.Core(0).pending && submitted<=3 && ++busyReads>=3)Respond();
#endif
  return io.Core(0).ResidentNocRead(address-0xffb20000);
 }
 void Store(uint32_t address,uint32_t value)override{
  namespace C=PioneerQ38GlobalChainLayout;
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
  namespace T=PioneerQ38TokenLayout;
  if(traffic && address>=0xffb20000 && address<0xffb20028){
   assert(!io.Core(0).pending);io.Core(0).ResidentNocWrite(address-0xffb20000,value);return;
  }
  if(traffic && address==0xffb20040){assert(value==1);Submit();return;}
  const bool bootstrapData=traffic && (Range(address,T::SessionLocal,512) || Range(address,C::Head,512)
   || Range(address,T::TerminalLocal,256) || Range(address,C::Progress,256)
   || Range(address,C::R::Original,20480) || Range(address,C::R::OriginalCopy,20480)
   || Range(address,C::R::Input,20480) || Range(address,C::R::InputShadow,20480)
   || Range(address,C::R::Native,10240) || Range(address,C::R::NativeShadow,10240)
   || Range(address,C::R::Scales,128) || Range(address,C::R::ScaleShadow,128));
  if(bootstrapData){assert(!(address&3));io.Raw(0,address)=value;deviceWrites++;return;}
#endif
  assert(!(address&3) && (Range(address,0x6e000,8192) || Range(address,C::R::Status,64)));
  if(Range(address,0x6e000,8192))minStack=std::min(minStack,address);
  else{
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
   if(traffic && address==C::R::Status+20){assert(value==0x300);io.Raw(0,address)=value;deviceWrites++;return;}
#endif
   assert(address==C::R::Status+12 && value==PioneerQ38TokenLayout::Magic && !markerWrites);markerWrites++;
  }
  io.Raw(0,address)=value;deviceWrites++;
 }
 void Advance(unsigned count){
  assert(io.Core(0).coreReset==0x46800);
  for(unsigned i=0;i<count;i++){
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
   if(traffic){if(submitted==4 && io.Core(0).pending)break;}
   else
#endif
   if(markerWrites)break;
   StepBlock();
  }
  assert(!requests && !responses && !pending && !matrixInstructions && !moveInstructions && !floatOps);
 } // test scheduler PAUSES this CPU after the ACTUAL marker-producing store;NEVER fabricates acknowledgment
};
