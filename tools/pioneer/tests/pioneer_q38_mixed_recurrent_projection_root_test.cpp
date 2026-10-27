/* SPDX-License-Identifier: MIT */
// Execute real QRP1/QMW1 instructions. Only physical byte endpoints are
// modeled; no matrix products, nonces, PC or arithmetic state are seeded.
#define PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
#define PIONEER_Q38_MIXED_PROJECTION_ROOT_MACHINE_ONLY
#include "pioneer_q38_mixed_projection_root_test.cpp"
int main(int argc,char** argv){
 assert(argc==5 || (argc==6 && !strcmp(argv[5],"--full")));
 (void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;setvbuf(stdout,nullptr,_IOLBF,0);
 printf("START actual QRP1/119 QMW1 boot ONCE and genuine diverse arithmetic histories;NOT hardware timing\n");
 auto m=std::make_unique<ProjectionRootMachine>(argv[1],argv[2],argv[3],argv[4]);assert(m->WaitProjection(0));
 unsigned refusals=ProjectionRefusals(*m,argc==6),matrices=0;
 for(unsigned changed=0;changed<2;changed++)for(unsigned role:{10U,11U,5U,6U,13U,2U,3U,4U}){
  unsigned layer=changed?62:0;
  m->StageProjection(role,changed,layer);assert(m->WaitProjection(m->sequence));m->VerifyProjection();matrices++;
  printf("PASS ACTUAL QRP1 layer%u role%u rows%u workers%u/card quantize+Tensix+complete gather+GDDRscatter/SAME119 lifetime\n",
   layer,role,m->descriptor[11],m->active);
 }
 assert(0x70000-m->minStack<=8192);
 printf("PASS actual recurrent matrices%u/refusals%u/119bootsONCE/noSTOP/noReset/ROOTstack%u;SYNTHETIC weights and supplied inputs,NOT recurrent chain/native/fullmodel/TPS\n",
  matrices,refusals,0x70000-m->minStack);
}
