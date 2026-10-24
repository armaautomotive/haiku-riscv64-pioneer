/* SPDX-License-Identifier: MIT */
// Actual QRR1 -> SAME GDDR -> QFG3 + ALL119 actual RV32/FPU/Tensix peers.
// Byte-only simulated endpoints;synthetic weights,not native lifetime/TPS.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_FFN_GRAPH_COOPERATIVE_FIXTURE_ONLY
#include "pioneer_q38_ffn_graph_cooperative_test.cpp"
#include "pioneer_q38_ffn_residual_graph_fixture.h"
#define PIONEER_GRAPH_MATH_BASE_ALREADY_INCLUDED
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_Q38_RESIDUAL_RMS_MACHINE_ONLY
#define PIONEER_Q38_RESIDUAL_RMS_REUSE_SCALAR
#include "pioneer_q38_residual_rms_image_test.cpp"
struct FfnRmsProducer:ResidualRmsMachine {
 std::vector<uint32_t>& fabric;
 FfnRmsProducer(const char* image,const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned bank,
  std::vector<uint32_t>& data,unsigned changed):ResidualRmsMachine(image,canonical,token,bank),fabric(data){
  for(unsigned i=0;i<5120;i++){
   raw[i]=Bits(.5f+float((i*17+changed*31)%127)/256.f);
   projected[i]=Bits(float(int((i*37+changed)%31)-15)/256.f);
   weight[i]=Bits(.75f+float((i*13+changed)%63)/128.f);
   fabric.at(i)=fabric.at(TT_Q38_RMS_BACKUP/4+i)=0x7fc00001;
  }
 }
 void CopyPacket(bool write,bool learned,bool rawRead,bool normalized,unsigned offset,unsigned local,unsigned bytes)override{
  if(!write){ResidualRmsMachine::CopyPacket(write,learned,rawRead,normalized,offset,local,bytes);return;}
  for(unsigned i=0;i<bytes/4;i++)fabric.at((normalized?0U:unsigned(TT_Q38_RMS_BACKUP)/4)+offset/4+i)
   =(normalized?resident:backup)[offset/4+i]=memory.at(local+i*4);
 }
};
int main(int argc,char** argv){
 assert(argc==4 || (argc==5 && (!strcmp(argv[4],"--once") || !strcmp(argv[4],"--edge-once"))));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&ResidualRmsPlanTests;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned trials=argc==5?1:2,start=argc==5 && !strcmp(argv[4],"--edge-once")?1:0,maxStack=0;std::vector<uint32_t> first;
 for(unsigned changed=start;changed<start+trials;changed++){
  unsigned bank=changed?6:0,token=changed?248319:0;
  auto root=std::make_unique<ResidualGraphFixture<GraphArithmeticRoot>>(argv[1],argv[2],true,bank,changed!=0,0);
  FfnRmsProducer post(argv[3],*canonical,token,bank,root->gddr,changed);
  for(unsigned i=0;i<16;i++)post.descriptor[16+i]=post.source[i]=root->rootSource[i];post.SetDescriptor();
  assert(post.Run());post.Verify(); // actual instructions publish BOTH poisoned GDDR inputs
  auto& d=root->fixture.manifest;
  d[4]=post.descriptor[4];d[6]=post.descriptor[6]+1;d[13]=d[6]+5;d[40]=d[41]=d[42]=0;
  for(unsigned i=32;i<40;i++)d[i]=post.descriptor[i];
  for(unsigned i=0;i<8;i++)root->counts[i]=post.counts[i]; // real inherited physical counter history
  for(unsigned i=0;i<10;i++)root->command[i]=post.command[i];
  for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)root->memory[TT_Q38_FFN_GRAPH_MANIFEST+i*4]=d[i];
  assert(PioneerQ38FfnGraphManifest(d.data(),root->fixture.source.data()));root->CaptureProduced();
  auto input=root->savedInput,raw=root->savedRaw;
  if(!root->RunGraph()){
   fprintf(stderr,"QFG3_ACTUAL_FAILURE rootPC%08x packets%u matrices%u producers%u\n",root->pc,root->packets,root->finishedMatrices,root->finishedProducers);return 1;
  }
  root->VerifyActual();root->VerifyResidual();assert(root->savedInput==input && root->savedRaw==raw);
  for(unsigned w=0;w<119;w++)assert(root->actual[w]->verified==PioneerQ38FfnGraphNextPeer(d.data(),w));
  if(first.empty())first.assign(root->gddr.begin(),root->gddr.begin()+5120);
  else assert(first!=std::vector<uint32_t>(root->gddr.begin(),root->gddr.begin()+5120));
  auto output=root->gddr;root->StageStop();assert(root->RunGraph() && root->gddr==output && root->issued==6);
  uint64_t math=0;for(const auto& p:root->actual){assert(p->pc==TT_Q38_PEER_STOP && p->prepared==p->verified && !p->dotProbeReads);math+=p->arithmetic;}
  maxStack=std::max(maxStack,0x70000-root->minStack);
  printf("PASS ACTUAL QRR1->QFG3 sharedGDDR normalized5120+residual5120/ALL119peers full5120->17408 gate+up/68SiLU/17408->5120down/final5120F32 residual BITEXACT/119STOP/token%u/bank%u/TensixInstructions%llu;NO CPU interstage arithmetic;SYNTHETIC_WEIGHTS_NOT_NATIVE_OR_MODEL_SPEED\n",token,bank,(unsigned long long)math);
 }
 printf("PASS joined actual QRR1+QFG3+ALL119 peers trials%u/maxROOTstack%u;card-resident FFN+residual instruction path ONLY,not SAME native owner/full model/physical TPS\n",trials,maxStack);
}
