/* SPDX-License-Identifier: MIT */
// Actual QBR1 + SAME119 QMW1 programs. Byte fabric only; CPU observers
// never provide controller inputs, numeric products, status or nonces.
#define PIONEER_Q38_MIXED_JOINED_ROOT_MACHINE_ONLY
#include "pioneer_q38_mixed_joined_root_test.cpp"
#include "../PioneerQ38MixedBlockRootLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BlockFfnRecipe.h"
namespace B=PioneerQ38MixedBlockRootLayout;
struct BlockRootMachine:JoinedRootMachine {
 std::array<uint32_t,128> blockOuter={};
 unsigned initialNorm=0,postNorm=0,activation=0,finalWords=0,quantized=0,lastInputPhase=0;
 double activationError=0;
 BlockRootMachine(const char* root,const char* worker,const char* qdp,const char* qda):JoinedRootMachine(root,worker,qdp,qda){
  boot[0]=B::Magic;memory[B::Boot]=B::Magic;
  for(auto span:std::array<std::pair<unsigned,unsigned>,15>{{{B::Outer,512},{B::OriginalCopy,20480},{B::Raw,20480},{B::RawCopy,20480},
   {B::Weight,20480},{B::InputShadow,17408*4},{B::Native,17408*2},{B::NativeShadow,17408*2},{B::ScaleShadow,640},
   {B::PostResidual,20480},{B::PostNormalized,20480},{B::Final,20480},{B::Input,17408*4},{B::Projected,17408*4},{0x92000,2048}}})
   for(unsigned a=span.first;a<span.first+span.second;a+=4)memory[a]=0x7fc00001;
 }
 unsigned Phase()const{return memory.at(B::Status+20);}
 void Store(uint32_t a,uint32_t value)override{
  if(a==0xffb20040){assert(value==1 && !pending);SubmitBlock();return;}
  bool extra=Range(a,B::OriginalCopy,20480) || Range(a,B::Raw,20480) || Range(a,B::RawCopy,20480) || Range(a,B::Weight,20480)
   || Range(a,B::InputShadow,17408*4) || Range(a,B::Native,17408*2) || Range(a,B::NativeShadow,17408*2) || Range(a,B::ScaleShadow,640)
   || Range(a,B::PostResidual,20480) || Range(a,B::PostNormalized,20480) || Range(a,B::Final,20480)
   || Range(a,B::Input,17408*4) || Range(a,B::Projected,17408*4) || Range(a,0x92000,2048);
  if(extra){assert(!pending);memory[a]=value;return;}
  JoinedRootMachine::Store(a,value);
 }
 void RmsObserver(unsigned input,unsigned output,unsigned role){
  double sum=0;for(unsigned i=0;i<5120;i++){float x=Float(memory.at(input+i*4));sum+=double(x*x);}
  float mean=float(sum/5120.0),scale=1.f/sqrtf(mean+1e-6f);
  for(unsigned i=0;i<5120;i++){
   float x=Float(memory.at(input+i*4)),weight=Float(WordAt(PioneerQ38ProjectionRoute(blockOuter[64+role*2]),blockOuter[65+role*2]+i*4));
   assert(memory.at(output+i*4)==Bits((x*scale)*weight));
  }
 }
 void ObserveInitial(){
  assert(!initialNorm && !projections);RmsObserver(B::Original,B::Raw,0);
  for(unsigned i=0;i<5120;i++)assert(memory.at(B::Original+i*4)==memory.at(B::OriginalCopy+i*4)
   && memory.at(B::Raw+i*4)==memory.at(B::RawCopy+i*4) && memory.at(B::Input+i*4)==memory.at(B::Raw+i*4));
  initialNorm++;
  puts("OBSERVED ACTUAL QBR1 initial RMS ALL5120 BIT-EXACT;original residual input preserved separately");
 }
 void ObservePost(){
  assert(!postNorm && projections==4);
  for(unsigned i=0;i<5120;i++){
   float x=Float(memory.at(B::Original+i*4)),projected=Float(WordAt(outer[8],outer[9]+TT_Q38_DENSE_O+i*4));
   assert(memory.at(B::PostResidual+i*4)==Bits(x+projected));
  }
  RmsObserver(B::PostResidual,B::PostNormalized,1);postNorm++;
  puts("OBSERVED ACTUAL QBR1 O+ORIGINAL residual and post-RMS ALL5120 BIT-EXACT;not normalized-input residual");
 }
 void ObserveActivation(){
  assert(!activation && projections==6);double delta=0,norm=0;
  for(unsigned i=0;i<17408;i++){
   float gate=Float(WordAt(outer[8],outer[9]+B::GateOffset+i*4)),up=Float(WordAt(outer[8],outer[9]+B::UpOffset+i*4));
   double e=exp(-fabs(double(gate))),sigmoid=gate>=0?1/(1+e):e/(1+e);
   float wanted=(gate*float(sigmoid))*up,actual=Float(memory.at(B::Input+i*4));
   assert(isfinite(actual));delta+=(double(actual)-wanted)*(double(actual)-wanted);norm+=double(wanted)*wanted;
  }
  double error=sqrt(delta/(norm?norm:1));assert(error<4e-6);activationError=std::max(activationError,error);activation++;
  printf("OBSERVED ACTUAL QBR1 SiLU(gate)*up ALL17408/L2%.12g -> actual down input;NO CPU products supplied\n",error);
 }
 void ObserveQuantized(){
  ReadDescriptor();unsigned phase=Phase();if(phase==lastInputPhase)return;lastInputPhase=phase;
  if(phase==1)ObserveInitial();if(phase==8)ObservePost();if(phase==11)ObserveActivation();
  for(unsigned at=0;at<descriptor[14];at+=256){
   float raw[256];PioneerQ38ProjectionInput expected;
   for(unsigned i=0;i<256;i++)raw[i]=Float(memory.at(descriptor[53]+(at+i)*4));ProjectionInputReference(descriptor[15],raw,expected);
   for(unsigned i=0;i<128;i++)assert(memory.at(B::Native+(at/2+i)*4)==expected.operand[i]);
   unsigned count=descriptor[15]==8?8:1,first=at>>(descriptor[15]==8?5:8);
   for(unsigned i=0;i<count;i++)assert(memory.at(B::Scales+(first+i)*4)==expected.scale[i]);
  }
  quantized++;
 }
 void ObserveProjections()override{
  JoinedRootMachine::ObserveProjections();
  if(descriptor[43]>=2 && descriptor[43]<=4)assert(descriptor[15]==12 && active==119);
 }
 void SubmitBlock(){
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2],phase=Phase();
  bool bank=false;for(unsigned b=0;b<7;b++)if(route==PioneerQ38ProjectionRoute(b))bank=true;
  if(bank && ((!write && (phase==1 || phase==7 || phase==10)) || (write && phase==12))){
   unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
   assert(bytes && bytes<=16384 && !(bytes&63) && !(remote&63) && local==B::Scratch && !command[1] && !command[4]
    && !command[6] && !command[9] && (write?command[2]:command[5])==129 && (write || !command[7]));
   packets++;if(packets==failPacket){pending=true;return;}
   if(write){
    assert(remote==outer[9]+B::FinalOffset+finalWords*4 && route==outer[8] && projections==7);
    for(unsigned i=0;i<bytes/4;i++){
     unsigned row=finalWords+i;float down=Float(WordAt(route,outer[9]+B::DownOffset+row*4)),residual=Float(memory.at(B::PostResidual+row*4));
     assert(memory.at(local+i*4)==Bits(down+residual));dram[Key(route,remote+i*4)]=memory.at(local+i*4);
    }
    finalWords+=bytes/4;
   }else for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=WordAt(route,remote+i*4);
   counts[write?4:0]++;counts[write?5:1]+=bytes/64;counts[write?7:3]++;counts[2]++;counts[6]++;
   if(packets==counterFaultPacket)counts[3]++;if(packets==sourceFaultPacket)source[11]^=1;
   if(packets==headerFaultPacket)memory[B::Header+48*4]^=1;if(packets==bootFaultPacket)memory[B::Boot+8*4]^=1;
   return;
  }
  if(!bank && write && command[3]==M::Job && memory.at(B::Header)==Q::Magic)ObserveQuantized();
  SubmitJoined();
 }
 void StageBlock(unsigned tokens,unsigned trial,unsigned layer=3){
  StageJoined(tokens,trial,layer);initialNorm=postNorm=activation=finalWords=quantized=lastInputPhase=0;
  blockOuter.fill(0);for(unsigned i=0;i<64;i++)blockOuter[i]=outer[i];
  blockOuter[0]=B::Magic;blockOuter[2]=128;blockOuter[47]=B::RawCopy;blockOuter[51]=B::Raw;
  blockOuter[55]=B::Original;blockOuter[56]=B::OriginalCopy;blockOuter[57]=B::FfnRecipes;blockOuter[58]=B::FfnCopies;blockOuter[59]=B::Final;
  for(unsigned role=0;role<2;role++)for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(p->kind==canonical->WeightKind && p->layer==layer && p->role==role){
    assert(p->rows==1 && p->rowBytes==20480);blockOuter[64+role*2]=p->bank;blockOuter[65+role*2]=canonical->Base+p->offset;
    for(unsigned row=0;row<5120;row++)dram[Key(PioneerQ38ProjectionRoute(p->bank),canonical->Base+p->offset+row*4)]
     =Bits(float(1+sin(row*.017+role*.37)*.1));
   }
  }
  blockOuter[68]=outer[9]+B::MetaOffset;blockOuter[69]=outer[8];blockOuter[70]=3;blockOuter[71]=B::RecipeStride;
  assert(B::OuterShape(blockOuter.data()));for(unsigned i=0;i<128;i++)memory[B::Outer+i*4]=blockOuter[i];
  for(unsigned slot=0;slot<3;slot++){
   PioneerQ38ProjectionJobs jobs;assert(jobs.Build(*canonical,layer,slot+2,tokens-1) && jobs.Count()==119 && jobs.Waves()==1 && jobs.Type()==12);
   std::array<uint32_t,1280> metadata={};auto* d=metadata.data();auto* h=d+64;auto* parts=reinterpret_cast<uint32_t(*)[8]>(d+80);auto* rows=d+1032;auto* banks=d+1151;
   for(unsigned i=0;i<64;i++)d[i]=memory.at(B::Recipes+i*4);
   d[0]=B::MatrixMagic;d[6]=outer[6]+5+slot;d[7]=119;d[10]=B::Offset(slot+2);d[11]=jobs.Outputs();d[12]=d[11]*4;
   d[14]=jobs.Inputs();d[15]=12;d[43]=slot+2;d[44]=119;
   const unsigned fields[]={B::MatrixMagic,1,1,12,jobs.Inputs(),119,d[6],TT_Q38_DENSE_MATRIX_INPUT,1};for(unsigned i=0;i<9;i++)h[i]=fields[i];
   for(unsigned b=0;b<7;b++)banks[b]=PioneerQ38ProjectionRoute(b);
   for(unsigned w=0;w<119;w++){
    const auto* p=jobs.Part(w);parts[w][0]=canonical->Base+p->offset;parts[w][1]=p->bank;parts[w][2]=p->groups;
    parts[w][4]=banks[p->bank];parts[w][5]=child[w]->source[0];rows[w]=p->firstRow;
   }
   assert(PioneerQ38BlockFfnRecipe(d,h,banks,parts,rows));
   for(unsigned i=0;i<1280;i++)dram[Key(outer[8],blockOuter[68]+slot*B::RecipeStride+i*4)]=metadata[i];
  }
 }
 bool WaitBlock(unsigned n,bool quiet=false){
  while(pc!=0x4009c && !(pc==0x4004c && memory.at(B::Status+4)==n))StepScalar();
  if(pc==0x4009c){assert(memory.at(B::Status+8)==B::Fault);if(!quiet)fprintf(stderr,"BLOCK_FAULT phase%u projections%u packets%u sp%x\n",Phase(),projections,packets,regs[2]);return false;}
  assert(memory.at(B::Status+12)==B::Magic && !memory.at(B::Status+8) && !memory.at(B::Status+16));return true;
 }
 void VerifyBlock(){
  assert(projections==7 && acceptedJobs==723 && prepared==1 && attentionChecks==1 && prepareReads==15 && memory.at(B::JointSummary)==B::Magic
   && memory.at(B::JointSummary+4)==sequence && scatterWords==59392 && initialNorm==1 && postNorm==1 && activation==1 && finalWords==5120 && quantized==7);
  for(unsigned i=0;i<5120;i++)assert(memory.at(B::Original+i*4)==memory.at(B::OriginalCopy+i*4));
  for(unsigned w=0;w<119;w++)assert(child[w]->memory.at(M::Status+4)==firstNonce[w]+(w<64?10:w<96?8:5));
 }
};
template<class Change> static void BlockRefusal(BlockRootMachine& base,unsigned& refused,Change change,bool noIO=false){
 fflush(nullptr);pid_t pid=fork();assert(pid>=0);
 if(!pid){
  auto& m=base;unsigned start=m.packets;change(m);bool okay=m.WaitBlock(m.sequence,true);
  if(okay)fprintf(stderr,"BLOCK_REFUSAL %u unexpectedly succeeded\n",refused);
  assert(!okay && m.memory.at(B::JointSummary)!=B::Magic);
  if(noIO)assert(m.packets==start && !m.projections);
  unsigned packets=m.packets,words=m.finalWords;for(unsigned i=0;i<32;i++)m.StepScalar();
  assert(m.packets==packets && m.finalWords==words);_exit(0);
 }
 int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));refused++;
}
static void BlockInitialRefusals(BlockRootMachine& m,unsigned& refused){
 for(unsigned word=0;word<128;word++)BlockRefusal(m,refused,[&](BlockRootMachine& b){
  b.memory[B::Outer+word*4]^=(word==13 || word==15 || word==65 || word==67)?1:0x80000000U;
 },true);
 for(unsigned row:{0U,5119U})BlockRefusal(m,refused,[&](BlockRootMachine& b){b.memory[B::Original+row*4]=0x7fc00000;},true);
 for(unsigned i=0;i<8;i++)if(i!=6)BlockRefusal(m,refused,[&](BlockRootMachine& b){b.counts[i]++;},true);
 while(!m.AtSubmitRead(m.blockOuter[65],16384)){assert(m.pc!=0x4009c);m.StepScalar();}
 for(unsigned which:{0U,1U})BlockRefusal(m,refused,[&](BlockRootMachine& b){
  b.WordAt(PioneerQ38ProjectionRoute(b.blockOuter[64]),b.blockOuter[65]+(which?5119:0)*4)=0x7fc00000;
 });
 for(unsigned word=1;word<=10;word++)BlockRefusal(m,refused,[&](BlockRootMachine& b){b.badReadback=b.readbacks+word;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.failPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.counterFaultPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.sourceFaultPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.headerFaultPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.bootFaultPacket=b.packets+1;});
 printf("PASS QBR1 initial-stage refusals%u/actual reached continuation/no completion/fault inert\n",refused);
}
static void BlockLaterRefusals(BlockRootMachine& m,unsigned& refused){
 while(!m.AtSubmitRead(m.blockOuter[67],16384)){assert(m.pc!=0x4009c);m.StepScalar();}
 for(unsigned row:{0U,5119U})BlockRefusal(m,refused,[&](BlockRootMachine& b){
  b.WordAt(PioneerQ38ProjectionRoute(b.blockOuter[66]),b.blockOuter[67]+row*4)=0x7fc00000;
 });
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.failPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.counterFaultPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.sourceFaultPacket=b.packets+1;});
 while(!m.AtSubmitRead(m.blockOuter[68],B::RecipeStride)){assert(m.pc!=0x4009c);m.StepScalar();}
 for(unsigned slot=0;slot<3;slot++)for(unsigned word:{0U,6U,14U,32U,43U,80U+3U,1032U})
  BlockRefusal(m,refused,[&](BlockRootMachine& b){b.WordAt(b.outer[8],b.blockOuter[68]+slot*B::RecipeStride+word*4)^=0x80000000U;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.memory[B::Original+5119*4]^=1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.memory[B::Raw+5119*4]^=1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.failPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.sourceFaultPacket=b.packets+1;});
 while(!m.AtSubmitRead(m.outer[9]+B::GateOffset,1024)){assert(m.pc!=0x4009c);m.StepScalar();}
 for(unsigned offset:{B::GateOffset,B::UpOffset})BlockRefusal(m,refused,[&](BlockRootMachine& b){b.WordAt(b.outer[8],b.outer[9]+offset)=0x7fc00000;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.failPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.counterFaultPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.sourceFaultPacket=b.packets+1;});
 while(m.Phase()!=12){assert(m.pc!=0x4009c);m.StepScalar();}
 for(unsigned address:{unsigned(B::Projected),unsigned(B::PostResidual)})BlockRefusal(m,refused,[&](BlockRootMachine& b){b.memory[address]=0x7fc00000;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.memory[B::Original+5119*4]^=1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.memory[B::FfnRecipes+3*B::RecipeStride-4]^=1;});
 for(;;){
  // Use the actual decoded store,not a fabricated PC/continuation.
  unsigned insn=m.Load(m.pc);if((insn&127)==0x23 && ((insn>>12)&7)==2){
   unsigned imm=unsigned(Sign(((insn>>25)<<5)|((insn>>7)&31),12));
   if(m.regs[(insn>>15)&31]+imm==0xffb20040)break;
  }
  assert(m.pc!=0x4009c);m.StepScalar();
 }
 assert(m.command[7]==0x212 && m.command[3]==m.outer[9]+B::FinalOffset && m.command[8]==16384);
 for(unsigned word=1;word<=10;word++)BlockRefusal(m,refused,[&](BlockRootMachine& b){b.badReadback=b.readbacks+word;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.failPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.counterFaultPacket=b.packets+1;});
 BlockRefusal(m,refused,[](BlockRootMachine& b){b.sourceFaultPacket=b.packets+1;});
 printf("PASS QBR1 all-stage refusals%u/late metadata+SiLU+final write faults/all outputs provisional/fault inert\n",refused);
}
#ifndef PIONEER_Q38_MIXED_BLOCK_ROOT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==5);(void)&JoinedRefusals;(void)&ProjectionRefusals;(void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START ACTUAL QBR1 complete dense block/SAME ROOT+119 workers;synthetic RAW endpoints;NOT native timing");
 auto m=std::make_unique<BlockRootMachine>(argv[1],argv[2],argv[3],argv[4]);assert(m->WaitBlock(0));
 unsigned refused=0;
 for(unsigned trial=0;trial<2;trial++){
  m->StageBlock(trial?65:33,trial,trial?7:3);
  if(!trial){BlockInitialRefusals(*m,refused);BlockLaterRefusals(*m,refused);}
  assert(m->WaitBlock(m->sequence));m->VerifyBlock();
  printf("PASS ACTUAL QBR1 frame%u N%u/initial+postRMS/7matrix+attention+SiLU/final residual ALL5120/SAME119 noSTOP or reboot/attentionL2%.12g activationL2%.12g stack%u\n",
   m->sequence,trial?65:33,m->maxError,m->activationError,0x70000-m->minStack);
 }
 printf("PASS complete dense block actual instructions/refusals%u;SYNTHETIC RAW learned/KV/table only;NO native/original full64/stateKVtokencommit/throughput grant\n",refused);
}
#endif
