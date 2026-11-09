/* SPDX-License-Identifier: MIT */
// Execute the actual linked ROOT producer-frame builders. Synthetic metadata
// only: no card, live worker, full model, ownership or throughput claim.
#define PIONEER_Q38_HYBRID_ACTUAL_ROOT_MACHINE_ONLY
#include "pioneer_q38_hybrid_actual_root_test.cpp"
#include "../PioneerQ38DenseQ8MatrixWorkerLayout.h"
#include "../PioneerQ38Bf16ProducerLayout.h"
#include <fstream>
namespace Q=PioneerQ38DenseQ8MatrixWorkerLayout;
namespace P=PioneerQ38Bf16ProducerLayout;
static std::vector<unsigned> Symbols(const char* folder,const char* name){
 std::ifstream f(std::string(folder)+"/program.demangled");std::string line;std::vector<unsigned> out;
 while(std::getline(f,line))if(line.find(name)!=std::string::npos)out.push_back(unsigned(std::stoul(line.substr(0,8),nullptr,16)));
 assert(out.size()==2);return out;
}
static bool Call(ActualRoot& m,unsigned pc,unsigned d,unsigned worker,unsigned sequence,unsigned output,bool four){
 std::fill(std::begin(m.regs),std::end(m.regs),0);m.regs[1]=0x60004;m.regs[2]=0x70000;
 m.regs[10]=d;m.regs[11]=worker;m.regs[12]=four?sequence:output;if(four)m.regs[13]=output;
 m.pc=pc;m.steps=0;while(m.pc!=0x60004 && m.pc!=0x60000){m.Step();assert(m.regs[2]>=0x6e000 && m.regs[2]<=0x70000);}
 return m.pc==0x60004;
}
int main(int argc,char** argv){
 assert(argc==2);auto m=std::make_unique<ActualRoot>(argv[1]);
 auto descriptors=Symbols(argv[1],"Projection::BfDescriptor(");auto jobs=Symbols(argv[1],"Projection::BfProducerJob(");
 const unsigned d=0x74e00,out=0x78000;for(unsigned i=0;i<64;i++)m->memory[d+4*i]=0;
 m->memory[d+4*3]=0;m->memory[d+4*4]=0x04000000;m->memory[d+4*6]=2025;
 m->memory[d+4*8]=PioneerQ38ProjectionRoute(0);m->memory[d+4*14]=5120;m->memory[d+4*15]=8;
 for(unsigned i=0;i<4;i++)m->memory[d+4*(32+i*2)]=i+1;
 m->memory[d+4*41]=248319;m->memory[d+4*42]=3;
 for(unsigned w:{0U,19U}){
  uint32_t boot[64]={};boot[0]=Q::Magic;boot[1]=1;boot[2]=64;boot[3]=w+1;boot[4]=Q::Peer(w+1);
  boot[5]=boot[16]=0x14000;boot[6]=8192;boot[7]=64;
  for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
  for(unsigned i=0;i<3;i++)for(unsigned lane=0;lane<6;lane++)boot[17+i]|=(i*6+lane)<<(lane*5);
  for(unsigned i=0;i<2;i++)for(unsigned lane=0;lane<6;lane++)boot[23+i]|=(i*6+lane)<<(lane*5);
  for(unsigned i=40;i<=50;i++)boot[i]=Q::BootWord(i);assert(Q::BootShape(boot));
  for(unsigned i=0;i<64;i++)m->memory[0x170000+w*256+i*4]=boot[i];
  for(unsigned role=5;role<=7;role++)for(unsigned component=0;component<2;component++){
   m->memory[d+4*43]=role;
   if(!Call(*m,descriptors[component],d,w,0,out,false)){
    fprintf(stderr,"descriptor fault w=%u role=%u component=%u\n",w,role,component);
    for(unsigned i=0;i<64;i++)fprintf(stderr,"%u:%08x%c",i,m->memory[out+4*i],i%8==7?'\n':' ');
    assert(false);
   }
   uint32_t descriptor[64];
   for(unsigned i=0;i<64;i++)descriptor[i]=m->memory.at(out+4*i);
   assert(P::DescriptorShape(descriptor) && descriptor[5]==w && descriptor[7]==w+1 && descriptor[43]==role);
   assert(Call(*m,jobs[component],d,w,7,out,true));uint32_t job[64];
   for(unsigned i=0;i<64;i++)job[i]=m->memory.at(out+4*i);
   assert(Q::JobShape(job,boot,6,0) && job[28]==role && job[29]==5120 && job[1]==7);
  }
 }
 m->memory[d+4*43]=8;assert(!Call(*m,descriptors[0],d,0,0,out,false));
 m->memory[d+4*43]=5;m->memory[d+4*42]=2;assert(!Call(*m,descriptors[0],d,0,0,out,false));
 m->memory[d+4*42]=3;assert(!Call(*m,descriptors[0],d,20,0,out,false));
 assert(!Call(*m,jobs[0],d,0,0,out,true));
 puts("PASS actual linked QGWD ROOT dense producer descriptors and jobs for Q/G,K,V in both components; synthetic only, NO HARDWARE OR TPS");
}
