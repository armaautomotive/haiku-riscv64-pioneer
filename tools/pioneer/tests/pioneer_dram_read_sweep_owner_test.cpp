/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDramReadSweepOwner.h"
#include <map>
#include <memory>
using Image=BlackholeDramReadSweepImage;
struct Rig;
struct SweepCpu:Machine {
 Rig& bus;
 explicit SweepCpu(Rig& r):Machine(),bus(r){}
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || (a>=Image::Entry && a<Image::Entry+Image::TextWords*4);}
 unsigned Index(uint32_t a)override;
 unsigned StepLimit()const override{return 50000000;}
 uint32_t Load(uint32_t a)override;
 void Store(uint32_t a,uint32_t v)override;
 uint32_t ReadCycleCsr(unsigned csr)override;
 void Instruction(uint32_t)override{assert(false && "read-only sweep emits no engine instruction");}
};
struct Rig {
 PioneerDramReadSweepOwner owner;
 PioneerDramReadSweepResult result;
 tt_dram_samples_result proof={};tt_compute_noc_result noc={};
 std::map<uint32_t,uint32_t> memory,original;
 std::unique_ptr<SweepCpu> cpu;
 std::vector<uint32_t> backing;
 uint32_t tlb[2][3]={{1,2,3},{4,5,6}},originalTlb[2][3]={{1,2,3},{4,5,6}};
 uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
  0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
 uint32_t command[10]={},config=source[0],counter[4]={0xfffffffe,0xffff0000,0xfffffffe,0xfffffffe};
 uint32_t reset=0x47800;unsigned requests=0,responses=0,waits=0;
 uint32_t droppedOld=0,droppedRequested=0;
 unsigned calls=0,resetWrites=0,clockStarts=0,clockEnds=0,coreMaskReads=0,minStack=0x70000;
 unsigned failPhase=0,failKind=0,failAt=1,seen=0;
 bool faultUsed=false,quarantined=false,running=false,pending=false,clock=false,scope=true;
 bool deviceTimeout=false,corruptPayload=false,badReport=false;
 bool corruptInitialSnapshot=false,corruptRestoredNeighbor=false;
 bool dropReportClear=false;
 bool staleFifo=false,overflowFifo=false,lateFifo=false;
 uint32_t entryOverride=0;unsigned overridePhase=0,overrideReads=0;
 bool unstableOverride=false;
 bool badCounter=false,badNonce=false,badImmutableCode=false,loseScope=false,scratchAlias=false;
 enum Kind { TlbRead=1,TlbWrite,ReadL1,WriteL1,ReadNoc,WriteNoc,ReadReset,WriteReset,ReadPci,ReadSource,ReadWindow,BeginClock,EndClock,ReadEntry };
 Rig():backing(Image::SavedWords+Image::RawWords,0xfeedabba){
  proof.version=1;proof.state=TT_DRAM_SAMPLE_ROUTES_OK;proof.routes_verified=7;
  proof.tlb_restored=proof.data_restored=1;memcpy(proof.source,source,sizeof(source));
  assert(BlackholeDramBuildRoutes(proof));
  noc.version=1;noc.state=TT_COMPUTE_NOC_PASSED;noc.tlb_restored=1;
  for(unsigned s=0;s<3;s++){
   noc.node[s][0]=0x1c611081;noc.node[s][1]=0x10000;memcpy(noc.source[s],source,sizeof(source));
  }
  for(unsigned i=0;i<Image::SavedWords;i++)memory[Image::Address(i)]=i==3?UINT32_MAX:0x68a4bc19U^(i*0x10203U);
  original=memory;
  cpu=std::make_unique<SweepCpu>(*this);assert(!cpu->steps && cpu->pc==0x6000);
 }
 bool Touch(unsigned kind){
  assert(!quarantined);calls++;
  if(result.phase==failPhase && kind==failKind && ++seen==failAt){faultUsed=true;return false;}
  return true;
 }
 static bool Range(uint32_t a,uint32_t base,unsigned bytes){return a>=base && a-base<bytes;}
 static uint32_t Payload(unsigned bank,uint32_t a){return ((a>>2)*0x10203U)^(bank*0x80101001U)^0x6be4c395U;}
 bool Matches(unsigned slot,uint32_t base,unsigned x,unsigned y,bool ro)const{
  for(unsigned i=0;i<3;i++)if(tlb[slot][i]!=BlackholeDramFixedTlbWord(base,x,y,ro,i))return false;
  return true;
 }
 unsigned SelectedBank(uint32_t route)const{
  for(unsigned b=0;b<7;b++)if(route==(proof.translated[b][0]|(proof.translated[b][1]<<6)))return b;
  assert(false);return 0;
 }
 uint32_t* SweepSavedWords(){return backing.data();}
 uint32_t* SweepScratchWords(){return backing.data()+(scratchAlias?0:Image::SavedWords);}
 unsigned SweepAllocationWords(){return unsigned(backing.size());}
 bool SweepNativeScope(const void* o,const tt_dram_samples_result* d,const tt_compute_noc_result* n,
  const uint32_t* s,const uint32_t* t,unsigned sw,unsigned tw){
  return scope && !quarantined && o==&owner && d==&proof && n==&noc && s==backing.data()
   && t==backing.data()+Image::SavedWords && sw==Image::SavedWords && tw==Image::RawWords;
 }
 void SweepQuarantineMemoryOnly(){assert(!quarantined);quarantined=true;}
 bool SweepClockBegin(){if(!Touch(BeginClock))return false;assert(!clock);clock=true;clockStarts++;return true;}
 bool SweepClockEnd(){
  if(!Touch(EndClock))return false;
  if(!(clock && !running && reset==0x47800 && !pending && memory==original
   && !memcmp(tlb,originalTlb,sizeof(tlb)) && config==source[0])){
   fprintf(stderr,"CLOCK_END preconditions clock%u running%u reset%x pending%u memory%u tlb%u config%x/%x size%zu/%zu testFault%u:%u\n",
    clock,running,reset,pending,memory==original,!memcmp(tlb,originalTlb,sizeof(tlb)),config,source[0],memory.size(),original.size(),failPhase,failKind);
   for(const auto& item:memory)if(!original.count(item.first) || original.at(item.first)!=item.second)
    fprintf(stderr,"UNRESTORED actual address%x value%x expected%x\n",item.first,item.second,original.count(item.first)?original.at(item.first):0);
  }
  assert(clock && !running && reset==0x47800 && !pending && memory==original
   && !memcmp(tlb,originalTlb,sizeof(tlb)) && config==source[0]);
  for(auto v:command)assert(!v);
  clock=false;clockEnds++;return true;
 } // clock hooks are a fixture lease, NOT a real ARC-transport qualification
 uint32_t SourceRead(unsigned i){assert(i<16);return Touch(ReadSource)?source[i]:source[i]^1;}
 uint32_t HostPciRead(unsigned p,uint16_t a,uint8_t size){
  assert(p<2);if(!Touch(ReadPci))return UINT32_MAX;
  if(a==4 && size==2)return 0x402;
  if(a==0x34 && size==1)return 0x40;
  if(a==0x40 && size==2)return 0x10;
  if(a==0x42 && size==2)return 2;
  assert(a==0x4a && size==2);return 0;
 }
 uint32_t DramTlbRead(unsigned s,unsigned i){assert(s<2 && i<3);return Touch(TlbRead)?tlb[s][i]:tlb[s][i]^1;}
 void DramTlbWrite(unsigned s,unsigned i,uint32_t v){
  assert(s<2 && i<3);
  if(Touch(TlbWrite))tlb[s][i]=v;
  else{droppedOld=tlb[s][i];droppedRequested=v;}
 }
 uint32_t ResetRead(){
  const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
  bool held=false;
  for(unsigned x:columns)for(unsigned y=2;y<=11;y++)held|=Matches(1,0xffa00000,x,y,true);
  if(held){assert(!running);coreMaskReads++;}
  else assert(Matches(1,0xffa00000,1,2,false));
  return Touch(ReadReset)?(held?0x47800:reset):0;
 }
 uint32_t SweepEntryOverrideRead(){
  assert(Matches(1,0xffa00000,1,2,false));
  if(!Touch(ReadEntry))return UINT32_MAX;
  overrideReads++;
  if(unstableOverride)return overrideReads&1?0:2;
  return entryOverride|unsigned(overridePhase && result.phase>=overridePhase);
 }
 void SweepResetWrite(uint32_t v){
  if(!Touch(WriteReset))return;
  assert(Matches(1,0xffa00000,1,2,false));resetWrites++;
  if(v==0x87ffff){assert(resetWrites==1 && !cpu->steps && cpu->pc==0x6000 && !running);}
  else if(v==0x46800){assert(resetWrites==2 && reset==0x87ffff && !cpu->steps);running=true;}
  else{assert(v==0x47800 && resetWrites==3 && reset==0x46800 && cpu->pc==Image::Park && !pending);running=false;}
  reset=v;
 }
 uint32_t ResidentNocRead(uint32_t a){
  assert(Matches(1,0xffa00000,1,2,false));
  if(!Touch(ReadNoc))return UINT32_MAX;
  if(a==0x64)return !(config&0x10000)?0:
   (staleFifo || (lateFifo && result.phase==5 && cpu->pc==Image::Park))?0x1010100f:0x10101010;
  if(a==0x68)return overflowFifo && (config&0x10000)?1:0;
  for(unsigned i=0;i<16;i++)if(a==BlackholeDramSourceOffset(i))return i?source[i]:config;
  if(a==0x178)return 0;
  for(unsigned i=0;i<4;i++)if(a==0x40+i*0x800)return !i && pending?1:0;
  for(unsigned i=0;i<32;i++)if(a==0x240+i*4)return !i && pending?1:0;
  for(unsigned i=0;i<4;i++)if(a==0x208+i*4)return counter[i]+unsigned(badCounter && result.phase==5 && cpu->pc==Image::Park);
  assert(a<40 && !(a&3));return command[a/4];
 }
 void ResidentNocWrite(uint32_t a,uint32_t v){
  assert(!running && !pending && Matches(1,0xffa00000,1,2,false));
  assert(a!=0x40);if(!Touch(WriteNoc))return;
  if(a==0x100){assert(v==source[0] || v==(source[0]|0x10000));config=v;return;}
  assert(a<40 && !(a&3));command[a/4]=v;
 }
 bool SweepReadChecked(uint32_t a,uint32_t& value){
  assert(Matches(0,0,1,2,false));if(!Touch(ReadL1))return false;
  assert(memory.count(a));value=memory.at(a);
  if(corruptInitialSnapshot && result.phase==2 && a==Image::Address(3) && !faultUsed){value^=1;faultUsed=true;}
  if(badReport && result.phase==5 && a==PioneerDramReadSweep::Report && value==PioneerDramReadSweep::ReportMagic)value^=1;
  if(badNonce && result.phase==5 && a==PioneerDramReadSweep::Report+8 && cpu->pc==Image::Park)value^=1;
  return true;
 }
 bool SweepWrite(uint32_t a,uint32_t v){
  assert(!running && !pending && Matches(0,0,1,2,false) && memory.count(a));
  if(!Touch(WriteL1))return false;memory[a]=v;
  if(dropReportClear && result.phase==3 && a==PioneerDramReadSweep::Report && !v)memory[a]=original.at(a);
  if(corruptRestoredNeighbor && result.phase==8 && a==0x90000)memory[Image::Address(3)]^=1;
  return true;
 }
 bool SweepWindowReadChecked(uint32_t offset,uint32_t& value){
  assert(!running && !pending && offset<0x200000 && !(offset&3));
  if(!Touch(ReadWindow))return false;
  bool found=false;
  for(unsigned b=0;b<7;b++)if(Matches(0,0x04600000,proof.translated[b][0],proof.translated[b][1],true)){
   value=Payload(b,0x04600000+offset);found=true;
  }
  assert(found);return true;
 }
 uint32_t SweepPcRead(){assert(!quarantined && running);calls++;return cpu->pc;}
 void Pause(){
  assert(!quarantined);
  for(unsigned i=0;i<2048 && running && cpu->pc!=Image::Park && cpu->pc!=Image::FaultPark;i++)cpu->Step();
  if(badImmutableCode && cpu->pc==Image::Park)memory[Image::Address(3)]^=1;
  if(loseScope)scope=false;
 }
 uint32_t DeviceRead(uint32_t a){
  assert(running && !quarantined && !(a&3));
  if(memory.count(a))return memory.at(a);
  assert(a>=0xffb20000 && a<0xffb22000);const unsigned offset=a-0xffb20000;
  if(offset<40)return command[offset/4];
  for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800){
   if(i)return 0;
   if(pending && ++waits>=3 && !deviceTimeout){
    unsigned bank=SelectedBank(command[2]);
    for(unsigned w=0;w<command[8]/4;w++)memory[command[3]+w*4]=Payload(bank,command[0]+w*4)^((corruptPayload && w==17)?1:0);
    pending=false;responses++;counter[0]++;counter[1]+=command[8]/64;counter[2]++;counter[3]++;
   }
   return pending?1:0;
  }
  if(offset==0x208)return counter[0];
  for(unsigned i=0;i<32;i++)if(offset==0x240+i*4)return !i && pending?1:0;
  assert(false);return 0;
 }
 void DeviceWrite(uint32_t a,uint32_t v){
  assert(running && !quarantined && !(a&3));
  if(Range(a,0x6e000,8192)){minStack=std::min(minStack,a);memory[a]=v;return;}
  if(Range(a,PioneerDramReadSweep::Report,64)){memory[a]=v;return;}
  assert(a>=0xffb20000 && a<=0xffb20040);const unsigned offset=a-0xffb20000;
  if(offset<40){assert(!pending);command[offset/4]=v;return;}
  assert(offset==0x40 && v==1 && !pending);
  const unsigned shift=memory.at(PioneerDramReadSweep::Job+12),bytes=1U<<shift;
  assert(command[0]==PioneerDramReadSweep::Base+((requests<<shift)&(PioneerDramReadSweep::Span-1)));
  assert(command[1]==0 && command[2]==memory.at(PioneerDramReadSweep::Job+16)
   && command[3]==0x90000 && command[4]==0 && command[5]==129 && command[6]==0
   && command[7]==0 && command[8]==bytes && command[9]==0 && requests==responses);
  SelectedBank(command[2]);requests++;pending=true;waits=0;
 }
 bool Execute(unsigned bank=0,unsigned shift=14,unsigned sweeps=1){return owner.Execute(*this,proof,noc,bank,shift,sweeps,0x32435a67,result);}
};
unsigned SweepCpu::Index(uint32_t a){assert(CodeAddress(a) && !(a&3));l1[8191]=bus.memory.at(a);return 8191;}
uint32_t SweepCpu::Load(uint32_t a){return bus.DeviceRead(a);}
void SweepCpu::Store(uint32_t a,uint32_t v){bus.DeviceWrite(a,v);}
uint32_t SweepCpu::ReadCycleCsr(unsigned csr){
 assert(bus.running && (csr==0xb00 || csr==0xb80));
 const uint64_t ticks=UINT64_C(0x1ffffe000)+steps;
 return csr==0xb00?uint32_t(ticks):uint32_t(ticks>>32);
}
int main(int argc,char** argv){
 assert(argc==3);const auto boot=Read(argv[1]),text=Read(argv[2]);
 assert(boot.size()==4 && Word(boot,0)==Image::Code(0) && text.size()==Image::TextWords*4);
 for(unsigned i=0;i<Image::TextWords;i++)assert(Word(text,i*4)==Image::Code(i+1));
 std::map<uint32_t,bool> addresses;
 for(unsigned i=0;i<Image::SavedWords;i++){const uint32_t a=Image::Address(i);assert(a!=UINT32_MAX && !addresses[a]);addresses[a]=true;}
 assert(Image::Address(Image::SavedWords)==UINT32_MAX);
 unsigned passed=0,refused=0;
 for(unsigned bank=0;bank<7;bank++){
  Rig r;assert(r.Execute(bank) && r.result.closed && !r.result.unsafe && !r.quarantined);
  assert(r.memory==r.original && !memcmp(r.tlb,r.originalTlb,sizeof(r.tlb)) && !r.clock
   && r.resetWrites==3 && r.clockStarts==1 && r.clockEnds==1 && r.coreMaskReads==480);
  assert(r.result.savedWords==Image::SavedWords && r.result.restoredWords==Image::SavedWords && r.result.comparedWords==4096);
  assert(r.cpu->steps && r.cpu->pc==Image::Park && r.minStack>=0x6e000);
  assert(!r.cpu->matrixInstructions && !r.cpu->moveInstructions && !r.cpu->configWrites);
  const unsigned calls=r.calls;PioneerDramReadSweepResult twice;
  assert(!r.owner.Execute(r,r.proof,r.noc,bank,14,1,1,twice) && calls==r.calls);passed++;
 }
 for(unsigned shift:{6U,10U,12U}){Rig r;assert(r.Execute(0,shift));passed++;}
 {Rig r;assert(r.Execute(3,14,2));passed++;}
 {Rig r;r.entryOverride=0xfffffffe;assert(r.Execute() && r.overrideReads);passed++;}
 for(unsigned phase:{1U,2U,3U,4U,5U,6U,7U,8U}){
  Rig r;r.overridePhase=phase;
  assert(!r.Execute() && r.quarantined && r.result.unsafe && !r.result.closed);
  if(phase<=3)assert(!r.resetWrites && !r.cpu->steps);
  if(phase==1)assert(r.memory==r.original && !r.result.savedWords);
  if(phase==4)assert(r.resetWrites==1 && !r.cpu->steps && !r.result.launched);
  assert(!r.clockEnds);refused++;
 }
 {Rig r;r.unstableOverride=true;assert(!r.Execute() && r.quarantined
   && !r.resetWrites && !r.cpu->steps && r.memory==r.original);refused++;}
 {Rig r;r.failPhase=1;r.failKind=Rig::TlbWrite;r.failAt=120;
  assert(r.Execute() && r.faultUsed && r.droppedOld==r.droppedRequested && r.result.closed
   && r.memory==r.original && !memcmp(r.tlb,r.originalTlb,sizeof(r.tlb)));
  passed++;} // dropping a redundant same-value write causes NO hardware change
 for(unsigned fault=0;fault<3;fault++){
  Rig r;r.staleFifo=fault==0;r.overflowFifo=fault==1;r.lateFifo=fault==2;
  assert(!r.Execute() && r.quarantined && r.result.unsafe && !r.result.closed);
  if(fault<2)assert(!r.resetWrites && !r.cpu->steps && r.result.phase==3);
  else assert(r.resetWrites==2 && r.cpu->pc==Image::Park && r.result.phase==5 && !r.result.held);
  refused++;
 }
 for(unsigned fault=0;fault<10;fault++){
  Rig r;
  if(fault==0)r.proof.routes_verified=6;
  if(fault==1)r.proof.translated[0][0]^=1;
  if(fault==2)r.noc.source[2][3]^=1;
  if(fault==3)r.noc.commands[1][0]=1;
  if(fault==4)r.noc.node[2][0]^=1;
  if(fault==5)r.noc.outstanding[2][9]=1;
  if(fault==6)r.scope=false;
  if(fault==7)r.result.held=1;
  if(fault==8)r.backing.pop_back();
  if(fault==9)r.scratchAlias=true;
  assert(!r.Execute() && !r.calls && !r.cpu->steps && !r.quarantined);refused++;
 }
 const uint32_t invalidParameters[][4]={{7,14,1,1},{UINT32_MAX,14,1,1},
  {0,0,1,1},{0,31,1,1},{0,32,1,1},{0,14,0,1},{0,14,257,1},{0,14,UINT32_MAX,1},{0,14,1,0}};
 for(const auto& p:invalidParameters){
  Rig r;assert(!r.owner.Execute(r,r.proof,r.noc,p[0],p[1],p[2],p[3],r.result)
   && !r.calls && !r.cpu->steps && !r.quarantined);refused++;
 }
 const unsigned lateFailures[][3]={{1,Rig::TlbRead,6},{1,Rig::TlbRead,7},{1,Rig::TlbRead,12},
  {1,Rig::TlbWrite,3},{1,Rig::TlbWrite,119},
  {2,Rig::ReadL1,314},{2,Rig::ReadL1,315},{2,Rig::ReadL1,6490},{2,Rig::ReadL1,6491},{2,Rig::ReadL1,12980},
  {3,Rig::WriteL1,313},{3,Rig::WriteL1,314},{3,Rig::WriteL1,345},{3,Rig::WriteL1,346},
  {8,Rig::WriteL1,313},{8,Rig::WriteL1,6490},
  {8,Rig::ReadL1,6490},{8,Rig::ReadL1,6491},{8,Rig::ReadL1,12980},
  {8,Rig::WriteNoc,3},{8,Rig::WriteNoc,4},{8,Rig::WriteNoc,6},{8,Rig::WriteNoc,9},{8,Rig::WriteNoc,11},
  {9,Rig::TlbWrite,3},{9,Rig::TlbWrite,6},{9,Rig::TlbRead,6},{9,Rig::TlbRead,12}};
 for(const auto& f:lateFailures){
  Rig r;r.failPhase=f[0];r.failKind=f[1];r.failAt=f[2];
  const bool completed=r.Execute();
  if(completed || !r.faultUsed || !r.quarantined || !r.result.unsafe || r.result.closed)
   fprintf(stderr,"LATE_FAULT phase%u kind%u ordinal%u completed%u used%u quarantined%u unsafe%u closed%u\n",
    f[0],f[1],f[2],completed,r.faultUsed,r.quarantined,r.result.unsafe,r.result.closed);
  assert(!completed && r.faultUsed && r.quarantined && r.result.unsafe && !r.result.closed);
  refused++;
 }
 for(unsigned f=0;f<4;f++){
  Rig r;r.badCounter=f==0;r.badNonce=f==1;r.badImmutableCode=f==2;r.loseScope=f==3;
  assert(!r.Execute() && r.quarantined && r.result.unsafe && !r.result.closed && r.resetWrites==2);
  assert(!r.clockEnds && !r.result.restoredWords);refused++;
 }
 const unsigned failures[][2]={{1,Rig::BeginClock},{1,Rig::TlbRead},{1,Rig::TlbWrite},{1,Rig::ReadReset},
  {1,Rig::ReadEntry},{4,Rig::ReadEntry},{5,Rig::ReadEntry},
  {2,Rig::ReadL1},{2,Rig::ReadSource},{3,Rig::WriteNoc},{3,Rig::WriteL1},{3,Rig::ReadL1},
  {4,Rig::WriteReset},{4,Rig::ReadReset},{5,Rig::ReadSource},{5,Rig::ReadPci},
  {6,Rig::WriteReset},{6,Rig::ReadL1},{7,Rig::ReadWindow},{8,Rig::WriteL1},{8,Rig::ReadL1},
  {8,Rig::WriteNoc},{9,Rig::TlbWrite},{9,Rig::TlbRead},{9,Rig::EndClock}};
 for(const auto& f:failures){
  Rig r;r.failPhase=f[0];r.failKind=f[1];
  assert(!r.Execute() && r.faultUsed && r.quarantined && r.result.unsafe && !r.result.closed);
  const unsigned calls=r.calls;PioneerDramReadSweepResult again;
  assert(!r.owner.Execute(r,r.proof,r.noc,0,14,1,1,again) && calls==r.calls);refused++;
 }
 for(unsigned f=0;f<3;f++){
  Rig r;r.deviceTimeout=f==0;r.corruptPayload=f==1;r.badReport=f==2;
  assert(!r.Execute() && r.quarantined && r.result.unsafe && !r.result.closed && !r.clockEnds);
  if(!f)assert(r.requests==1 && r.pending && r.resetWrites==2);
  if(f==1)assert(r.result.held && r.resetWrites==3 && !r.result.restoredWords);
  refused++;
 }
 for(unsigned f=0;f<3;f++){
  Rig r;r.corruptInitialSnapshot=f==0;r.corruptRestoredNeighbor=f==1;
  r.dropReportClear=f==2;
  assert(!r.Execute() && r.quarantined && r.result.unsafe && !r.result.closed && !r.clockEnds);
  if(f!=1)assert(!r.requests && !r.resetWrites);
  refused++;
 }
 printf("PASS actual isolated RV32 owner histories%u/refusals%u;ALL6490 old L1 words+NIU/TLB restored on clean closure;NO hardware bandwidth/native driver/ARC transport claim\n",passed,refused);
}
