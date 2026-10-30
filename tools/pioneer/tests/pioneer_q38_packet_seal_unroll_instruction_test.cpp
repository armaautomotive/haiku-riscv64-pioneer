/* SPDX-License-Identifier: MIT */
// Isolated emitted-function equivalence only; synthetic ABI arguments are
// NOT a genuine native owner, boot history, model or completion capability.
#define PIONEER_Q38_WORD_SEAL_MACHINE_ONLY
#include "pioneer_q38_word_seal_instruction_test.cpp"
struct PacketSealMachine:SealMachine {
 std::vector<std::pair<unsigned,unsigned>> visible;
 PacketSealMachine(const SealImage& rom):SealMachine(rom,0){
  for(unsigned i=0;i<8;i++)memory[Owner+768+(32+i)*4]=memory.at(Owner+(8+i)*4);
 }
 uint32_t Load(uint32_t address)override{
  const auto value=SealMachine::Load(address);
  // Include private synthetic arguments in order, not compiler spill slots.
  if(!CodeAddress(address) && !(address>=0x6f000 && address<0x70000))visible.emplace_back(address,value);
  return value;
 }
};
static unsigned cases=0,failures=0;
static unsigned CheckPacket(const SealImage& old,const SealImage& candidate,unsigned address=0,
 unsigned mask=0,bool okay=true,unsigned owner=SealMachine::Owner,unsigned frame=SealMachine::Owner+768){
 auto a=std::make_unique<PacketSealMachine>(old),b=std::make_unique<PacketSealMachine>(candidate);
 if(address){assert(a->memory.count(address) && b->memory.count(address));a->memory.at(address)^=mask;b->memory.at(address)^=mask;}
 a->regs[27]=b->regs[27]=owner;a->regs[10]=b->regs[10]=frame;
 const bool x=a->Run(),y=b->Run();
 if(x!=okay || y!=okay || a->visible!=b->visible){
  fprintf(stderr,"PACKET_SEAL_MISMATCH address%x mask%x owner%x frame%x expected%u old%u new%u reads%zu/%zu\n",
   address,mask,owner,frame,unsigned(okay),unsigned(x),unsigned(y),a->visible.size(),b->visible.size());assert(false);
 }
 cases++;failures+=!okay;
 if(!address && okay){
  assert(b->steps<a->steps);
  printf("PASS isolated QGW3 packet seal: SAME%zu ordered private+doorbell+MMIO reads, actual instructions%u->%u;stack stores ONLY,inert2048;NOT model/native/physical/TPS\n",
   a->visible.size(),a->steps-2048,b->steps-2048);
 }
 return b->steps-2048;
}
int main(int argc,char** argv){
 assert(argc==5);SealImage old(argv[1],argv[3]),candidate(argv[2],argv[4]);
 CheckPacket(old,candidate);
 for(unsigned phase=2;phase<=15;phase++)CheckPacket(old,candidate,SealMachine::Owner+1024,1U^phase);
 for(unsigned phase:{0U,16U,UINT32_MAX})CheckPacket(old,candidate,SealMachine::Owner+1024,1U^phase,false);
 for(unsigned i=0;i<24;i++)for(unsigned bit=0;bit<32;bit++){
  const unsigned at=i<16?16+i:32+i-16;
  CheckPacket(old,candidate,SealMachine::Owner+768+at*4,1U<<bit,false);
  CheckPacket(old,candidate,SealMachine::Owner+(i<16?16+i:8+i-16)*4,1U<<bit,false);
 }
 for(unsigned i=0;i<16;i++)for(unsigned bit=0;bit<32;bit++)
  CheckPacket(old,candidate,0xffb20000U+Source(i),1U<<bit,false);
 for(unsigned bit=0;bit<32;bit++)CheckPacket(old,candidate,R::Doorbell,1U<<bit,false);
 for(unsigned nonce:{0U,0x7fffffffU,UINT32_MAX})CheckPacket(old,candidate,SealMachine::Owner+256+48*4,7U^nonce,false);
 for(unsigned owner:{0U,0x6dffcU,0x6fc00U,UINT32_MAX})CheckPacket(old,candidate,0,0,false,owner);
 CheckPacket(old,candidate,0,0,false,SealMachine::Owner,SealMachine::Owner+772);
 printf("PASS actual pinned old/candidate packet seals: %u cases/%u refusals;every observed argument+MMIO read/value/order EXACT,immediate fault and2048 inert instructions;synthetic isolated ABI ONLY,not real owner/native/model/physical/Work/TPS\n",cases,failures);
}
