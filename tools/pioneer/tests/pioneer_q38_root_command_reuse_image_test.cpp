/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SENTINEL_ROOT_IMAGE_MODEL_ONLY
#include "pioneer_q38_sentinel_root_image_test.cpp"
// All inherited packet/source/PC/proof/DMA observers remain active. Clear
// read evidence at EACH issue so a previous request can never prove this
// request's command. Hardware-reserved control changes happen ONLY after
// completed peer packets;the normal final PCI command remains unchanged.
template<class Base> struct CommandAudit:CycleRoot<Base>{
 using CycleRoot<Base>::CycleRoot;
 unsigned commandReads=0,commandWrites=0,corruptWord=10,corruptBeforePacket=0;
 bool mutateReserved=false;
 std::vector<std::array<uint32_t,10>> issued;
 uint32_t Load(uint32_t at)override{
  if(this->Range(at,0xffb20000,40)){
   commandReads++;unsigned w=(at-0xffb20000)/4;
   uint32_t result=CycleRoot<Base>::Load(at);
   return result^(w==corruptWord&&this->packets+1==corruptBeforePacket?1U:0U);
  }
  return CycleRoot<Base>::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  if(this->Range(at,0xffb20000,40))commandWrites++;
  if(at==0xffb20040){
   for(bool seen:this->seen)assert(seen);
   std::array<uint32_t,10> c;for(unsigned w=0;w<10;w++)c[w]=this->command[w];issued.push_back(c);
   CycleRoot<Base>::Store(at,value);for(bool& seen:this->seen)seen=false;return;
  }
  CycleRoot<Base>::Store(at,value);
 }
 void Packet()override{
  bool pci=(this->command[7]==0x212?this->command[5]:this->command[2])==1555;
  CycleRoot<Base>::Packet();
  if(!pci&&!this->pending&&mutateReserved)this->command[7]|=0x80000000U;
 }
};
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);unsigned cases=0;
 for(unsigned workers:{1U,2U,7U,119U})for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U}){
  printf("COMMAND_REUSE_ROOT workers%u type%u K%u\n",workers,type,k);
  CommandAudit<DispatchMachine> old(argv[1],workers,type,k),next(argv[2],workers,type,k);
  old.mutateReserved=next.mutateReserved=true;
  for(unsigned trial=1;trial<=2;trial++){
   if(trial==2){old.Configure(2);next.Configure(2);old.DmaHeader();next.DmaHeader();old.reads.clear();next.reads.clear();old.dmaPackets=next.dmaPackets=0;std::fill(old.host.begin(),old.host.end(),0xdeadbeef);std::fill(next.host.begin(),next.host.end(),0xdeadbeef);}
   assert(old.RunRoot()&&next.RunRoot());old.CheckCycles();next.CheckCycles();old.VerifyCompactHost();next.VerifyCompactHost();
   assert(old.host==next.host&&old.issued==next.issued&&old.packets==next.packets&&old.responses==next.responses&&old.gathers==next.gathers);
   assert(next.commandWrites<old.commandWrites);
   for(unsigned w=0;w<57;w++)assert(old.memory.at(0x7f800+w*4)==next.memory.at(0x7f800+w*4));
   cases++;
  }
  old.active=next.active=119;old.Configure(3,true);next.Configure(3,true);old.reads.clear();next.reads.clear();
  unsigned ow=old.commandWrites,nw=next.commandWrites,orr=old.commandReads,nr=next.commandReads;
  assert(old.RunRoot()&&next.RunRoot());assert(old.issued==next.issued);
  assert(old.commandWrites-ow==next.commandWrites-nw&&old.commandReads-orr==next.commandReads-nr); // STOP original
  for(unsigned w=0;w<64;w++)assert(old.memory.at(0x7f800+w*4)==next.memory.at(0x7f800+w*4));cases++;
 }
 // CHECKED explicitly retains the full original command write sequence.
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U}){
  CommandAudit<DispatchMachine> old(argv[1],7,type,k),next(argv[2],7,type,k);
  for(unsigned i=8;i<13;i++){old.memory[0x74000+i*4]=0;next.memory[0x74000+i*4]=0;}
  assert(old.RunRoot()&&next.RunRoot()&&old.issued==next.issued&&old.host==next.host);
  assert(old.commandReads==next.commandReads&&old.commandWrites==next.commandWrites);
  for(unsigned w=0;w<64;w++)assert(old.memory.at(0x7f800+w*4)==next.memory.at(0x7f800+w*4));cases++;
 }
 // Instruction-count-independent peer readiness;NEVER treat a reduced
 // polling count or fabricated clock as hardware speed evidence.
 for(unsigned workers:{2U,7U,119U})for(unsigned late:{0U,1U}){
  CommandAudit<TimedPollFabric> old(argv[1],workers),next(argv[2],workers);old.lateWorker=next.lateWorker=late;
  assert(old.RunRoot()&&next.RunRoot());old.CheckCycles();next.CheckCycles();old.VerifyCompactHost();next.VerifyCompactHost();assert(old.host==next.host);
  for(unsigned w=0;w<workers;w++)assert(next.pcReads[w]==2&&next.resetReads[w]==1&&next.peers[w].finished==next.peers[w].requested);cases++;
 }
 for(unsigned fault=0;fault<10;fault++)for(unsigned packet:{1U,20U,40U}){
  CommandAudit<DispatchMachine> m(argv[2],7);m.corruptWord=fault;m.corruptBeforePacket=packet;
  assert(!m.RunRoot()&&m.unknown&&!m.dmaPackets&&m.packets==packet-1);m.VerifyFaultInert();
 }
 for(unsigned field=0;field<64;field++){
  CommandAudit<DispatchMachine> m(argv[2],7);m.corruptWorker=7;m.corruptProof=field;
  assert(!m.RunRoot()&&m.unknown&&!m.dmaPackets);m.VerifyFaultInert();
 }
 for(unsigned packet:{1U,20U,40U}){
  CommandAudit<DispatchMachine> m(argv[2],7);m.faultAt=packet;
  assert(!m.RunRoot()&&m.unknown&&m.pending&&m.packets==packet);m.VerifyFaultInert();
 }
 for(unsigned worker:{1U,3U,7U}){
  CommandAudit<DispatchMachine> m(argv[2],7);m.badPcWorker=worker;
  assert(!m.RunRoot()&&m.unknown&&!m.dmaPackets);m.VerifyFaultInert();
 }
 for(unsigned packet:{1U,4U,8U}){
  CommandAudit<DispatchMachine> m(argv[2],119);m.failDma=packet;
  assert(!m.RunRoot()&&m.unknown&&m.pending&&m.dmaPackets==packet);m.VerifyFaultInert();
 }
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U}){
  CommandAudit<CooperativeDispatch> m(argv[2],argv[3],2,type,k);assert(m.RunRoot());m.CheckCycles();m.VerifyCompactHost();
  for(auto& peer:m.actual)assert(peer->CorrectRows()&&!peer->dotProbeReads);cases++;
 }
 printf("PASS OFFLINE ACTUAL command-reuse root:%u Work/repeated/STOP+real-peer cases;EVERY command's fresh10-field read proof,identical packets/routes/57safetyfields/PC/source/NoC/DMA/exact outputs,hardware-reserved control changes repaired;30 first/middle/late per-field readback failures+64proof/PC/NoC/DMA faults retain UNKNOWN/inert;NO native/speed qualification\n",cases);
}
