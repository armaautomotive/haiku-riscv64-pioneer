/* SPDX-License-Identifier: MIT */
#define PIONEER_DRAM_SWEEP_QUEUED 1
#define PIONEER_DRAM_SWEEP_OWNER_TEST_COMPONENT
#include "pioneer_dram_read_sweep_owner_test.cpp"
#include <deque>
// Same ACTUAL generic owner and emitted queued instructions,with a separate
// asynchronous FIFO/response endpoint. Clock/scope remain fixture hooks.
struct QueuedRig:Rig {
 struct Packet {uint32_t source,dest,bytes,route;unsigned due;};
 std::deque<Packet> fifo;std::vector<Packet> flying;
 unsigned ticks=0,depth=1,maxPending=0,reordered=0,corruptSlot=UINT32_MAX;
 bool stalledFifo=false,lostDoorbell=false;
 void Tick(){
  ticks++;
  if(!fifo.empty() && ticks%3==0 && !stalledFifo){
   auto p=fifo.front();fifo.pop_front();
   const unsigned slot=(p.dest-Program::Raw)/p.bytes;
   p.due=ticks+80+(depth-1-slot)*32;flying.push_back(p);counter[2]++;counter[3]++;
  }
  for(unsigned i=unsigned(flying.size());i>0;i--){
   const auto p=flying[i-1];if(p.due>ticks || deviceTimeout)continue;
   if(i>1)reordered++;
   const unsigned slot=(p.dest-Program::Raw)/p.bytes;
   for(unsigned w=0;w<p.bytes/4;w++)memory[p.dest+w*4]=Payload(SelectedBank(p.route),p.source+w*4)
    ^unsigned((corruptPayload || corruptSlot==slot) && w==17);
   flying.erase(flying.begin()+i-1);responses++;counter[0]++;counter[1]+=p.bytes/64;
  }
  pending=!fifo.empty() || !flying.empty();
 }
 uint32_t ResidentNocRead(uint32_t a){
  if(a!=0x64)return Rig::ResidentNocRead(a);
  assert(Matches(1,0xffa00000,1,2,false));if(!Touch(ReadNoc))return UINT32_MAX;
  if(!(config&0x10000))return 0;
  if(staleFifo || (lateFifo && result.phase==5 && cpu->pc==Image::Park))return 0x1010100f;
  return 0x10101000|uint32_t(16-fifo.size());
 }
 uint32_t DeviceRead(uint32_t a)override{
  assert(running && !quarantined && !(a&3));
  if(memory.count(a))return memory.at(a);
  assert(a>=0xffb20000 && a<0xffb22000);Tick();const unsigned o=a-0xffb20000;
  if(o<40)return command[o/4];
  if(o==0x100)return config;
  if(o==0x64)return 0x10101000|uint32_t(16-fifo.size());
  if(o==0x68)return 0;
  for(unsigned i=0;i<4;i++)if(o==0x40+i*0x800)return 0;
  if(o==0x208)return counter[0];
  for(unsigned i=0;i<32;i++)if(o==0x240+i*4)return !i?uint32_t(flying.size()):0;
  assert(false);return 0;
 }
 void DeviceWrite(uint32_t a,uint32_t value)override{
  assert(running && !quarantined && !(a&3));
  if(Range(a,0x6e000,8192)){minStack=std::min(minStack,a);memory[a]=value;return;}
  if(Range(a,Program::Report,64)){memory[a]=value;return;}
  assert(a>=0xffb20000 && a<=0xffb20040);const unsigned o=a-0xffb20000;
  if(o<40){command[o/4]=value;return;}
  assert(o==0x40 && value==1 && fifo.size()<16);
  const unsigned shift=memory.at(Program::Job+12),bytes=1U<<shift,slot=requests&(depth-1);
  if(!slot)assert(!pending && requests==responses);
  assert(command[0]==Program::Base+((requests<<shift)&(Program::Span-1))
   && command[1]==0 && command[2]==memory.at(Program::Job+16)
   && command[3]==Program::Raw+slot*bytes && command[4]==0 && command[5]==129
   && command[6]==0 && command[7]==0 && command[8]==bytes && command[9]==0);
  for(const auto& p:fifo)assert(p.dest+bytes<=command[3] || command[3]+bytes<=p.dest);
  for(const auto& p:flying)assert(p.dest+bytes<=command[3] || command[3]+bytes<=p.dest);
  requests++;
  if(!lostDoorbell || requests!=1)fifo.push_back({command[0],command[3],bytes,command[2],0});
  pending=!fifo.empty() || !flying.empty();
  maxPending=std::max(maxPending,unsigned(fifo.size()+flying.size()));assert(maxPending<=depth);
 }
 bool Execute(unsigned bank=0,unsigned shift=10,unsigned sweeps=1,unsigned count=16){
  depth=count;return owner.Execute(*this,proof,noc,bank,shift,sweeps,0x32435a67,result,count);
 }
};
int main(int argc,char** argv){
 assert(argc==3);const auto boot=Read(argv[1]),text=Read(argv[2]);
 assert(boot.size()==4 && Word(boot,0)==Image::Code(0) && text.size()==Image::TextWords*4);
 for(unsigned i=0;i<Image::TextWords;i++)assert(Word(text,i*4)==Image::Code(i+1));
 std::map<uint32_t,bool> addresses;
 for(unsigned i=0;i<Image::SavedWords;i++){const auto a=Image::Address(i);assert(a!=UINT32_MAX && !addresses[a]);addresses[a]=true;}
 assert(Image::Address(Image::SavedWords)==UINT32_MAX && Image::SavedWords==6641);
 unsigned good=0,bad=0;
 for(unsigned bank=0;bank<7;bank++){
  QueuedRig r;assert(r.Execute(bank) && r.result.closed && !r.quarantined && !r.result.unsafe);
  assert(r.memory==r.original && !memcmp(r.tlb,r.originalTlb,sizeof(r.tlb)) && !r.clock
   && r.resetWrites==3 && r.clockStarts==1 && r.clockEnds==1 && r.coreMaskReads==480);
  assert(r.result.savedWords==6641 && r.result.restoredWords==6641 && r.result.comparedWords==4096
   && r.maxPending>1 && r.reordered && r.requests==8192 && r.responses==8192);
  const unsigned calls=r.calls;PioneerDramReadSweepResult again;
  assert(!r.owner.Execute(r,r.proof,r.noc,bank,10,1,1,again,16) && r.calls==calls);good++;
 }
 for(const auto& g:std::vector<std::pair<unsigned,unsigned>>{{6,16},{10,1},{10,2},{10,4},{10,8},{12,4},{14,1}}){
  QueuedRig r;assert(r.Execute(0,g.first,1,g.second) && r.result.comparedWords==((1U<<g.first)*g.second)/4
   && r.memory==r.original);good++;
 }
 {QueuedRig r;assert(r.Execute(3,10,2,16) && r.requests==16384 && r.memory==r.original);good++;}
 for(unsigned slot=0;slot<16;slot++){
  QueuedRig r;r.corruptSlot=slot;
  assert(!r.Execute() && r.quarantined && r.result.held && r.result.phase==7
   && r.result.comparedWords==slot*256+17 && !r.result.restoredWords && !r.clockEnds);bad++;
 }
 for(unsigned count:{0U,3U,17U,UINT32_MAX}){
  QueuedRig r;assert(!r.Execute(0,10,1,count) && !r.calls && !r.quarantined);bad++;
 }
 {QueuedRig r;assert(!r.Execute(0,14,1,2) && !r.calls && !r.quarantined);bad++;}
 for(unsigned fault=0;fault<9;fault++){
  QueuedRig r;
  if(fault==0)r.staleFifo=true;
  if(fault==1)r.overflowFifo=true;
  if(fault==2)r.lateFifo=true;
  if(fault==3)r.deviceTimeout=true;
  if(fault==4)r.stalledFifo=true;
  if(fault==5)r.lostDoorbell=true;
  if(fault==6)r.badReport=true;
  if(fault==7)r.overridePhase=4;
  if(fault==8)r.corruptRestoredNeighbor=true;
  assert(!r.Execute() && r.quarantined && r.result.unsafe && !r.result.closed && !r.clockEnds);
  if(fault<2)assert(!r.resetWrites && !r.cpu->steps);
  if(fault>=3 && fault<=5)assert(r.requests==16 && r.resetWrites==2);
  bad++;
 }
 for(const auto& f:std::vector<std::pair<unsigned,unsigned>>{{1,Rig::ReadEntry},{2,Rig::ReadL1},
  {3,Rig::WriteL1},{4,Rig::WriteReset},{5,Rig::ReadSource},{6,Rig::WriteReset},
  {7,Rig::ReadWindow},{8,Rig::WriteL1},{8,Rig::ReadL1},{9,Rig::TlbWrite},{9,Rig::EndClock}}){
  QueuedRig r;r.failPhase=f.first;r.failKind=f.second;
  assert(!r.Execute() && r.faultUsed && r.quarantined && !r.result.closed);bad++;
 }
 printf("PASS actual queued owner %u valid/%u refused;ALL6641 snapshot words restored;entire last batch independently compared;NO native hardware/ARC/bandwidth/Work claim\n",good,bad);
}
