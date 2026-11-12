/* SPDX-License-Identifier: MIT */
// Execute linked controller helpers against synthetic ROOT memory only.
#define PIONEER_Q38_HYBRID_ACTUAL_ROOT_MACHINE_ONLY 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_INSPECTION 1
#include "pioneer_q38_hybrid_actual_root_test.cpp"
#include "../PioneerQ38DenseQ8MatrixWorkerLayout.h"
#include "../PioneerQ38DenseQ8MatrixRootFrame.h"
#include "../PioneerQ38RecurrentQ8MatrixRootJobs.h"
#include <fstream>
namespace Q=PioneerQ38DenseQ8MatrixWorkerLayout;
namespace P=PioneerQ38Bf16ProducerLayout;
struct Schedule {
 uint32_t table[7][3]={},parts[119][4]={};
 unsigned Table(unsigned b,unsigned f){return table[b][f];}
 void Output(unsigned w,unsigned f,unsigned v){parts[w][f]=v;}
};
static std::vector<unsigned> Symbols(const char* folder,const char* name){
 std::ifstream f(std::string(folder)+"/program.demangled");std::string line;std::vector<unsigned> out;
 while(std::getline(f,line))if(line.find(name)!=std::string::npos && line.find("[clone")==std::string::npos)
  out.push_back(unsigned(std::stoul(line.substr(0,8),nullptr,16)));
 assert(out.size()==2);return out;
}
static bool Call(ActualRoot& m,unsigned pc,unsigned d,unsigned worker,unsigned sequence,unsigned output,bool four){
 std::fill(std::begin(m.regs),std::end(m.regs),0);m.regs[1]=0x60004;m.regs[2]=0x70000;
 m.regs[10]=d;m.regs[11]=worker;m.regs[12]=four?sequence:output;if(four)m.regs[13]=output;
 m.pc=pc;m.steps=0;while(m.pc!=0x60004 && m.pc!=0x60000){m.Step();assert(m.regs[2]>=0x6e000 && m.regs[2]<=0x70000);}
 return m.pc==0x60004;
}
static bool RunBuildWithStub(ActualRoot& m,unsigned pc,unsigned read,unsigned d,unsigned offset,const uint32_t* record){
 std::fill(std::begin(m.regs),std::end(m.regs),0);m.regs[1]=0x60004;m.regs[2]=0x70000;
 m.regs[10]=d;m.regs[11]=0x6f000;m.pc=pc;m.steps=0;unsigned reads=0;
 while(m.pc!=0x60004 && m.pc!=0x60000){
  if(m.pc==read){
   assert(++reads==1 && m.regs[10]==d && m.regs[11]==0x6f000 && m.regs[12]==offset && m.regs[13]==128);
   for(unsigned i=0;i<32;i++)m.memory[0x78000+i*4]=record[i];m.pc=m.regs[1];
  }else m.Step();
  assert(m.regs[2]>=0x6e000 && m.regs[2]<=0x70000);
 }
 assert(reads==1);return m.pc==0x60004;
}
int main(int argc,char** argv){
 assert(argc==2);auto m=std::make_unique<ActualRoot>(argv[1]);
 auto descriptor=Symbols(argv[1],"Projection::BfDescriptor(");
 auto producer=Symbols(argv[1],"Projection::BfProducerJob(");
 auto seals=Symbols(argv[1],"Projection::BfDenseSeal(");
 auto builds=Symbols(argv[1],"Projection::BfBuildDenseSchedule(");
 auto reads=Symbols(argv[1],"Projection::BfBankRead(");
 std::ifstream symbols(std::string(argv[1])+"/program.demangled");std::string line;unsigned record=0;
 while(std::getline(symbols,line))if(line.find("Projection::BfRecurrentRecord(")!=std::string::npos){
  assert(!record);record=unsigned(std::stoul(line.substr(0,8),nullptr,16));
 }
 assert(record);
 const unsigned d=0x74e00,out=0x78000;for(unsigned i=0;i<64;i++)m->memory[d+i*4]=0;
 m->memory[d+4*4]=0x04000000;m->memory[d+4*6]=2025;m->memory[d+4*8]=PioneerQ38ProjectionRoute(0);
 m->memory[d+4*15]=8;m->memory[d+4*41]=248319;
 for(unsigned i=0;i<4;i++)m->memory[d+4*(32+i*2)]=i+1;
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned role:{5U,6U,13U}){
  m->memory[d+4*42]=layer;m->memory[d+4*43]=role;assert(m->Run(record,d));
  assert(m->regs[10]==0x3000200U+((layer-layer/4)*3+(role==5?0:role==6?1:2))*128);
 }
 for(unsigned layer:{0U,1U,2U,4U,62U})for(unsigned role:{5U,6U,13U}){
  const unsigned width=role==13?6144:5120,units=role==5?192:role==6?320:160;
  m->memory[d+4*14]=width;m->memory[d+4*42]=layer;m->memory[d+4*43]=role;
  Schedule schedule;unsigned first=0;
  for(unsigned b=0;b<7;b++){unsigned n=units/7+unsigned(b<units%7);schedule.table[b][0]=0x04000000+b*0x100000;
   schedule.table[b][1]=first;schedule.table[b][2]=n*32;first+=n*32;}
  assert(PioneerQ38RecurrentQ8MatrixRootJobs::Build(schedule,layer,role));
  uint32_t recordData[32]={};
  for(unsigned b=0;b<7;b++)for(unsigned f=0;f<3;f++)recordData[b*3+f]=schedule.table[b][f];
  const unsigned offset=0x3000200U+((layer-layer/4)*3+(role==5?0:role==6?1:2))*128;
  assert(RunBuildWithStub(*m,builds[0],reads[0],d,offset,recordData));
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<4;i++){
   assert(m->memory.at(0x16ebc0+w*16+i*4)==schedule.parts[w][i]);
   assert(m->memory.at(0x16f330+w*16+i*4)==schedule.parts[w][i]);
  }
  recordData[31]=1;assert(!RunBuildWithStub(*m,builds[0],reads[0],d,offset,recordData));recordData[31]=0;
  for(unsigned b=0;b<7;b++)for(unsigned f=0;f<3;f++){
   m->memory[0x16fb40+(b*3+f)*4]=schedule.table[b][f];
   m->memory[0x16fac0+(b*3+f)*4]=schedule.table[b][f];
  }
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<4;i++)
   m->memory[0x16ebc0+w*16+i*4]=m->memory[0x16f330+w*16+i*4]=schedule.parts[w][i];
  assert(m->Run(seals[0],d));
  for(unsigned w:{0U,19U,23U,62U,118U}){
   uint32_t boot[64]={};boot[0]=Q::Magic;boot[1]=1;boot[2]=64;boot[3]=w+1;boot[4]=Q::Peer(w+1);
   boot[5]=boot[16]=0x14000;boot[6]=8192;boot[7]=64;
   for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
   for(unsigned i=0;i<3;i++)for(unsigned lane=0;lane<6;lane++)boot[17+i]|=(i*6+lane)<<(lane*5);
   for(unsigned i=0;i<2;i++)for(unsigned lane=0;lane<6;lane++)boot[23+i]|=(i*6+lane)<<(lane*5);
   for(unsigned i=40;i<=50;i++)boot[i]=Q::BootWord(i);assert(Q::BootShape(boot));
   for(unsigned i=0;i<64;i++)m->memory[0x170000+w*256+i*4]=boot[i];
   if(w<width/256){
    assert(Call(*m,descriptor[0],d,w,0,out,false));
    uint32_t data[64];for(unsigned i=0;i<64;i++)data[i]=m->memory.at(out+i*4);
    assert(P::DescriptorShape(data) && data[5]==w && data[11]==width);
    assert(Call(*m,producer[0],d,w,7,out,true));
    for(unsigned i=0;i<64;i++)data[i]=m->memory.at(out+i*4);
    assert(Q::JobShape(data,boot,6,0));
   }else assert(!Call(*m,descriptor[0],d,w,0,out,false));
   if(!m->Run(m->job[0],d,w,7)){
    std::fprintf(stderr,"matrix job fault role=%u worker=%u return=%08x steps=%u\n",role,w,m->regs[1],m->steps);
    assert(false);
   }
   uint32_t frame[64],expected[64];for(unsigned i=0;i<64;i++)frame[i]=m->memory.at(out+i*4);
   assert(PioneerQ38DenseQ8MatrixRootFrame::Build(schedule.parts[w],boot,layer,role,0,2025,6,expected));
   for(unsigned i=0;i<64;i++)assert(frame[i]==expected[i]);
  }
  m->memory[0x16fb40]^=1;assert(!m->Run(seals[0],d));m->memory[0x16fb40]^=1;
  m->memory[0x16ebc0]^=1;assert(!m->Run(seals[0],d));m->memory[0x16ebc0]^=1;
 }
 puts("PASS linked RV32 recurrent descriptors, producer jobs, matrix jobs and mutation seals; SYNTHETIC ROOT MEMORY NO DEVICE");
}
