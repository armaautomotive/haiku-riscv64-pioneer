/* SPDX-License-Identifier: MIT */
// TEST byte endpoints shared DIRECTLY with the actual held/native fixture.
// A virgin CPU is constructed before binding/initialization. Installed L1,
// boot DATA,status,stack and NIU CSR accesses use that SAME backing. No PC,
// registers,history,status or ready flag is copied from another machine.
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
