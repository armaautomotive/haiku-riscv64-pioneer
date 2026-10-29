/* SPDX-License-Identifier: MIT */
// TEST byte endpoints shared DIRECTLY with the actual held/native fixture.
// A virgin CPU is constructed before binding/initialization. Installed L1,
// boot DATA,status,stack and NIU CSR accesses use that SAME backing. No PC,
// registers,history,status or ready flag is copied from another machine.
#ifdef PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE
#include "pioneer_q38_native_peer_test_bridge.h"
static const char* tokenPeerOriginalImage=nullptr;
static const char* tokenPeerGlobalImage=nullptr;
struct TokenBootstrapPeer {
 TokenHeldEndpoints& io;const unsigned worker;std::unique_ptr<Q38NativePeerTestCpu> cpu;
 unsigned pc=0x6000,steps=0,matrixInstructions=0,moveInstructions=0,floatOps=0,requests=0,responses=0,pending=0;
 unsigned deviceReads=0,deviceWrites=0,statusWrites=0,minStack=0x70000;
 TokenBootstrapPeer(const char* image,TokenHeldEndpoints& native,unsigned w,const Q38NativePeerTestBytes& bytes):io(native),worker(w){
  assert(image && tokenPeerOriginalImage && tokenPeerGlobalImage);
  cpu=std::make_unique<Q38NativePeerTestCpu>(tokenPeerOriginalImage,tokenPeerGlobalImage,w,bytes);Observe();assert(!steps && pc==0x6000);
 }
 void Observe(){const auto s=cpu->Stats();pc=s.pc;steps=s.steps;matrixInstructions=s.matrixInstructions;moveInstructions=s.moveInstructions;
  floatOps=s.floatOps;requests=s.requests;responses=s.responses;pending=s.pending;deviceReads=s.deviceReads;deviceWrites=s.deviceWrites;
  statusWrites=s.statusWrites;minStack=s.minStack;
 } // READ-ONLY host observations,NEVER copied into any processor or native owner
 void Link(TokenBootstrapPeer& other){assert(!steps && !other.steps);cpu->Link(*other.cpu);}
 void Advance(unsigned count){assert(io.Core(worker).coreReset==0x46800);cpu->Advance(count);Observe();}
 bool Idle()const{return pc==0x2004c || pc==0x20050;}
};
#else
struct TokenBootstrapPeer:BlockMachine {
 TokenHeldEndpoints& io;const unsigned worker;unsigned deviceReads=0,deviceWrites=0,statusWrites=0;
 TokenBootstrapPeer(const char* image,TokenHeldEndpoints& native,unsigned w)
  :BlockMachine(image,8,0,.01f,nullptr,true),io(native),worker(w){assert(worker && worker<120 && !steps && pc==0x6000);}
 bool CodeAddress(uint32_t address)const override{
  uint32_t word;return PioneerQ38TokenPrograms::ProgramWord(worker,address,word);
 }
 unsigned Index(uint32_t address)override{
  assert(CodeAddress(address) && !(address&3));l1[8191]=io.Raw(worker,address);return 8191;
 } // ISA fetch only;NOT a saved/installed code buffer or private-history transplant
 unsigned StepLimit()const override{return 20000000;}
 uint32_t Load(uint32_t address)override{
  assert(!(address&3));deviceReads++;
  if(address<0x180000)return io.Raw(worker,address);
  assert(address>=0xffb20000 && address<0xffb22000);
  return io.Core(worker).ResidentNocRead(address-0xffb20000);
 }
 void Store(uint32_t address,uint32_t value)override{
  namespace W=PioneerQ38GlobalWorkerLayout;
  assert(!(address&3) && (Range(address,0x6e000,8192) || Range(address,W::Status,64)));
  if(Range(address,0x6e000,8192))minStack=std::min(minStack,address);
  else statusWrites++;
  deviceWrites++;io.Raw(worker,address)=value;
 } // boot emits ONLY stack/status stores,no matrix,NoC packet or host products
 void Advance(unsigned count){
  assert(io.Core(worker).coreReset==0x46800);
  for(unsigned i=0;i<count;i++)StepBlock();
  assert(!requests && !responses && !pending && !matrixInstructions && !moveInstructions && !floatOps);
 }
 bool Idle()const{return pc==0x2004c || pc==0x20050;}
};
#endif
