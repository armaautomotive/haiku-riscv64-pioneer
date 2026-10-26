/* SPDX-License-Identifier: MIT */
// ACTUAL QDM1 ROOT/geometry only. Peer products/physical status SYNTHETIC.
#define PIONEER_Q38_PROJECTION_MATRIX_MACHINE_ONLY
#include "pioneer_q38_projection_matrix_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
 unsigned positive=0,refusals=0,rows=0,minStack=0x70000;
 for(unsigned layer=3;layer<64;layer+=4)for(unsigned role:{5U,6U,7U,10U}){
  auto jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(jobs->Build(*c,layer,role,123));
  ProjectionMatrixMachine m(argv[1],*c,*jobs,layer,role,0,layer%7);assert(m.Run());m.Verify();m.Inert();
  assert(m.outputPackets==jobs->Count() && m.gathers==jobs->Count());positive++;rows+=jobs->Outputs();minStack=std::min(minStack,m.minStack);
 }
 auto small=std::make_unique<PioneerQ38ProjectionJobs>();assert(small->Build(*c,3,6,123));
 for(unsigned word=0;word<64;word++){
  ProjectionMatrixMachine m(argv[1],*c,*small,3,6,0);m.corruptWorker=3;m.corruptProof=word;assert(!m.Run() && !m.outputPackets);m.Inert();refusals++;
 }
 for(unsigned stage=1;stage<=2;stage++)for(unsigned kind=0;kind<4;kind++){
  ProjectionMatrixMachine m(argv[1],*c,*small,3,6,0);if(!kind)m.failRead=stage;if(kind==1)m.lateSourceRead=stage;
  if(kind==2)m.lateFrameRead=stage;if(kind==3)m.foreignRead=stage;
  assert(!m.Run() && m.inputPackets==stage && !m.descriptors && !m.outputPackets);m.Inert();refusals++;
 }
 for(unsigned kind=0;kind<4;kind++){
  ProjectionMatrixMachine m(argv[1],*c,*small,3,6,0);if(!kind)m.failWrite=1;if(kind==1)m.lateSourceWrite=1;
  if(kind==2)m.lateRowsWrite=1;if(kind==3)m.foreignWrite=1;
  assert(!m.Run() && m.outputPackets==1);m.Inert();refusals++;
 }
 for(unsigned field=0;field<64;field++){
  ProjectionMatrixMachine m(argv[1],*c,*small,3,6,0);m.frame[field]^=0x80000000U;
  if(m.Shape())continue;m.SetFrame();assert(!m.Run() && !m.packets);refusals++;
 }
 printf("PASS ACTUAL QDM1 ROOT cases%u/globalrows%u/faults%u/maxStack%u;ALL16dense layersQG/K/V/Q6-O distinctslots,NoCPU-readback endpoints;SYNTHETIC peer products/status,NOT arithmetic/native/original-model/speed qualification\n",positive,rows,refusals,0x70000-minStack);
}
