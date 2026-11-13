/* SPDX-License-Identifier: MIT */
// Inspection-only ROOT job emitter. stdin: 119 simulated worker boot records;
// stdout: linked ROOT producer descriptors/jobs, then 119 matrix jobs.
// The host pipe is inspection transport, not a simulated ROOT NoC.
#define main recurrent_root_helper_main
#include "pioneer_q38_recurrent_actual_root_test.cpp"
#undef main
#include <cstdlib>

int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !std::strcmp(argv[3],"--store")));
 const bool store=argc==4;const unsigned role=unsigned(std::atoi(argv[2]));assert(role==5 || role==6 || role==13);
 const unsigned width=role==13?6144:5120,rows=role==5?6144:role==6?10240:5120,slot=role==5?0:role==6?1:2;
 std::array<std::array<uint32_t,64>,119> boots;
 assert(fread(boots.data(),sizeof(uint32_t),119*64,stdin)==119*64);
 std::vector<uint32_t> results;if(store){results.resize(rows);assert(fread(results.data(),sizeof(uint32_t),rows,stdin)==rows);}
 assert(fgetc(stdin)==EOF);
 for(unsigned w=0;w<119;w++)assert(Q::BootShape(boots[w].data()) && boots[w][3]==w+1);
 auto m=std::make_unique<ActualRoot>(argv[1]);const unsigned d=0x74e00;
 for(unsigned i=0;i<64;i++)m->memory[d+i*4]=0;
 m->memory[d+4*4]=0x04000000;m->memory[d+6*4]=2025;m->memory[d+8*4]=PioneerQ38ProjectionRoute(0);
 m->memory[d+14*4]=width;m->memory[d+15*4]=8;m->memory[d+41*4]=248319;m->memory[d+42*4]=0;m->memory[d+43*4]=role;
 for(unsigned i=0;i<4;i++)m->memory[d+(32+i*2)*4]=i+1;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();PioneerQ38HybridRecurrentQ8MatrixPlan::Proposal proposal;
 auto workspace=std::make_unique<std::array<uint32_t,PioneerQ38RecurrentQ8MatrixWorkspace::Words>>();
 assert(canonical->Build() && PioneerQ38HybridRecurrentQ8MatrixPlan::Build(*canonical,proposal)
  && PioneerQ38RecurrentQ8MatrixWorkspace::Build(*canonical,proposal,workspace->data(),workspace->size()));
 const auto builds=Symbols(argv[1],"Projection::BfBuildDenseSchedule(");
 const auto reads=Symbols(argv[1],"Projection::BfBankRead(");
 assert(RunBuildWithStub(*m,builds[0],reads[0],d,0x3000200+slot*128,workspace->data()+slot*32));
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<64;i++)m->memory[0x170000+w*256+i*4]=boots[w][i];
 if(store){
  m->memory[d+11*4]=rows;const auto stores=Symbols(argv[1],"Projection::BfStoreDenseMatrix(");
  const auto seals=Symbols(argv[1],"Projection::BfDenseSeal(");std::vector<uint8_t> seen(rows,0);
  for(unsigned row=0;row<rows;row++)m->memory[0x80000+row*4]=0x7fc00001;
  for(unsigned w=0;w<119;w++){
   const unsigned first=m->memory.at(0x16ebc0+w*16+8),count=m->memory.at(0x16ebc0+w*16+12)*32;
   assert(count && first<rows && count<=rows-first);
   for(unsigned i=0;i<count;i++){assert(!seen[first+i]++);m->memory[0x78000+(64+i)*4]=results[first+i];}
   assert(m->Run(stores[0],d,w));
  }
  assert(m->Run(seals[0],d)); // actual full pre-scatter seal, called by this fixture
  for(unsigned row=0;row<rows;row++){assert(seen[row]==1);results[row]=m->memory.at(0x80000+row*4);}
  assert(fwrite(results.data(),sizeof(uint32_t),rows,stdout)==rows && !fflush(stdout));return 0;
 }
 const auto descriptors=Symbols(argv[1],"Projection::BfDescriptor(");
 const auto producers=Symbols(argv[1],"Projection::BfProducerJob(");
 for(unsigned w=0;w<width/256;w++){
  uint32_t descriptor[64],producer[64];
  assert(Call(*m,descriptors[0],d,w,0,0x78000,false));
  for(unsigned i=0;i<64;i++)descriptor[i]=m->memory.at(0x78000+i*4);
  assert(Call(*m,producers[0],d,w,1,0x78000,true));
  for(unsigned i=0;i<64;i++)producer[i]=m->memory.at(0x78000+i*4);
  assert(fwrite(descriptor,sizeof(uint32_t),64,stdout)==64 && fwrite(producer,sizeof(uint32_t),64,stdout)==64);
 }
 for(unsigned w=0;w<119;w++){
  const unsigned previous=w<width/256?1:0;assert(m->Run(m->job[0],d,w,previous+1));uint32_t job[64];
  for(unsigned i=0;i<64;i++)job[i]=m->memory.at(0x78000+i*4);
  assert(Q::JobShape(job,boots[w].data(),previous,0));assert(fwrite(job,sizeof(uint32_t),64,stdout)==64);
 }
 assert(!fflush(stdout));
}
