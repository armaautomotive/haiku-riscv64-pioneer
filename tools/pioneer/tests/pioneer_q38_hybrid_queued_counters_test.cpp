/* SPDX-License-Identifier: MIT */
// Execute the emitted QGW7 queued-counter helper and real recurrent packet
// seal on synthetic out-of-order NoC responses. No hardware or model claim.
#define main pioneer_legacy_counter_gather_main
#include "pioneer_q38_counter_gather_instruction_test.cpp"
#undef main

struct HybridGatherMachine:GatherMachine {
 std::vector<uint8_t> bf16;
 HybridGatherMachine(const AuditImage& image,const char* directory,unsigned worker)
  :GatherMachine(image,worker),bf16(Read((std::string(directory)+"/bf16.bin").c_str())){
  for(unsigned i=0;i<8;i++)memory[Owner+768+(32+i)*4]=memory[R::Header+(32+i)*4]=memory.at(Owner+(8+i)*4);
 }
 bool CodeAddress(unsigned a)const override{
  return (a>=0x20000 && a-0x20000<bf16.size()) || AuditMachine::CodeAddress(a);
 }
 unsigned Index(unsigned a)override{
  if(a>=0x20000 && a-0x20000<bf16.size()){
   assert(!(a&3));l1[8180]=Word(bf16,a-0x20000);return 8180;
  }
  return AuditMachine::Index(a);
 }
 bool RunHybrid(){
  while(pc!=Return && pc!=image.entry[2])Step();const bool okay=pc==Return;
  if(okay){
   assert(submitted==8 && responded==8 && started==8 && maximumQueued==8 && maximumOutstanding==8);
   assert(queued.empty() && !active && inflight.empty() && !outstanding && outputWrites==8 && trackedWrites==8);
   for(unsigned i=0;i<8;i++){
    assert(memory.at(Output+i*4)==peer.at(0xffb20000U+Counter(i)));
    assert(memory.at(Tracked+i*4)==uint32_t(original[i]+(i==0 || i==1 || i==2 || i==3 || i==6?8U:0U)));
   }
  }else{
   assert(pc==image.entry[2] && !outputWrites && !trackedWrites);
   for(unsigned i=0;i<8;i++)assert(memory.at(Tracked+i*4)==original[i] && memory.at(Output+i*4)==0x7fc00001U);
  }
  const auto before=memory;const unsigned commands=submitted,writeCount=writes,received=responded;
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==(okay?Return:image.entry[2]) && memory==before && submitted==commands && writes==writeCount && responded==received);
  return okay;
 }
};

int main(int argc,char** argv){
 assert(argc==4);AuditImage image(argv[1],argv[2]);
 std::ifstream addresses(argv[3]);addresses>>std::hex>>image.entry[0]>>image.entry[2];assert(addresses.good());
 assert(image.entry[0]>=0x20000 && image.entry[0]<0x30000 && image.entry[2]==0x60000);
 unsigned cases=0;
 for(unsigned worker:{0U,95U,118U})for(unsigned delay:{1U,3U,17U})for(unsigned order=0;order<3;order++){
  HybridGatherMachine m(image,argv[1],worker);m.delay=delay;m.order=order;assert(m.RunHybrid());cases++;
 }
 for(unsigned i=0;i<38;i++)for(unsigned bit=0;bit<32;bit++){
  HybridGatherMachine m(image,argv[1],0);
  const unsigned at=i<4?0xffb20040U+i*0x800:i==4?0xffb20064U:i==5?0xffb20068U:0xffb20240U+(i-6)*4;
  m.memory.at(at)^=1U<<bit;const bool ignored=i>=6 && bit>=8;
  assert(m.RunHybrid()==ignored);if(!ignored)assert(!m.submitted);cases++;
 }
 for(unsigned packet=0;packet<8;packet++)for(unsigned word=0;word<10;word++){
  HybridGatherMachine m(image,argv[1],0);m.badReadPacket=packet;m.badReadWord=word;
  assert(!m.RunHybrid() && m.submitted==packet);cases++;
 }
 for(unsigned packet=0;packet<8;packet++)for(unsigned source=0;source<16;source++){
  HybridGatherMachine m(image,argv[1],0);m.badSourcePacket=packet;m.badSource=source;
  assert(!m.RunHybrid() && m.submitted==packet);cases++;
 }
 for(unsigned word=0;word<10;word++){
  HybridGatherMachine m(image,argv[1],0);m.badPostWord=word;
  assert(!m.RunHybrid() && m.submitted==8 && m.responded==8);cases++;
 }
 for(unsigned bit=0;bit<4;bit++){
  HybridGatherMachine m(image,argv[1],0);m.postOverflow=1U<<bit;assert(!m.RunHybrid() && m.submitted==8);cases++;
 }
 for(unsigned tid=0;tid<32;tid++){
  HybridGatherMachine m(image,argv[1],0);m.postTid=tid;assert(!m.RunHybrid() && m.responded==8);cases++;
 }
 for(unsigned index=0;index<8;index++){
  HybridGatherMachine m(image,argv[1],0);m.badCounter=index;assert(!m.RunHybrid() && m.responded==8);cases++;
 }
 {
  HybridGatherMachine m(image,argv[1],0);m.fifoEnabled=false;
  for(unsigned at:{unsigned(AuditMachine::Owner+16*4),unsigned(AuditMachine::Owner+768+16*4),
   unsigned(R::Boot+16*4),unsigned(R::Header+16*4),0xffb20100U})m.memory.at(at)&=~0x10000U;
  assert(!m.RunHybrid() && !m.submitted);cases++;
 }
 for(unsigned which=0;which<2;which++){
  HybridGatherMachine m(image,argv[1],0);m.neverInitiate=which==0;m.dropResponse=which==1;
  assert(!m.RunHybrid() && m.submitted==8);cases++;
 }
 printf("PASS QGW7 emitted queued counter helper/%u synthetic cases:8 simultaneous requests,reverse replies,source/command/TID/counter faults and missing responses;no device or TPS\n",cases);
}
