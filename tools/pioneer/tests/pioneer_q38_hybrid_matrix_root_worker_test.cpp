/* SPDX-License-Identifier: MIT */
// Actual emitted ROOT dense jobs -> actual QGW7/QDM2 worker instructions.
// Synthetic frame/data and direct harness delivery: not real NoC or model.
#define PIONEER_Q38_HYBRID_MATRIX_TRANSITION_MACHINE_ONLY
#include "pioneer_q38_hybrid_matrix_attention_transition_test.cpp"
#define PIONEER_Q38_HYBRID_ACTUAL_ROOT_MACHINE_ONLY
#include "pioneer_q38_hybrid_actual_root_test.cpp"
#include "../PioneerQ38DensePartitionRootLayout.h"

static void RootJob(ActualRoot& root,TransitionImage& worker,unsigned phase,unsigned sequence,uint32_t* job){
 const unsigned ordinal=worker.boot[3]-1;
 assert(root.denseJob.size()==1 && ordinal<96);
 for(unsigned i=0;i<64;i++)root.memory[PioneerQ38DensePartitionRootLayout::Shadow+ordinal*256+i*4]=worker.WordRead(Dense::Header+i*4);
 assert(root.Run(root.denseJob[0],ordinal,phase,sequence));
 for(unsigned i=0;i<64;i++)job[i]=root.memory.at(PioneerQ38DensePartitionRootLayout::Scratch+i*4);
}
int main(int argc,char** argv){
 assert(argc==5);auto root=std::make_unique<ActualRoot>(argv[1]);auto old=std::make_unique<ActualRoot>(argv[2]);
 auto image=std::make_unique<TransitionImage>(argv[3],argv[4]);image->Boot();
 for(unsigned layer=0;layer<3;layer++){
  if(layer)image->RearmTestAccounting();
  image->Stage(layer,layer+1,4);assert(image->FinishCombined(layer+1));
 }
 for(unsigned phase=1;phase<=3;phase++){
  const unsigned sequence=3+phase;image->StageDense(phase,sequence);
  uint32_t job[64],legacy[64];RootJob(*root,*image,phase,sequence,job);RootJob(*old,*image,phase,sequence,legacy);
  assert(HW::JobShape(job,image->boot.data(),sequence-1,phase-1));
  assert(!HW::JobShape(legacy,image->boot.data(),sequence-1,phase-1));
  assert(job[0]==HW::Magic && legacy[0]==0x35574751U);
  for(unsigned i=1;i<64;i++)assert(job[i]==legacy[i]);
  for(unsigned i=0;i<64;i++)image->Put(HW::Job+i*4,job[i]);
  assert(image->FinishCombined(sequence));
  assert(image->WordRead(Dense::Summary)==Dense::Magic && image->WordRead(Dense::Local)==phase);
  if(phase==1)for(unsigned h=0;h<6;h++){
   float expected=0;
   for(unsigned d=0;d<256;d++)expected+=float(int((h*256+d)%7)-3)*.125f*float(int(d%11)-5)*.125f;
   expected*=.0625f;
   assert(PioneerAttentionFloat(image->WordRead(Dense::Scores+h*Dense::Stride*4))==expected);
  }
  uint32_t internal[16],frame[64];for(unsigned i=0;i<16;i++)internal[i]=image->WordRead(Dense::Job+i*4);
  for(unsigned i=0;i<64;i++)frame[i]=image->WordRead(Dense::Header+i*4);
  assert(Dense::JobShape(internal,frame,sequence,phase));
 }
 assert(image->native.matrixOps-image->matrixBefore==6*(128+4*8*16) && image->native.Clean());
 for(unsigned h=0;h<6;h++)for(unsigned d=0;d<256;d++){
  const float expected=float(int(d%17)-8)*.0625f;
  const float high=PioneerAttentionFloat(image->WordRead(Dense::Partial+(h*512+d*2)*4));
  const float low=PioneerAttentionFloat(image->WordRead(Dense::Partial+(h*512+d*2+1)*4));
  assert(high+low==expected);
 }
 image->denseMode=false;image->RearmTestAccounting();image->Stage(3,7,4);
 assert(image->FinishCombined(7) && image->native.Clean());
 const auto* golden=PioneerNativeBfp4BandGolden(17408);assert(golden);
 for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++)
  assert(image->Get(W::Output+(band*32+row)*4)==golden[row]);
 // A real legacy ROOT instruction emits a QGW5 job. The actual QGW7
 // dispatcher must fault before starting any matrix work or NoC request.
 auto rejected=std::make_unique<TransitionImage>(argv[3],argv[4]);rejected->Boot();
 for(unsigned layer=0;layer<3;layer++){
  if(layer)rejected->RearmTestAccounting();
  rejected->Stage(layer,layer+1,4);assert(rejected->FinishCombined(layer+1));
 }
 rejected->StageDense(1,4);uint32_t legacy[64];RootJob(*old,*rejected,1,4,legacy);
 const unsigned matrixBefore=rejected->native.matrixOps,transfersBefore=rejected->transfer.issues;
 for(unsigned i=0;i<64;i++)rejected->Put(HW::Job+i*4,legacy[i]);
 assert(!rejected->FinishCombined(4) && rejected->native.matrixOps==matrixBefore && rejected->transfer.issues==transfersBefore);
 puts("PASS actual QGW7 ROOT dense jobs -> actual QGW7/QDM2 three-phase attention -> next projection; old ROOT jobs refused; synthetic direct delivery, no device/model/TPS");
}
