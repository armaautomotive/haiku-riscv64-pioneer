/* SPDX-License-Identifier: MIT */
// Actual virgin ROOT startup instructions on the SAME installed L1/CSR
// bytes. Deliberately bounded at its genuine startup-marker store: this is
// PARTIAL execution evidence,not model arithmetic,terminal or STOP proof.
struct TokenStartupRoot:BlockMachine {
 TokenHeldEndpoints& io;unsigned deviceReads=0,deviceWrites=0,markerWrites=0;
 TokenStartupRoot(const char* image,TokenHeldEndpoints& native):BlockMachine(image,8,0,.01f,nullptr,true),io(native){assert(!steps && pc==0x6000);}
 bool CodeAddress(uint32_t address)const override{uint32_t word;return PioneerQ38TokenPrograms::ProgramWord(0,address,word);}
 unsigned Index(uint32_t address)override{
  assert(CodeAddress(address) && !(address&3));l1[8191]=io.Raw(0,address);return 8191;
 }
 unsigned StepLimit()const override{return 2000000;}
 uint32_t Load(uint32_t address)override{
  assert(!(address&3));deviceReads++;
  if(address<0x180000)return io.Raw(0,address);
  assert(address>=0xffb20000 && address<0xffb22000);return io.Core(0).ResidentNocRead(address-0xffb20000);
 }
 void Store(uint32_t address,uint32_t value)override{
  namespace C=PioneerQ38GlobalChainLayout;
  assert(!(address&3) && (Range(address,0x6e000,8192) || Range(address,C::R::Status,64)));
  if(Range(address,0x6e000,8192))minStack=std::min(minStack,address);
  else{assert(address==C::R::Status+12 && value==PioneerQ38TokenLayout::Magic && !markerWrites);markerWrites++;}
  io.Raw(0,address)=value;deviceWrites++;
 }
 void Advance(unsigned count){
  assert(io.Core(0).coreReset==0x46800);
  for(unsigned i=0;i<count && !markerWrites;i++)StepBlock();
  assert(!requests && !responses && !pending && !matrixInstructions && !moveInstructions && !floatOps);
 } // test scheduler PAUSES this CPU after the ACTUAL marker-producing store;NEVER fabricates acknowledgment
};
