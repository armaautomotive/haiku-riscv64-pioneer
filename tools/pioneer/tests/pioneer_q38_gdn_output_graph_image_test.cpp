/* SPDX-License-Identifier: MIT */
// ACTUAL QOP1 ROOT+ALL119 op6/matrix peers;transport/weights simulated.
// No native owner, learned model, hardware timing or throughput proof.
#define PIONEER_Q38_PROJECTION_COOPERATIVE_FIXTURE_ONLY
#include "pioneer_q38_projection_graph_cooperative_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputGraphShape.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputGraphProof.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
struct GdnGraphPeer:ParentArithmeticPeer {
 GdnGraphPeer(const char* image,unsigned worker,std::vector<uint32_t>& data):ParentArithmeticPeer(image,worker,data,false){}
 void Prepare(const std::map<uint32_t,uint32_t>& rootMemory){
  if(memory.at(0x74034)!=6){ParentArithmeticPeer::Prepare(rootMemory);return;}
  assert(pc==TT_Q38_PEER_WAIT_LOAD && !pending);ClearTensorState();uint32_t physical[8];
  for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  NextPackets();for(unsigned i=0;i<8;i++)countBase[i]=independentBefore[i]=physical[i];
  activation=projection=true;input=false;activationPackets=commandReads=activatedWrites=nativeWrites=scaleWrites=lineWrites=0;observed=false;
  for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4);
 }
 void VerifyGdn(){
  assert(activationPackets==3 && nativeWrites==128 && lineWrites==40 && !activatedWrites);
  float values[256];for(unsigned i=0;i<256;i++)values[i]=Float(gate[i]);PioneerQ38ProjectionInput reference;
  ProjectionInputReference(8,values,reference);
  for(unsigned i=0;i<128;i++)assert(native[i]==reference.operand[i]);
  for(unsigned i=0;i<8;i++)assert(line[i]==reference.scale[i]);
  assert(PioneerQ38GdnOutputInputLine(line.data(),descriptor.data()));
 }
};
struct GdnGraphMachine:ArithmeticParent {
 std::vector<uint32_t> manifest,original;
 unsigned sourceShadowWrites=0,gatherShadowWrites=0,token=0,sourceMutatePacket=0,sourceMutateWord=0;
 uint64_t rootStores=0;
 GdnGraphMachine(const char* root,const char* peer,unsigned bank=0):ArithmeticParent(root,peer,false){
  for(unsigned w=0;w<119;w++){
   actual[w]=std::make_unique<GdnGraphPeer>(peer,w+1,gddr);auto& p=*actual[w];
   for(unsigned i=0;i<16;i++)p.source[i]=i?0x12340000+w*0x100+i:0x16005+w*0x100;
   p.memory[0x7404c]=p.source[0];assert(p.BootNew());boots++;
  }
  RetargetGdn(bank,0,false);
 }
 void RetargetGdn(unsigned bank,unsigned selectedToken,bool stop){
  assert(!pending && !unknown);fixture.layer=0;fixture.role=13;fixture.bank=bank;fixture.stop=stop;fixture.token=0;
  fixture.phase=runs?memory.at(TT_Q38_PROJECTION_GRAPH_STATS+54*4):16;fixture.input=stop?0:1;
  fixture.jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(fixture.jobs->Build(*fixture.canonical,0,13,0));
  fixture.graph=std::make_unique<PioneerQ38ProjectionGraphPlan>();
  for(unsigned w=0;w<119;w++){
   auto& p=*actual[w];assert(p.pc==TT_Q38_PEER_WAIT_LOAD && !p.pending);fixture.sequence[w]=p.memory.at(0x74144);
   for(unsigned i=0;i<16;i++)fixture.sources[w][i]=p.source[i];
   node[w].sequence=node[w].requested=fixture.sequence[w];node[w].op=0;node[w].stopped=false;node[w].verified=true;
   for(unsigned i=0;i<8;i++)node[w].count[i]=p.Count(i);
  }
  assert(fixture.Build());kind=8;k=6144;token=selectedToken;
  const auto* old=fixture.graph->Manifest();manifest.assign(old,old+TT_Q38_PROJECTION_GRAPH_WORDS);
  manifest[0]=TT_Q38_GDN_OUTPUT_GRAPH_MARKER;manifest[40]=0;manifest[41]=fixture.phase;
  manifest[46]=stop?0:TT_Q38_GDN_RENDEZVOUS_MARKER;manifest[47]=stop?0:token;
  assert(PioneerQ38GdnOutputGraphManifest(manifest.data(),fixture.sources[0]) && !PioneerQ38ProjectionGraphFrame(manifest.data()));
  for(unsigned i=0;i<manifest.size();i++)memory[TT_Q38_PROJECTION_GRAPH_MANIFEST+i*4]=manifest[i];
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++){
   memory[TT_Q38_GDN_OUTPUT_GRAPH_SOURCES+w*64+i*4]=fixture.sources[w][i];
   memory[TT_Q38_GDN_OUTPUT_GRAPH_SOURCE_SHADOW+w*64+i*4]=0xdeadbeef;
  }
  outputCover.assign(5120,0);expected.assign(5120,0x7fc00001);
  for(unsigned i=0;i<6144;i++)gddr.at(TT_Q38_GDN_GATED_OUTPUT/4+i)=Bits(float(int((i*37+runs*71)%255)-127)*.125f);
  original=gddr;producerJobs=matrixJobs=stopJobs=lineReads=operandReads=scaleWrites=scaleReads=outputWrites=0;
  sourceShadowWrites=gatherShadowWrites=0;pc=0x6000;regs[0]=0;
 }
 void InputFinished(ParentArithmeticPeer& p)override{static_cast<GdnGraphPeer&>(p).VerifyGdn();}
 // Same transport path as the qualified cooperative fixture;only op6
 // descriptor selection is different. No result/proof/status fabricated.
 void PeerPacket()override{
  bool write=command[7]==0x212;unsigned w=Worker(write?command[5]:command[2])-1;
  if(write && command[3]==0x74100){
   auto& p=static_cast<GdnGraphPeer&>(*actual.at(w));assert(command[8]==4 && memory.at(command[0])==p.memory.at(0x74144)+1);
   p.Prepare(memory);BeforeDoorbell(w);p.memory[0x74100]=memory.at(command[0]);Retire(4,true);return;
  }
  ArithmeticParent::PeerPacket();
 }
 void DramGdn(){
  bool write=command[7]==0x212;unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert((write?command[2]:command[5])==129 && remote>=manifest[4] && bytes && bytes<=16384 && !(bytes&63));
  unsigned offset=remote-manifest[4];assert(offset<gddr.size()*4 && bytes<=gddr.size()*4-offset);
  if(!write && local==0x90e00){assert(bytes==128 && offset>=TT_Q38_PROJECTION_LINES_OFFSET && offset<TT_Q38_PROJECTION_LINES_OFFSET+24*128);lineReads++;}
  else if(!write && Range(local,0xa0000,12288)){assert(offset==TT_Q38_PROJECTION_NATIVE_OFFSET+local-0xa0000);operandReads++;}
  else if(local==0xab000){assert(offset==TT_Q38_PROJECTION_COMPACT_OFFSET);if(write)scaleWrites++;else scaleReads++;}
  else{
   assert(write && local>=0x80000 && local+bytes<=0x80000+119*1024 && offset>=TT_Q38_GDN_OUTPUT_RESULT);
   unsigned first=(offset-TT_Q38_GDN_OUTPUT_RESULT)/4;assert(first<outputCover.size() && bytes/4<=outputCover.size()-first);
   for(unsigned i=0;i<bytes/4;i++)assert(!outputCover[first+i]++);outputWrites++;
  }
  for(unsigned i=0;i<bytes/4;i++)if(write)gddr.at(offset/4+i)=memory.at(local+i*4);else memory[local+i*4]=gddr.at(offset/4+i);
  Retire(bytes,write);
 }
 void Packet()override{
  assert(pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9]);if(packets==dropPacket)return;
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];
  if(route==manifest[8])DramGdn();else PeerPacket();
  if(packets==mutateAtPacket)memory[TT_Q38_PROJECTION_GRAPH_MANIFEST+mutateWord*4]^=1;
  if(packets==sourceMutatePacket)memory[TT_Q38_GDN_OUTPUT_GRAPH_SOURCES+sourceMutateWord*4]^=1;
 }
 void Store(uint32_t a,uint32_t value)override{
  rootStores++;
  if(a<0xff000000)assert(PioneerQ38ProjectionGraphSnapshotContains(0,a) && !PioneerQ38ProjectionGraphProgramImmutable(0,a));
  if(a==0x17ffc8 && value==TT_Q38_GDN_OUTPUT_GRAPH_FAULT){unknown=true;l1[Index(a)]=value;return;}
  if(Range(a,TT_Q38_GDN_OUTPUT_GRAPH_SOURCE_SHADOW,119*64) || Range(a,TT_Q38_GDN_OUTPUT_GATHER_SHADOW,24*64)
   || Range(a,TT_Q38_GDN_OUTPUT_GATHER_SOURCES,24*64) || Range(a,0xa0000,12288)){
   assert(!pending && !unknown && !(a&3));memory[a]=value;
   if(Range(a,TT_Q38_GDN_OUTPUT_GRAPH_SOURCE_SHADOW,119*64))sourceShadowWrites++;
   if(Range(a,TT_Q38_GDN_OUTPUT_GATHER_SHADOW,24*64))gatherShadowWrites++;
   return;
  }
  assert(!Range(a,TT_Q38_GDN_OUTPUT_GRAPH_SOURCES,119*64));ProjectionParentMachine::Store(a,value);
 }
 void Inert(){auto stores=rootStores;ProjectionParentMachine::Inert();assert(rootStores==stores);}
 bool RunGdn(){
  while(pc!=TT_Q38_DISPATCH_PARK && pc!=TT_Q38_DISPATCH_FAULT_PARK)StepBlock();assert(minStack>=0x6e000);
  if(pc==TT_Q38_DISPATCH_FAULT_PARK){
   fprintf(stderr,"QOP1_FAULT packets%u producer%u matrix%u lines%u output%u stage%08x detail%08x lastRemote%08x bytes%u\n",packets,producerJobs,matrixJobs,lineReads,outputWrites,l1[Index(0x17ffc0)],l1[Index(0x17ffc4)],command[7]==0x212?command[3]:command[0],command[8]);
   assert(unknown);Inert();return false;
  }
  assert(l1[Index(0x17ffc8)]==TT_Q38_GDN_OUTPUT_GRAPH_MARKER);
  if(l1[Index(0x17ffc4)]!=(fixture.stop?119U:5120U))fprintf(stderr,"QOP1_REJECT packets%u result%u\n",packets,l1[Index(0x17ffc4)]);
  return l1[Index(0x17ffc4)]==(fixture.stop?119U:5120U);
 }
 void VerifyGdn(){
  assert(!pending && !unknown && boots==119 && sourceShadowWrites==119*16 && gatherShadowWrites==(fixture.stop?0U:24*16));
  for(unsigned w=0;w<119;w++){
   assert(actual[w]->memory.at(0x74144)==PioneerQ38ProjectionGraphNextPeer(manifest.data(),w) && node[w].stopped==fixture.stop);
   for(unsigned i=0;i<16;i++)assert(memory.at(TT_Q38_GDN_OUTPUT_GRAPH_SOURCES+w*64+i*4)==fixture.sources[w][i]
    && memory.at(TT_Q38_GDN_OUTPUT_GRAPH_SOURCE_SHADOW+w*64+i*4)==fixture.sources[w][i]);
  }
  if(fixture.stop)assert(stopJobs==119 && !producerJobs && !matrixJobs && !outputWrites && gddr==original);
  else{
   assert(producerJobs==24 && matrixJobs==119 && lineReads==24 && operandReads==2 && scaleWrites==1 && scaleReads==1);
   for(unsigned row=0;row<5120;row++)assert(outputCover[row]==1 && gddr.at(TT_Q38_GDN_OUTPUT_RESULT/4+row)==expected[row]);
   float input[6144];for(unsigned i=0;i<6144;i++)input[i]=Float(original.at(TT_Q38_GDN_GATED_OUTPUT/4+i));
   PioneerQ38CohortInput ref;assert(PioneerQ38CohortQuantize(8,6144,input,ref));
   for(unsigned i=0;i<3072;i++)assert(gddr.at(TT_Q38_PROJECTION_NATIVE_OFFSET/4+i)==ref.activation[i]);
   for(unsigned i=0;i<192;i++)assert(gddr.at(TT_Q38_PROJECTION_COMPACT_OFFSET/4+i)==ref.scales[i]);
   for(unsigned i=0;i<gddr.size();i++){
    unsigned at=i*4;bool written=Range(at,TT_Q38_PROJECTION_NATIVE_OFFSET,12288) || Range(at,TT_Q38_PROJECTION_LINES_OFFSET,3072)
     || Range(at,TT_Q38_PROJECTION_COMPACT_OFFSET,768) || Range(at,TT_Q38_GDN_OUTPUT_RESULT,20480);
    if(!written)assert(gddr[i]==original[i]);
   }
  }
  assert(memory.at(TT_Q38_PROJECTION_GRAPH_STATS)==TT_Q38_GDN_OUTPUT_GRAPH_MARKER && memory.at(TT_Q38_PROJECTION_GRAPH_STATS+56*4)==1);
  uint32_t receipt[64],before[8],after[8];
  for(unsigned i=0;i<64;i++)receipt[i]=memory.at(TT_Q38_PROJECTION_GRAPH_STATS+i*4);
  for(unsigned i=0;i<8;i++){before[i]=receipt[16+i];after[i]=counts[i==6?2:i];}
  assert(PioneerQ38GdnOutputGraphReceipt(receipt,manifest.data(),before,after)
   && !PioneerQ38ProjectionGraphReceipt(receipt,manifest.data(),before,after));
  for(unsigned i=0;i<64;i++){receipt[i]^=0x80000000U;assert(!PioneerQ38GdnOutputGraphReceipt(receipt,manifest.data(),before,after));receipt[i]^=0x80000000U;}
  runs++;
 }
};
#ifndef PIONEER_Q38_GDN_OUTPUT_GRAPH_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 auto m=std::make_unique<GdnGraphMachine>(argv[1],argv[2]);unsigned rows=0;
 for(unsigned changed=0;changed<2;changed++){
  if(changed)m->RetargetGdn(6,248319,false);
  assert(m->RunGdn());m->VerifyGdn();rows+=5120;
  printf("PASS ACTUAL QOP1 ALL119matrix jobs/5120rows/run%u/bank%u/token%u/op6->QOG1->complete ssm_out GDDR,FP32+FP64/NO host interstage calls or resets\n",changed,m->fixture.bank,m->token);
 }
 m->RetargetGdn(6,0,true);assert(m->RunGdn());m->VerifyGdn();
 printf("PASS ACTUAL QOP1 retained119boots ONCE/48producers/238matrixjobs/%uoutputrows/ALL119STOP/maxRootStack%u/matrixInstructions%llu;SOURCE+SHADOW disjoint/ALL stores inside old snapshot/unrelated GDDR unchanged/TEST_ONLY weights+transport,NOT_NATIVE_OR_MODEL_SPEED\n",rows,0x70000-m->minStack,(unsigned long long)m->matrixInstructions);
}
#endif
