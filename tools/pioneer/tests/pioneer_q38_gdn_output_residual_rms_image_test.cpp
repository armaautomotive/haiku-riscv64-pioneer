/* SPDX-License-Identifier: MIT */
// Actual QPE1 -> QRN1 residual bytes and actual ALL119 QOP1 -> QRR1.
// Synthetic weights/GDN input and byte-only NoC/GDDR endpoints. No native
// lifetime,physical-card performance or complete-model inference claim.
#define PIONEER_Q38_GDN_OUTPUT_GRAPH_MACHINE_ONLY
#include "pioneer_q38_gdn_output_graph_image_test.cpp"
#define PIONEER_GRAPH_MATH_BASE_ALREADY_INCLUDED
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_Q38_RESIDUAL_RMS_MACHINE_ONLY
#define PIONEER_Q38_RESIDUAL_RMS_REUSE_SCALAR
#include "pioneer_q38_residual_rms_image_test.cpp"
#define PIONEER_Q38_EMBEDDING_MACHINE_ONLY
#define PIONEER_Q38_EMBEDDING_REUSE_SCALAR
#include "pioneer_q38_embedding_image_test.cpp"
#define PIONEER_Q38_RMS_MACHINE_ONLY
#define PIONEER_Q38_RMS_REUSE_SCALAR
#include "pioneer_q38_learned_rms_image_test.cpp"
struct JoinedResidualRms:ResidualRmsMachine {
 std::vector<uint32_t>& fabric;
 JoinedResidualRms(const char* image,const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned bank,GdnGraphMachine& parent)
  :ResidualRmsMachine(image,canonical,token,bank),fabric(parent.gddr){
  assert(parent.fixture.stop && parent.stopJobs==119 && !parent.pending && !parent.unknown);
  for(unsigned w=0;w<119;w++)assert(parent.actual[w]->pc==TT_Q38_PEER_STOP);
  for(unsigned i=0;i<16;i++)descriptor[16+i]=source[i]=parent.fixture.root[i];
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=parent.counts[i==6?2:i]; // actual history,NO counter reset
  for(unsigned i=0;i<10;i++)command[i]=parent.command[i];
  descriptor[6]=parent.manifest[6]+1;descriptor[53]=parent.manifest[6];SetDescriptor();
  assert(PioneerQ38ResidualRmsShape(descriptor.data()));
  // Observer snapshots only. The device's actual read packets below copy
  // from SAME shared GDDR,not these arrays or any CPU arithmetic oracle.
  for(unsigned i=0;i<5120;i++){
   projected[i]=fabric.at(TT_Q38_GDN_OUTPUT_RESULT/4+i);raw[i]=backup[i]=fabric.at(TT_Q38_RMS_BACKUP/4+i);
   initial[i]=resident[i]=fabric.at(i);
  }
 }
 void CopyPacket(bool write,bool learned,bool rawRead,bool normalized,unsigned offset,unsigned local,unsigned bytes)override{
  if(learned){ResidualRmsMachine::CopyPacket(write,learned,rawRead,normalized,offset,local,bytes);return;}
  unsigned remote=write?(normalized?0U:unsigned(TT_Q38_RMS_BACKUP)):rawRead?unsigned(TT_Q38_RMS_BACKUP):unsigned(TT_Q38_GDN_OUTPUT_RESULT);
  for(unsigned i=0;i<bytes/4;i++){
   if(!write)memory[local+i*4]=fabric.at((remote+offset)/4+i);
   else fabric.at((remote+offset)/4+i)=(normalized?resident:backup)[offset/4+i]=memory.at(local+i*4);
  }
 }
};
int main(int argc,char** argv){
 assert(argc==6);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 (void)&PlanTests;(void)&RmsPlanTests;(void)&ResidualRmsPlanTests;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned rows=0,maxStack=0;
 for(unsigned changed=0;changed<2;changed++){
  unsigned bank=changed?6:0,token=changed?248319:0;
  EmbeddingMachine embedding(argv[4],*canonical,token,0,bank,changed);
  for(unsigned block=0;block<20;block++)for(unsigned lane=0;lane<16;lane++){
   uint16_t d=uint16_t(0x1800+((block+lane+changed)%7)*32),m=uint16_t(0x1000+((block*3+lane+changed)%5)*64);
   embedding.Put(block,lane,0,uint8_t(d));embedding.Put(block,lane,1,uint8_t(d>>8));
   embedding.Put(block,lane,2,uint8_t(m));embedding.Put(block,lane,3,uint8_t(m>>8));
  }
  assert(embedding.Run());embedding.Verify();
  LearnedRmsMachine pre(argv[5],*canonical,0,0,bank,changed);pre.input=pre.resident=embedding.resident;assert(pre.Run());pre.Verify();
  auto output=std::make_unique<GdnGraphMachine>(argv[1],argv[2],bank);
  if(token)output->RetargetGdn(bank,token,false);
  for(unsigned i=0;i<5120;i++){output->gddr.at(i)=pre.resident[i];output->gddr.at(TT_Q38_RMS_BACKUP/4+i)=pre.backup[i];}
  output->original=output->gddr;
  assert(output->RunGdn());output->VerifyGdn();
  auto projected=output->gddr;output->RetargetGdn(bank,token,true);assert(output->RunGdn());output->VerifyGdn();
  for(unsigned i=0;i<5120;i++)assert(output->gddr.at(TT_Q38_GDN_OUTPUT_RESULT/4+i)==projected.at(TT_Q38_GDN_OUTPUT_RESULT/4+i)
   && output->gddr.at(TT_Q38_RMS_BACKUP/4+i)==embedding.resident[i]);
  auto before=output->gddr;JoinedResidualRms post(argv[3],*canonical,token,bank,*output);assert(post.Run());post.Verify();
  for(unsigned i=0;i<output->gddr.size();i++){
   unsigned a=i*4;bool written=ScalarMachine::Range(a,0,20480) || ScalarMachine::Range(a,TT_Q38_RMS_BACKUP,20480);
   if(!written)assert(output->gddr[i]==before[i]);
  }
  bool changedResidual=false,changedNormalized=false;
  for(unsigned i=0;i<5120;i++){changedResidual|=post.backup[i]!=embedding.resident[i];changedNormalized|=post.resident[i]!=post.backup[i];}
  assert(changedResidual && changedNormalized);rows+=5120;maxStack=std::max(maxStack,0x70000-post.minStack);
  printf("PASS ACTUAL QPE1->QRN1 preserved raw5120+QOP1 ALL119matrix/5120output/119STOP->QRR1 shared-GDDR residual5120+postRMS5120 BIT-EXACT/token%u/bank%u/actual counters+NIUs retained/unrelated GDDR unchanged;NO CPU interstage arithmetic\n",token,bank);
 }
 printf("PASS OFFLINE actual joined QOP1->QRR1 %u output rows/maxQRR1Stack%u;SYNTHETIC_WEIGHTS_GDN_INPUT_TRANSPORT_NOT_NATIVE_LIFETIME_OR_MODEL_SPEED\n",rows,maxStack);
}
