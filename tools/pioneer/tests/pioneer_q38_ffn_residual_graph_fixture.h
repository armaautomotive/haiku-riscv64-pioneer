/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_RESIDUAL_GRAPH_FIXTURE_H
#define PIONEER_Q38_FFN_RESIDUAL_GRAPH_FIXTURE_H
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
// TEST endpoint ONLY. ROOT instructions,not the fixture,save/add/publish.
template<class Base> struct ResidualGraphFixture:Base {
 std::array<uint32_t,5120> savedRaw={},savedInput={};
 unsigned issued=0,completed=0,drop=0,mutateSource=0,mutateManifest=0,badCounterAt=0,mutateDown=0;
 unsigned writes=0,sumStores=0,lastFaultPacket=0;
 uint32_t downValue=0;
 template<class... Args> explicit ResidualGraphFixture(Args&&... args):Base(std::forward<Args>(args)...){
  for(unsigned i=0;i<5120;i++){
   savedRaw[i]=this->gddr.at(0x18000/4+i)=Bits(float(int(i%127)-63)/128.f);
   savedInput[i]=this->gddr.at(i);
   this->memory[0x94000+i*4]=this->memory[0x99000+i*4]=0x7fc00001;
  }
  for(unsigned a:{0x93ffcU,0x9e000U})this->memory[a]=0xdecafbad;
 }
 bool ResidualCommand()const{
  bool write=this->command[7]==0x212;unsigned local=write?this->command[0]:this->command[3];
  return (write?this->command[5]:this->command[2])==this->fixture.manifest[8] && this->Range(local,0x94000,40960);
 }
 uint32_t Load(uint32_t a)override{
  if(this->Range(a,0x94000,40960))return this->memory.at(a);
  return Base::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(this->Range(a,0x94000,40960)){
   assert(this->Range(a,0x99000,20480) && !this->pending && !this->unknown && completed==4 && issued==4);
   assert(PioneerQ38ProjectionGraphSnapshotContains(0,a));sumStores++;this->memory[a]=value;return;
  }
  if(a==0xffb20040 && ResidualCommand()){
   assert(issued==completed && issued<6);unsigned index=issued++;
   bool write=index>=4;unsigned offset=(index&1)*16384,bytes=(index&1)?4096:16384;
   unsigned local=(index<2?0x94000:0x99000)+offset;
   unsigned remote=this->fixture.manifest[4]+(index<2?0x18000:index<4?274880:0)+offset;
   const uint32_t expect[]={write?local:remote,0,write?129U:this->fixture.manifest[8],write?remote:local,0,
    write?this->fixture.manifest[8]:129U,0,write?0x212U:0,bytes,0};
   for(unsigned i=0;i<10;i++)assert(this->command[i]==expect[i]);
   if(index<2)assert(!this->stages[1] && !this->stages[3] && !this->stages[4]);
   else{
    unsigned jobs=0;for(unsigned role=0;role<3;role++)jobs+=this->fixture.manifest[PioneerQ38FfnGraphRoleOffset(role)+2];
    assert(this->stages[1]==jobs && this->stages[3]==68 && this->stages[4]==20);
   }
   if(write)assert(sumStores==5120 && completed>=4);
   if(issued==drop){this->faultAt=this->packets+1;lastFaultPacket=this->packets+1;}
  }
  Base::Store(a,value);
 }
 void Packet()override{
  bool residual=ResidualCommand(),write=this->command[7]==0x212;unsigned local=write?this->command[0]:this->command[3];
  Base::Packet();
  if(!residual)return;
  assert(!this->pending && issued==completed+1);completed++;writes+=unsigned(write);
  if(issued==mutateSource){this->rootSource[11]^=1;lastFaultPacket=this->packets;}
  if(issued==mutateManifest){this->memory[TT_Q38_FFN_GRAPH_MANIFEST+40*4]^=1;lastFaultPacket=this->packets;}
  if(issued==badCounterAt){this->counts[write?7:3]++;lastFaultPacket=this->packets;}
  if(issued==mutateDown){assert(issued==3 || issued==4);this->memory[local+this->command[8]-4]=downValue;
   // The program reads BOTH complete down chunks before checking math.
   // This is a numeric fixture fault,not a detected transport mismatch.
   lastFaultPacket=this->packets+unsigned(issued==3);
  }
 }
 void CaptureProduced(){
  for(unsigned i=0;i<5120;i++){savedRaw[i]=this->gddr.at(0x18000/4+i);savedInput[i]=this->gddr.at(i);}
 }
 void VerifyResidual(){
  assert(issued==6 && completed==6 && writes==2 && sumStores==5120);
  unsigned overwritten=0,changed=0;
  for(unsigned i=0;i<5120;i++){
   float sum=Float(savedRaw[i])+Float(this->gddr.at(274880/4+i));
   assert(this->gddr.at(i)==Bits(sum) && this->memory.at(0x94000+i*4)==savedRaw[i]
    && this->memory.at(0x99000+i*4)==Bits(sum));
   overwritten+=unsigned(this->gddr.at(0x18000/4+i)!=savedRaw[i]);changed+=unsigned(this->gddr.at(i)!=savedInput[i]);
  }
  assert(overwritten && changed); // prove the OLD residual GDDR really was clobbered
  const uint32_t receipt[]={0x33524651,20480,4,2,this->fixture.manifest[13],this->fixture.manifest[40],this->fixture.manifest[41]};
  for(unsigned i=0;i<7;i++)assert(this->memory.at(TT_Q38_FFN_GRAPH_STATS+(57+i)*4)==receipt[i]);
  assert(this->memory.at(TT_Q38_FFN_GRAPH_STATS+4)==3 && this->memory.at(TT_Q38_FFN_GRAPH_STATS+49*4)==9);
  for(unsigned a:{0x93ffcU,0x9e000U})assert(this->memory.at(a)==0xdecafbad);
 }
 void Failed(){
  assert(!this->RunGraph() && this->unknown);
  if(lastFaultPacket)assert(this->packets==lastFaultPacket);
  this->Inert();
 }
};
#endif
