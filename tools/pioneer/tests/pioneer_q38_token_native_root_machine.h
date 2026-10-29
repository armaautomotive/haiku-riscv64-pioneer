/* SPDX-License-Identifier: MIT */
// Actual virgin ROOT instructions on the SAME installed L1/CSR bytes.
// Separate profiles pause at the actual marker,raw request,embedding,or
// first layer dispatch. None proves a complete layer/token/terminal/STOP.
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
#include "../PioneerQ38GlobalFeedbackLayout.h"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
#include "pioneer_q38_embedding_raw_test_bridge.h"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
#define PIONEER_GRAPH_MATH_BASE_ALREADY_INCLUDED
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
using TokenNativeRootISA=ScalarMachine;
#else
using TokenNativeRootISA=BlockMachine;
#endif
struct TokenStartupRoot:TokenNativeRootISA {
 TokenHeldEndpoints& io;unsigned deviceReads=0,deviceWrites=0,markerWrites=0;
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
 unsigned floatOps=0;
#include "pioneer_q38_token_native_layer_machine.inc"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
 bool traffic=false;unsigned submitted=0,completed=0,busyReads=0;
 uint32_t request[10]={},before[8]={};
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
 Q38EmbeddingRawTestBridge* original=nullptr;bool embedding=false,sourceLost=false,corruptBack=false;
 unsigned rawTiles=0,embeddingWrites=0,embeddingBacks=0,sourceRefusals=0;
 uint32_t geometry[8]={};
 void ResumeEmbedding(){
  assert(!embedding && original && original->Ready() && submitted==4 && completed==3 && io.Core(0).pending);
  const unsigned token=io.runtimeDram.at(PioneerQ38TokenLayout::SessionOffset/4+7);
  assert(PioneerQ38GlobalFeedbackLayout::Describe(token,0,0,io.runtimeWorkspace,geometry));embedding=true;
 }
#endif
 void Submit(){
  namespace T=PioneerQ38TokenLayout;namespace C=PioneerQ38GlobalChainLayout;
  auto& c=io.Core(0);assert(traffic && markerWrites==1 && !c.pending && c.coreReset==0x46800);
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
  if(layerActive){SubmitLayer();return;}
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
  assert(submitted<(embedding?28U:4U));
#else
  assert(submitted<4);
#endif
  for(unsigned i=0;i<10;i++)request[i]=c.command[i];
  const unsigned workspace=io.runtimeWorkspace,route=PioneerQ38ProjectionRoute(0);
  unsigned remote=workspace+(submitted==0?unsigned(T::SessionOffset):submitted==1?unsigned(T::PromptOffset):unsigned(C::HeadOffset));
  unsigned length=submitted==1?64U:512U,source=route;
  bool write=false;
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
  if(embedding && submitted>=3){
   if(submitted<23){source=PioneerQ38ProjectionRoute(geometry[4]);remote=geometry[5]+(submitted-3)*2304;length=2304;}
   else if(submitted<27){write=submitted<25;unsigned ordinal=submitted-(write?23:25);
    remote=io.runtimeWorkspace+PioneerQ38GlobalFeedbackLayout::Offset+ordinal*16384;length=ordinal?4096:16384;source=route;
   }else{assert(C::Address(workspace,0,remote));source=route;length=512;}
  }else
#endif
  if(submitted==3){uint32_t plan[8];
   const unsigned token=io.runtimeDram.at(T::SessionOffset/4+7);
   assert(PioneerQ38GlobalFeedbackLayout::Describe(token,0,0,workspace,plan));
   source=PioneerQ38ProjectionRoute(plan[4]);remote=plan[5];length=2304;
  }
  const unsigned scratch=C::R::Scratch+(remote&31);
  const uint32_t expected[]={write?scratch:remote,0,write?129U:source,write?remote:scratch,0,write?source:129U,0,write?0x212U:0,length,0};
  for(unsigned i=0;i<10;i++)assert(request[i]==expected[i]);
  // These are actual emulated hardware counters,not a native journal update.
  // Accepted advances at request;read completion advances ONLY on response.
  c.doorbells++;c.doorbellRead=false;c.requests++;c.pending=true;c.accepted++;submitted++;busyReads=0;
 }
 void Respond(){
  namespace C=PioneerQ38GlobalChainLayout;auto& c=io.Core(0);
  assert(traffic && c.pending && submitted==completed+1);
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
  if(layerActive){RespondLayer();return;}
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
  if(embedding && completed>=3 && completed<23){
   if(sourceLost)return; // first source refusal holds this ONE response;NEVER retry/reopen/fallback
   assert(original && request[2]==PioneerQ38ProjectionRoute(geometry[4]) && request[0]==geometry[5]+rawTiles*2304 && request[8]==2304);
   std::array<uint8_t,2304> raw;
   if(!original->Read(request[2],request[0],raw.data(),raw.size())){sourceLost=true;sourceRefusals++;return;}
   for(unsigned i=0;i<raw.size()/4;i++)io.Raw(0,request[3]+i*4)=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;
   c.counters[0]++;c.counters[1]+=36;c.counters[3]++;c.pending=false;c.done=true;completed++;rawTiles++;return;
  }
  const bool write=embedding && request[7]==0x212;
  assert((embedding && completed<27) || completed<3);
#else
  const bool write=false;assert(completed<3);
#endif
  assert(request[write?5:2]==PioneerQ38ProjectionRoute(0));
  const unsigned remote=request[write?3:0],local=request[write?0:3],bytes=request[8];
  assert(remote>=io.runtimeWorkspace && bytes<=67108864 && remote-io.runtimeWorkspace<=67108864-bytes);
  for(unsigned i=0;i<bytes/4;i++){
   auto& physical=io.runtimeDram.at((remote-io.runtimeWorkspace)/4+i);
   if(write)physical=io.Raw(0,local+i*4);else io.Raw(0,local+i*4)=physical;
  }
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
  if(embedding && completed>=23){
   if(write)embeddingWrites++;else{embeddingBacks++;if(corruptBack && embeddingBacks==2)io.Raw(0,local)^=1;}
  }
#endif
  if(write){c.writeCounters[0]++;c.writeCounters[1]+=(bytes+63)/64;c.writeCounters[3]++;}
  else{c.counters[0]++;c.counters[1]+=(bytes+63)/64;c.counters[3]++;}
  c.pending=false;c.done=true;completed++;
 } // raw immutable bytes from actual native staging ONLY;NO model products/completion fabricated
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
 TokenStartupRoot(const char* image,TokenHeldEndpoints& native):ScalarMachine(0,0),io(native){assert(image && !steps && pc==0x6000);}
#else
 TokenStartupRoot(const char* image,TokenHeldEndpoints& native):BlockMachine(image,8,0,.01f,nullptr,true),io(native){assert(!steps && pc==0x6000);}
#endif
 bool CodeAddress(uint32_t address)const override{uint32_t word;return PioneerQ38TokenPrograms::ProgramWord(0,address,word);}
 unsigned Index(uint32_t address)override{
  assert(CodeAddress(address) && !(address&3));l1[8191]=io.Raw(0,address);return 8191;
 }
 unsigned StepLimit()const override{
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
  if(embedding)return 30000000; // HOST instruction watchdog only;firmware packet polls unchanged
#endif
  return 2000000;
 }
 uint32_t Load(uint32_t address)override{
  assert(!(address&3));deviceReads++;
  if(address<0x180000)return io.Raw(0,address);
  assert(address>=0xffb20000 && address<0xffb22000);
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
  bool mayRespond=submitted<=3;
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
  mayRespond |= embedding && submitted<28;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
  mayRespond |= layerActive && !layerHeld;
#endif
  if(traffic && address==0xffb20040 && io.Core(0).pending && mayRespond && ++busyReads>=3)Respond();
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
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
  if(layerActive){
   assert(PioneerQ38TokenL1::SnapshotContains(0,address) && !CodeAddress(address));
   if(Range(address,0x6e000,8192))minStack=std::min(minStack,address);
   io.Raw(0,address)=value;deviceWrites++;return;
  }
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
  if(embedding && (Range(address,PioneerQ38GlobalFeedbackLayout::Local,20480)
   || Range(address,PioneerQ38GlobalFeedbackLayout::Shadow,20480) || Range(address,PioneerQ38GlobalFeedbackLayout::Raw,2304)
   || Range(address,C::R::Scratch,16384))){assert(!(address&3));io.Raw(0,address)=value;deviceWrites++;return;}
#endif
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
   if(traffic){
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
    if(embedding){if(pc==0x60000 || pc==0x60004 || (submitted==28 && io.Core(0).pending
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
     && !layerActive
#endif
     ))break;}
    else
#endif
    if(submitted==4 && io.Core(0).pending)break;
   }
   else
#endif
   if(markerWrites)break;
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
   if(layerHeld)break;
   const uint32_t instruction=io.Raw(0,pc),op=instruction&127;
   if(op==7 || op==0x27 || op==0x53)floatOps++;
   StepScalar(); // unchanged FULL scalar ISA engine,from the very first virgin instruction
#elif defined(PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE)
   // Same existing RV32 interpreter plus the scalar FSUB used by the
   // original Q4_K decoder. Execute the instruction,never substitute values.
   const uint32_t instruction=io.Raw(0,pc);
   if(embedding && (instruction&127)==0x53 && instruction>>25==4){
    assert(CodeAddress(pc) && ++steps<StepLimit());floatOps++;
    const unsigned d=(instruction>>7)&31,s=(instruction>>15)&31,t=(instruction>>20)&31,f=(instruction>>12)&7;
    assert(f==0 || f==7);fp[d]=Bits(Float(fp[s])-Float(fp[t]));regs[0]=0;pc+=4;
   }else
#endif
#ifndef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
   StepBlock();
#endif
  }
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
  assert(!matrixInstructions && !moveInstructions);
#else
  assert(!requests && !responses && !pending && !matrixInstructions && !moveInstructions);
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
  if(!embedding)
#endif
  assert(!floatOps);
 } // test scheduler PAUSES this CPU after the ACTUAL marker-producing store;NEVER fabricates acknowledgment
};
