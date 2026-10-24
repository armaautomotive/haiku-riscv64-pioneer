/* SPDX-License-Identifier: MIT */
// Both actual instruction images must reject the other's DATA before NoC.
#define PIONEER_Q38_NEXT_RESIDUAL_RMS_MACHINE_ONLY
#include "pioneer_q38_next_residual_rms_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidualRmsPlan.h"
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&NextResidualRmsPlanTests;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned cases=0;
 for(unsigned bank=0;bank<7;bank++)for(unsigned token:{0U,248319U}){
  PioneerQ38GdnIdentity id={1,2,3,4};
  NextResidualRmsMachine next(argv[1],*canonical,token,bank,0,bank&1?62:1,bank&1?8191:0);
  PioneerQ38ResidualRmsPlan old;assert(old.Build(*canonical,token,bank,id,next.source,17));
  for(unsigned i=0;i<64;i++)next.descriptor[i]=old.Descriptor()[i];next.SetDescriptor();
  assert(PioneerQ38ResidualRmsShape(next.descriptor.data()) && !PioneerQ38NextResidualRmsShape(next.descriptor.data()));
  assert(!next.Run() && !next.packets && !next.nocReads && !next.commandWrites && !next.sumWrites && !next.normWrites);next.NoSink();cases++;
  NextResidualRmsMachine prior(argv[2],*canonical,token,bank,0,bank&1?62:1,bank&1?8191:0);
  assert(PioneerQ38NextResidualRmsShape(prior.descriptor.data()) && !PioneerQ38ResidualRmsShape(prior.descriptor.data()));
  while(prior.pc!=0x64038 && prior.pc!=0x64058)prior.StepScalar();
  assert(prior.pc==0x64038 && prior.memory.at(0x17ffc8)==TT_Q38_RESIDUAL_RMS_MARKER && !prior.memory.at(0x17ffc4)
   && !prior.packets && !prior.nocReads && !prior.commandWrites && !prior.sumWrites && !prior.normWrites);prior.NoSink();cases++;
  for(unsigned i=0;i<64;i++)assert(prior.memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4)==prior.descriptor[i]);
 }
 printf("PASS BOTH ACTUAL QRR1/QRR2 images reject other grammar/cases%u/ALL7banks/token+position+layer edges/zero NoC-command-numeric-sink writes/unchanged inputs;NO native or speed grant\n",cases);
}
