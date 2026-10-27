/* SPDX-License-Identifier: MIT */
// Actual QMP1 + SAME119 QMW1 workers. Byte endpoints only; no expected
// products/status/nonces/tensor state are installed by the fabric.
#define PIONEER_Q38_MIXED_DENSE_ROOT_MACHINE_ONLY
#include "pioneer_q38_mixed_dense_root_test.cpp"
#include "../PioneerQ38MixedProjectionRootLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DenseProjectionPlan.h"
#include "pioneer_q38_projection_input_reference.h"
#include "../PioneerQ38OriginalWeightBytes.h"
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
#include "../PioneerQ38MixedRecurrentProjectionPlan.h"
namespace Q=PioneerQ38MixedRecurrentProjectionRootLayout;
#else
namespace Q=PioneerQ38MixedProjectionRootLayout;
#endif
struct ProjectionRootMachine:MixedRootMachine {
 ProjectionExecutionLearnedBytes* originalLearned=nullptr; // TEST raw source only; never numeric products
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical;
 std::array<uint32_t,64> descriptor={};
 std::array<std::array<uint32_t,8>,119> parts={};std::array<uint32_t,119> rows={};
 std::vector<uint32_t> scattered;unsigned active=0,scatterPackets=0,quantWrites=0,completedGathers=0;
 unsigned badProofWord=~0U,inputFaultPacket=0,nativeFaultPacket=0,scaleFaultPacket=0,recipeFaultPacket=0;
 ProjectionRootMachine(const char* image,const char* worker,const char* qdp,const char* qda,unsigned romLimit=32764,bool bootstrapHistories=true)
  :MixedRootMachine(image,worker,qdp,qda,romLimit,bootstrapHistories),canonical(new PioneerQ38CanonicalModelPlan){
  assert(canonical->Build());boot[0]=Q::Magic;memory[Q::Boot]=Q::Magic;
  for(auto r:std::array<std::pair<unsigned,unsigned>,10>{{{Q::Parts,119*32},{Q::Rows,119*4},{Q::Banks,32},{Q::MatrixHeader,64},
   {Q::RecipeShadow,256},{Q::HeaderShadow,64},{Q::BankShadow,32},{Q::RowsShadow,119*4},{Q::PartsShadow,119*32},{Q::QuantizedChunk,576}}})
   for(unsigned a=r.first;a<r.first+r.second;a+=4)memory[a]=0x7fc00001;
  for(unsigned a=Q::Native;a<Q::Native+12288+224;a+=4)memory[a]=0x7fc00001;
  for(unsigned a=Q::Scales;a<Q::Scales+1024;a+=4)memory[a]=0x7fc00001;
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
  for(auto r:std::array<std::pair<unsigned,unsigned>,6>{{{Q::InputShadow,17408*4},{Q::NativeShadow,17408*2},
   {Q::ScaleShadow,768},{Q::Native,17408*2},{Q::Scales,768},{Q::Projected,17408*4}}})
   for(unsigned a=r.first;a<r.first+r.second;a+=4)memory[a]=0x7fc00001;
#endif
 }
 unsigned Index(uint32_t a)override{if(a==0x6000)return 0;assert(Range(a,Q::Code,unsigned(code.size())) && !(a&3));return 1+(a-Q::Code)/4;}
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,Q::Code,unsigned(code.size()));}
 uint32_t Load(uint32_t a)override{
  if(Range(a,Q::Code,unsigned(code.size())))return l1[Index(a)];
  return MixedRootMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  bool extra=Range(a,Q::RecipeShadow,256) || Range(a,Q::HeaderShadow,64) || Range(a,Q::BankShadow,32) || Range(a,Q::RowsShadow,119*4)
   || Range(a,Q::PartsShadow,119*32) || Range(a,Q::QuantizedChunk,576) || Range(a,Q::Native,12288+224) || Range(a,Q::Scales,1024)
   || Range(a,Q::Projected,49152);
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
  extra |= Range(a,Q::InputShadow,17408*4) || Range(a,Q::NativeShadow,17408*2) || Range(a,Q::ScaleShadow,768)
   || Range(a,Q::Native,17408*2) || Range(a,Q::Scales,768) || Range(a,Q::Projected,17408*4);
#endif
  if(extra){assert(!pending);memory[a]=value;if(Range(a,Q::Native,12288))quantWrites++;return;}
  MixedRootMachine::Store(a,value);
 }
 void Geometry(unsigned w){
  auto& p=*child[w];assert(p.pc==p.waitPC || p.pc==p.waitPC+4);p.ClearTensorState();
  uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=p.Count(i);
  unsigned type=p.memory.at(M::Job+17*4),k=p.memory.at(M::Job+21*4),g=p.memory.at(M::Job+23*4),base=p.memory.at(M::Job+22*4),route=p.memory.at(M::Job+24*4);
  p.denseActive=p.gdnActive=p.convActive=p.outputActive=p.activation=false;
  for(unsigned i=0;i<8;i++)p.countBase[i]=physical[i];p.requests=p.responses=p.matrixInstructions=0;p.doorbellRead=false;
  // Observer geometry/raw learned endpoints follow the ACTUAL received
  // job. PC/register/config/thread/LRegs/physicalDst are NOT replaced.
  p.type=type;p.inputs=k;p.count=type==8?32:256;p.FfnPersistentPeer::groups=g;p.base=base;
  p.tile.assign(PioneerQ38PanelTileBytes(type),0);p.tiles.assign(g*(k/p.count),p.tile);p.vector.resize(k);p.scales.resize(k/p.count);
  p.packets.clear();p.root.clear();unsigned scaleBytes=(unsigned(p.scales.size()*4)+63)&~63U;
  for(unsigned at=0;at<k*2;at+=16384)p.packets.push_back({Q::Native+at,129,Q::Native+at,std::min(16384U,k*2-at),-1});
  p.packets.push_back({Q::Scales,129,0x94000,scaleBytes,-1});p.inputReads=unsigned(p.packets.size());
  for(unsigned i=0;i<k/2;i++)p.root[Q::Native+i*4]=memory.at(Q::Native+i*4);
  for(unsigned i=0;i<scaleBytes/4;i++)p.root[Q::Scales+i*4]=memory.at(Q::Scales+i*4);
  for(unsigned i=0;i<k;i++)p.vector[i]=uint16_t(p.root.at(Q::Native+i/2*4)>>((i%2)*16));
  for(unsigned i=0;i<p.scales.size();i++)p.scales[i]=p.root.at(Q::Scales+i*4);
  for(unsigned t=0;t<p.tiles.size();t++){
   if(originalLearned){
    assert(originalLearned->Ready() && originalLearned->Read(route,base+t*unsigned(p.tile.size()),p.tiles[t].data(),unsigned(p.tile.size()))
     && originalLearned->Ready());
   }else for(unsigned row=0;row<16;row++){
    unsigned global=rows[w]+t/(k/p.count)*16+row;std::vector<uint8_t> raw(PioneerQ38BlockBytes(type),0);uint16_t half=uint16_t(0x0800+(global%4)*0x80);
    if(type==8){raw[0]=uint8_t(half);raw[1]=uint8_t(half>>8);for(unsigned i=0;i<32;i++)raw[2+i]=uint8_t(int((t*17+global*19+i*37)%255)-127);}
    else if(type==12){
     raw[0]=raw[2]=uint8_t(half);raw[1]=raw[3]=uint8_t(half>>8);
     for(unsigned i=4;i<16;i++)raw[i]=uint8_t(1+(t+global+i)%3);
     for(unsigned i=16;i<144;i++)raw[i]=uint8_t((t*17+global*19+i*37)%256);
    }else{assert(type==14);for(unsigned i=0;i<128;i++)raw[i]=uint8_t((t*17+global*19+i*37)%256);
     for(unsigned i=128;i<192;i++)raw[i]=global&1?0x55:0xaa;for(unsigned i=192;i<208;i++)raw[i]=uint8_t(global&2?-int(1+(t+i)%3):int(1+(t+i)%3));
     raw[208]=uint8_t(half);raw[209]=uint8_t(half>>8);
    }
    for(unsigned i=0;i<raw.size();i++)p.tiles[t][i*16+row]=raw[i];
   }
   p.packets.push_back({base+t*unsigned(p.tile.size()),route,0x90000,unsigned(p.tile.size()),int(t)});
  }
  p.Poison();p.PoisonTail();for(unsigned i=0;i<4096;i++)p.buffers[4096+i]=0x7fc00000;
 }
 void Submit(){
  bool write=command[7]==0x212;assert(write || !command[7]);unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  if(write && command[5]==descriptor[8]){
   assert(command[2]==129 && !command[1] && !command[4] && !command[6] && !command[9] && bytes && bytes<=16384 && !(bytes&63)
    && local==Q::Scratch && remote==descriptor[4]+descriptor[10]+scatterPackets*16384);
   packets++;if(packets==failPacket){pending=true;return;}
   unsigned first=(remote-descriptor[4]-descriptor[10])/4;for(unsigned i=0;i<bytes/4;i++)scattered.at(first+i)=memory.at(local+i*4);scatterPackets++;
   counts[4]++;counts[5]+=bytes/64;counts[7]++;counts[2]++;counts[6]++;
   if(packets==counterFaultPacket)counts[3]++;
   if(packets==sourceFaultPacket)source[11]^=1;
   if(packets==headerFaultPacket)memory[Q::Header+48*4]^=1;
   if(packets==bootFaultPacket)memory[Q::Boot+8*4]^=1;
   return;
  }
  unsigned w=Worker(write?command[5]:command[2]);auto& p=*child[w];Advance(w);
  assert(local==Q::Scratch+(remote&31) && bytes && local+bytes<=Q::Scratch+16384 && !command[1] && !command[4] && !command[6] && !command[9]
   && (write?command[2]:command[5])==129);
  packets++;if(packets==failPacket){pending=true;return;}
  if(write){
   assert(w<active && ((remote==M::Job && bytes==256) || (remote==M::Doorbell && bytes==4)));
   for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
   if(remote==M::Job)Geometry(w);else doorbells++;
   writes++;
  }else{
   reads++;
   if(remote==0xffb121b0){assert(bytes==4);memory[local]=badWorker==w+1 && badKind==1?0x46801:0x46800;resetRead[w]=true;}
   else if(remote==0xffb13140){assert(bytes==4 && resetRead[w]);
    if(badWorker==w+1 && badKind==1)heldPCReads++;
    memory[local]=badWorker==w+1 && badKind==2?0x20060:p.pc;resetRead[w]=false;}
   else if(Range(remote,0xffb20000,0x2000)){
    unsigned o=remote-0xffb20000;bool found=false;
    for(unsigned i=0;i<16;i++)if(o==DenseAttentionMachine::SourceOffset(i)){memory[local]=p.source[i];found=true;}
    for(unsigned i=0;i<8;i++)if(o==FfnPersistentPeer::CounterOffset(i)){memory[local]=p.Count(i);found=true;break;}assert(found);
    if(badWorker==w+1 && badKind==3 && o==DenseAttentionMachine::SourceOffset(11))memory[local]^=1;
   }else if(remote==0x83f00){
    assert(bytes>=256 && bytes<=1280);for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
    for(unsigned i=64;i<bytes/4;i++)memory[local+i*4]=p.buffers[4096+i-64];
    if(bytes>256 && badProofWord<64)memory[local+badProofWord*4]^=1;
    if(bytes>256)completedGathers++;
   }else{
    assert((remote==M::Boot && bytes==256) || (remote==M::Job && bytes==256) || (remote==M::Status && bytes==64)
     || (remote==M::Doorbell && bytes==4) || (remote==W::Summary && bytes==256));
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=p.memory.at(remote+i*4);
    if(badWorker==w+1){
     if(badKind==4 && remote==M::Status)memory[local+15*4]^=1;
     if(badKind==5 && remote==M::Boot)memory[local+8*4]^=1;
     if(badKind==6 && remote==M::Job)memory[local+4]^=1;
    }
   }
  }
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)>>6;counts[write?7:3]++;counts[2]++;counts[6]++;
  if(packets==counterFaultPacket)counts[3]++;
  if(packets==sourceFaultPacket)source[11]^=1;
  if(packets==headerFaultPacket)memory[Q::Header+48*4]^=1;
  if(packets==bootFaultPacket)memory[Q::Boot+8*4]^=1;
  if(packets==inputFaultPacket)memory[descriptor[53]]^=1;
  if(packets==nativeFaultPacket)memory[Q::Native]^=1;
  if(packets==scaleFaultPacket)memory[Q::Scales]^=1;
  if(packets==recipeFaultPacket)memory[Q::Parts+118*32+5*4]^=1;
 }
 bool WaitProjection(unsigned n){
  while(pc!=0x5809c && !(pc==0x5804c && memory.at(Q::Status+4)==n))StepScalar();
  if(pc==0x5809c){assert(memory.at(Q::Status+8)==Q::Fault);return false;}
  assert(memory.at(Q::Status+12)==Q::Magic && !memory.at(Q::Status+8) && !memory.at(Q::Status+16));return true;
 }
 bool AtSubmitRead(unsigned remote,unsigned bytes) {
  // Stop BEFORE an actually reached store-to-submit instruction. Only
  // observe the emitted instruction/registers; never change PC/registers.
  uint32_t ins=Load(pc);if((ins&0x707f)!=0x2023)return false;
  unsigned rs1=(ins>>15)&31;int32_t imm=Sign(((ins>>25)<<5)|((ins>>7)&31),12);
  return regs[rs1]+imm==0xffb20040 && !command[7] && command[0]==remote && command[8]==bytes;
 }
 void BeforeProof(){
  while(!AtSubmitRead(0x83f00,320)){assert(pc!=0x5809c);StepScalar();}
  assert(!completedGathers && doorbells==active && !pending && !scatterPackets);
 }
 void BeforeLastCounter(){
  while(!(completedGathers==active && command[2]==M::Peer(119) && AtSubmitRead(0xffb20000+FfnPersistentPeer::CounterOffset(7),4))){assert(pc!=0x5809c);StepScalar();}
  assert(!pending && !scatterPackets);
 }
 void BeforeScatter(){
  for(;;){
   assert(pc!=0x5809c);uint32_t ins=Load(pc);unsigned rs1=(ins>>15)&31;
   int32_t imm=Sign(((ins>>25)<<5)|((ins>>7)&31),12);
   if((ins&0x707f)==0x2023 && regs[rs1]+imm==0xffb20040 && command[7]==0x212 && command[5]==descriptor[8])break;
   StepScalar();
  }
  assert(completedGathers==active && !pending && !scatterPackets);
 }
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
 void StageProjection(unsigned role,unsigned changed,unsigned layer=0){
#else
 void StageProjection(unsigned role,unsigned changed,unsigned layer=3){
#endif
  sequence++;doorbells=scatterPackets=quantWrites=completedGathers=0;
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
  auto plan=std::make_unique<PioneerQ38MixedRecurrentProjectionPlan>();uint32_t sources[1920];
#else
  auto plan=std::make_unique<PioneerQ38DenseProjectionPlan>();uint32_t sources[1920];
#endif
  for(unsigned i=0;i<16;i++)sources[i]=source[i];for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)sources[(w+1)*16+i]=child[w]->source[i];
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
  assert(plan->Build(*canonical,layer,role,changed?8191:0,changed?248319:123,layer%7,1000+sequence,sequence,PioneerQ38GdnIdentity{1,2,3,4},sources));
  for(unsigned i=0;i<64;i++)descriptor[i]=plan->Frame()[i];
#else
  assert(plan->Build(*canonical,layer,changed?1:0,123,layer%7,1000+sequence,PioneerQ38GdnIdentity{1,2,3,4},sources));
  unsigned slot=role==10?3:role-5;for(unsigned i=0;i<64;i++)descriptor[i]=plan->Frame(slot)[i];
  descriptor[0]=Q::Magic;descriptor[13]=Q::Rows;descriptor[48]=sequence;descriptor[49]=119;descriptor[50]=Q::Parts;descriptor[51]=Q::MatrixHeader;
  descriptor[52]=Q::Banks;descriptor[53]=role==10?Q::Output:Q::Input;descriptor[54]=Q::Projected;descriptor[55]=Q::PeerJobs;descriptor[56]=Q::Nonces;descriptor[57]=Q::PeerBoots;
#endif
  assert(Q::Shape(descriptor.data()));active=descriptor[7];scattered.assign(descriptor[11],0x7fc00001);
  for(unsigned i=0;i<64;i++)memory[Q::Header+i*4]=descriptor[i];
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
  for(unsigned i=0;i<16;i++)memory[Q::MatrixHeader+i*4]=plan->Header()[i];for(unsigned i=0;i<8;i++)memory[Q::Banks+i*4]=plan->Banks()[i];
#else
  for(unsigned i=0;i<16;i++)memory[Q::MatrixHeader+i*4]=plan->Header(slot)[i];for(unsigned i=0;i<8;i++)memory[Q::Banks+i*4]=plan->Banks()[i];
#endif
  for(unsigned w=0;w<119;w++){
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
   firstNonce[w]=child[w]->memory.at(M::Status+4);rows[w]=plan->Rows()[w];memory[Q::Rows+w*4]=rows[w];
   for(unsigned i=0;i<8;i++)memory[Q::Parts+w*32+i*4]=parts[w][i]=plan->Parts()[w][i];
#else
   firstNonce[w]=child[w]->memory.at(M::Status+4);rows[w]=plan->Rows(slot)[w];memory[Q::Rows+w*4]=rows[w];
   for(unsigned i=0;i<8;i++)memory[Q::Parts+w*32+i*4]=parts[w][i]=plan->Parts(slot)[w][i];
#endif
  }
  for(unsigned i=0;i<descriptor[14];i++)memory[descriptor[53]+i*4]=Bits(float(int((i*37+changed*41)%257)-128)*.03125f);
  for(unsigned i=0;i<descriptor[11];i++)memory[Q::Projected+i*4]=0x7fc00001;
  memory[Q::Doorbell]=sequence;
 }
 void VerifyProjection(){
  assert(doorbells==active && scatterPackets==(descriptor[12]+16383)/16384 && memory.at(Q::Summary)==Q::Magic);
  for(unsigned w=0;w<119;w++){
   auto& p=*child[w];assert((p.pc==p.waitPC || p.pc==p.waitPC+4) && p.memory.at(M::Status+4)==firstNonce[w]+unsigned(w<active) && !p.memory.at(M::Status+8));
   if(w>=active)continue;assert(p.requests==p.packets.size() && !p.dotProbeReads && p.CorrectRows());
   for(unsigned i=0;i<p.FfnPersistentPeer::groups*16;i++)assert(memory.at(Q::Projected+(rows[w]+i)*4)==p.buffers[4096+i] && scattered[rows[w]+i]==p.buffers[4096+i]);
  }
  for(unsigned at=0;at<descriptor[14];at+=256){
   float raw[256];PioneerQ38ProjectionInput expected;for(unsigned i=0;i<256;i++)raw[i]=Float(memory.at(descriptor[53]+(at+i)*4));ProjectionInputReference(descriptor[15],raw,expected);
   for(unsigned i=0;i<128;i++)assert(memory.at(Q::Native+(at/2+i)*4)==expected.operand[i]);
   unsigned n=descriptor[15]==8?8:1,first=at>>(descriptor[15]==8?5:8);
   for(unsigned i=0;i<n;i++)assert(memory.at(Q::Scales+(first+i)*4)==expected.scale[i]);
  }
 }
};
static unsigned ProjectionRefusals(ProjectionRootMachine& base,bool full){
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
 constexpr unsigned RefusalRole=10;
#else
 constexpr unsigned RefusalRole=6;
#endif
 unsigned refused=0;
 auto branch=[&](unsigned kind,unsigned arg,bool staged=false){
  // OS snapshot of an ALREADY EXECUTED world, never an emulated reset,
  // changed private nonce, tensor-state substitution or expected product.
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   auto& m=base;if(!staged)m.StageProjection(RefusalRole,0);
   if(kind==0)m.failPacket=1;
   if(kind==1)m.counterFaultPacket=1;
   if(kind==2)m.sourceFaultPacket=1;
   if(kind==3)m.headerFaultPacket=1;
   if(kind==4)m.bootFaultPacket=1;
   if(kind==5)m.recipeFaultPacket=1;
   if(kind==6)m.badReadback=arg;
   if(kind==7){m.badWorker=1;m.badKind=arg;}
   if(kind==8){m.badWorker=119;m.badKind=arg;}
   if(kind==9)m.memory[Q::Header+arg*4]^=0x80000000U;
   if(kind==10)m.child[118]->countBase[arg]++;
   if(kind==11)m.memory[Q::Parts+arg*32+3*4]=1; // caller never supplies runtime nonce
   if(kind==12)m.memory[m.descriptor[53]]=0x7fc00000;
   if(kind==13)m.memory[m.descriptor[53]]^=1;
   if(kind==14)m.memory[Q::Native]^=1;
   if(kind==15)m.memory[Q::Scales]^=1;
   if(kind==16)m.badProofWord=arg;
   if(kind==17)m.failPacket=m.packets+1;
   if(kind==18)m.counterFaultPacket=m.packets+1;
   if(kind==19)m.badReadback=m.readbacks+arg;
   if(kind==20)m.sourceFaultPacket=m.packets+1;
   if(kind==21)m.headerFaultPacket=m.packets+1;
   if(kind==22)m.bootFaultPacket=m.packets+1;
   bool okay=m.WaitProjection(m.sequence);
   if(okay || m.memory.at(Q::Summary)==Q::Magic || m.heldPCReads)fprintf(stderr,"PROJECTION_ROOT_FAULT kind%u arg%u okay%u packets%u\n",kind,arg,unsigned(okay),m.packets);
   assert(!okay && m.memory.at(Q::Summary)!=Q::Magic && !m.heldPCReads);
   if(kind<17 || kind==17)assert(!m.scatterPackets);
   else assert(m.scatterPackets==1); // provisional GDDR data, never a published producer
   if(kind<=4)assert(m.packets==1);
   if(kind==5)assert(!m.writes && !m.doorbells); // future peer seal, before any dispatch
   if(kind==6 || kind==9 || kind==11 || kind==12)assert(!m.packets);
   unsigned packets=m.packets;for(unsigned i=0;i<32;i++)m.StepScalar();assert(m.packets==packets);_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));refused++;
 };
 for(unsigned kind=0;kind<=5;kind++)branch(kind,0);
 for(unsigned word=1;word<=10;word++)branch(6,word);
 for(unsigned kind=1;kind<=6;kind++){branch(7,kind);branch(8,kind);}
 for(unsigned word:{0U,2U,5U,9U,13U,14U,15U,40U,42U,43U,48U,49U,50U,51U,52U,53U,54U,55U,56U,57U,63U})branch(9,word);
 for(unsigned i=0;i<8;i++)if(i!=6)branch(10,i);
 for(unsigned w:{0U,63U,118U})branch(11,w);
 branch(12,0);
 base.StageProjection(RefusalRole,0);base.BeforeProof();
 // Reuse ONLY the OS snapshot of this actual continuation, AFTER real
 // quantization, admission, job dispatch and worker arithmetic. Each child
 // executes the still-pending actual submit; no products are cached/seeded.
 for(unsigned word=0;word<64;word++)if(full || word%8==0 || word==17 || word==20 || word==29 || word==63)branch(16,word,true);
 base.BeforeLastCounter();for(unsigned kind=13;kind<=15;kind++)branch(kind,0,true);
 base.BeforeScatter();
 branch(17,0,true);branch(18,0,true);for(unsigned word=1;word<=10;word++)branch(19,word,true);
 for(unsigned kind=20;kind<=22;kind++)branch(kind,0,true);
 assert(base.WaitProjection(base.sequence));base.VerifyProjection();
 printf("PASS ACTUAL QMP1 refusals%u/no current success/fault park inert/ALL119 source+receipt admission/held reset rejected BEFORE PC/failed GDDR writes stay PROVISIONAL\n",refused);
 return refused;
}
#ifndef PIONEER_Q38_MIXED_PROJECTION_ROOT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==5 || (argc==6 && !strcmp(argv[5],"--full")));(void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;setvbuf(stdout,nullptr,_IOLBF,0);
 printf("START actual QMP1/119 QMW1 boot ONCE and diverse actual matrix+GDN histories;NOT silicon timing\n");
 auto m=std::make_unique<ProjectionRootMachine>(argv[1],argv[2],argv[3],argv[4]);assert(m->WaitProjection(0));unsigned matrices=0;
 unsigned refusals=ProjectionRefusals(*m,argc==6);
 for(unsigned changed=0;changed<2;changed++)for(unsigned role:{5U,6U,7U,10U}){
  m->StageProjection(role,changed);assert(m->WaitProjection(m->sequence));m->VerifyProjection();matrices++;
  printf("PASS ACTUAL QMP1 canonical role%u rows%u changed%u ROOT quantize/dynamic119nonces/matrix/gather/GDDRscatter SAME119 lifetime;NOT native timing\n",role,m->descriptor[11],changed);
 }
 printf("PASS new mixed projections matrices%u/refusals%u/119bootsONCE/noSTOP/noReset/card-native input bytes+independent matrix output/ROOTstack%u;SYNTHETIC weights/byte endpoints ONLY\n",matrices,refusals,0x70000-m->minStack);
}
#endif
