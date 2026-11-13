/* SPDX-License-Identifier: MIT */
// Linked RV32 controller; binary pipe supplies verified simulated-worker
// boots or results. The pipe models host transport, not on-card NoC.
#define PIONEER_Q38_RECURRENT_ACTUAL_ROOT_HELPER_ONLY 1
#include "pioneer_q38_recurrent_actual_root_test.cpp"

int main(int argc,char** argv){
 assert(argc==3 && (!std::strcmp(argv[2],"--jobs") || !std::strcmp(argv[2],"--publish")));
 const bool jobs=!std::strcmp(argv[2],"--jobs");auto root=std::make_unique<ActualRoot>(argv[1]);
 const auto builds=Symbols(argv[1],"Projection::BfBuildDenseSchedule("),reads=Symbols(argv[1],"Projection::BfBankRead(");
 const auto descriptor=Symbols(argv[1],"Projection::BfDescriptor("),producer=Symbols(argv[1],"Projection::BfProducerJob(");
 const auto stores=Symbols(argv[1],"Projection::BfStoreDenseMatrix(");
 uint32_t boots[20][64];assert(fread(boots,sizeof(boots),1,stdin)==1);
 const unsigned d=0x74e00,out=0x78000;for(unsigned i=0;i<64;i++)root->memory[d+i*4]=0;
 root->memory[d+4*4]=0x04000000;root->memory[d+4*6]=2025;root->memory[d+4*8]=PioneerQ38ProjectionRoute(0);
 root->memory[d+4*11]=48;root->memory[d+4*14]=5120;root->memory[d+4*15]=8;
 root->memory[d+4*41]=248319; // Match the worker fixture's token identity.
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 PioneerQ38HybridSmallRecurrentBfp8Plan::Proposal proposal;
 auto workspace=std::make_unique<std::array<uint32_t,PioneerQ38SmallRecurrentWorkspace::Words>>();
 assert(canonical->Build() && PioneerQ38HybridSmallRecurrentBfp8Plan::Build(*canonical,proposal)
  && PioneerQ38SmallRecurrentWorkspace::Build(*canonical,proposal,workspace->data(),workspace->size()));
 for(unsigned i=0;i<4;i++)root->memory[d+(32+i*2)*4]=i+1;
 for(unsigned w=0;w<20;w++)for(unsigned i=0;i<64;i++)root->memory[0x170000+w*256+i*4]=boots[w][i];
 for(unsigned kind=0;kind<2;kind++){
  const unsigned role=10+kind;
  root->memory[d+4*42]=0;root->memory[d+4*43]=role;
  assert(RunBuildWithStub(*root,builds[0],reads[0],d,
   PioneerQ38SmallRecurrentWorkspace::Offset+kind*128,workspace->data()+kind*32));
  for(unsigned i=0;i<64;i++)root->memory[0x80000+i*4]=0x7fc00001;
  if(jobs){
   for(unsigned w=0;w<20;w++){
    const unsigned producerSequence=1+kind*(w<2?2:1);
    uint32_t data[64];assert(Call(*root,descriptor[0],d,w,0,out,false));
    for(unsigned i=0;i<64;i++)data[i]=root->memory.at(out+i*4);assert(fwrite(data,sizeof(data),1,stdout)==1);
    assert(Call(*root,producer[0],d,w,producerSequence,out,true));
    for(unsigned i=0;i<64;i++)data[i]=root->memory.at(out+i*4);assert(fwrite(data,sizeof(data),1,stdout)==1);
   }
   for(unsigned w=0;w<2;w++){
    uint32_t data[64];const unsigned matrixSequence=2+kind*2;
    assert(root->Run(root->job[0],d,w,matrixSequence));
    for(unsigned i=0;i<64;i++)data[i]=root->memory.at(out+i*4);assert(fwrite(data,sizeof(data),1,stdout)==1);
    unsigned bank=0;while(bank<7 && !(*workspace)[kind*32+bank*3])bank++;
    assert(bank<7 && data[19]==(*workspace)[kind*32+bank*3]+w*174080
     && data[21]==PioneerQ38ProjectionRoute(bank) && data[32]==w*32 && data[20]==1);
   }
  }else{
   for(unsigned w=0;w<2;w++){
    uint32_t rows[32];assert(fread(rows,sizeof(rows),1,stdin)==1);
    for(unsigned i=0;i<32;i++)root->memory[out+(64+i)*4]=rows[i];
    assert(root->Run(stores[0],d,w));const unsigned valid=w?16:32;
    for(unsigned i=0;i<valid;i++)assert(root->memory.at(0x80000+(w*32+i)*4)==rows[i]);
   }
  }
  if(!jobs)for(unsigned i=48;i<64;i++)assert(root->memory.at(0x80000+i*4)==0x7fc00001);
 }
 assert(fgetc(stdin)==EOF);if(jobs)assert(!fflush(stdout));else puts("PASS linked ROOT published verified simulated worker results; NO DEVICE");
}
