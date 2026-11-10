/* SPDX-License-Identifier: MIT */
// Direct calls into linked QGW8 ROOT instructions on synthetic fixture data.
// No complete parent/worker lifecycle, learned model or device.
#define PIONEER_Q38_HYBRID_ACTUAL_ROOT_MACHINE_ONLY
#include "pioneer_q38_hybrid_actual_root_test.cpp"
#include <fstream>

static unsigned Symbol(const char* folder,const char* name){
 std::ifstream file(std::string(folder)+"/program.demangled");std::string line;unsigned address=0,count=0;
 while(std::getline(file,line))if(line.find(name)!=std::string::npos){address=unsigned(std::stoul(line.substr(0,8),nullptr,16));count++;}
 assert(count==1);return address;
}
static bool Call(ActualRoot& m,unsigned address,std::initializer_list<unsigned> args){
 std::fill(std::begin(m.regs),std::end(m.regs),0);m.regs[1]=0x60004;m.regs[2]=0x70000;
 unsigned r=10;for(unsigned v:args){assert(r<=15);m.regs[r++]=v;}m.pc=address;m.steps=0;
 while(m.pc!=0x60004 && m.pc!=0x60000){m.Step();assert(m.regs[2]>=0x6e000 && m.regs[2]<=0x70000);}
 return m.pc==0x60004;
}
int main(int argc,char** argv){
 assert(argc==4 || argc==5);std::array<uint32_t,256> fixture={};std::array<uint32_t,64> expectedJob={};
 std::ifstream input(argv[2],std::ios::binary);input.read(reinterpret_cast<char*>(fixture.data()),sizeof(fixture));assert(input.gcount()==sizeof(fixture) && input.peek()==EOF);
 std::ifstream jobfile(argv[3],std::ios::binary);jobfile.read(reinterpret_cast<char*>(expectedJob.data()),sizeof(expectedJob));assert(jobfile.gcount()==sizeof(expectedJob) && jobfile.peek()==EOF);
 auto m=std::make_unique<ActualRoot>(argv[1]);const unsigned root=0x75000,head=0x7f000,peer=0x170000,out=0x78000;
 for(unsigned i=0;i<64;i++){m->memory[root+i*4]=fixture[i];m->memory[peer+i*4]=fixture[192+i];}
 for(unsigned i=0;i<128;i++)m->memory[head+i*4]=fixture[64+i];
 const unsigned shape=Symbol(argv[1],"PioneerQ38HybridHeadLayout::Shape(");
 const unsigned makeJob=Symbol(argv[1],"PioneerQ38HybridHeadLayout::Job(");
 const unsigned pack=Symbol(argv[1],"GlobalChain::HeadBf16Pack(");
 assert(Call(*m,shape,{head,root,0,fixture[68]}) && m->regs[10]==1);
#ifdef PIONEER_Q38_QGWD_PLAN_SIMULATION
 // The linked QGWD ROOT must reject the formerly generated QGW8 witness.
 m->memory[head+40*4]=0x38574751;
 assert(Call(*m,shape,{head,root,0,fixture[68]}) && !m->regs[10]);
 m->memory[head+40*4]=fixture[64+40];
#endif
 m->memory[head+14*4]^=1;assert(Call(*m,shape,{head,root,0,fixture[68]}) && !m->regs[10]);m->memory[head+14*4]^=1;
 assert(Call(*m,makeJob,{head,0,0,0,peer,out}) && m->regs[10]==1);
 for(unsigned i=0;i<64;i++)assert(m->memory.at(out+i*4)==expectedJob[i]);
 const unsigned source=0x140000,destination=0xa0000;
 for(unsigned i=0;i<256;i++){
  unsigned value=0x3f000000U+((i*12345U)&0x007fffffU);if(i&1)value|=0x80000000U;
  if(i==0)value=0;if(i==1)value=0x80000000U;m->memory[source+i*4]=value;
 }
 for(unsigned i=0;i<128;i++)m->memory[destination+i*4]=0xa5a5a5a5;
 assert(Call(*m,pack,{source,destination}) && m->regs[10]==1);
 for(unsigned i=0;i<128;i++){
  const unsigned a=m->memory.at(source+8*i),b=m->memory.at(source+8*i+4);
  const unsigned low=(a+0x7fff+((a>>16)&1))>>16,high=(b+0x7fff+((b>>16)&1))>>16;
  assert(m->memory.at(destination+4*i)==(low|(high<<16)));
 }
 for(unsigned bad:{0x7fc00000U,1U,0x7f7fffffU}){
  m->memory[source+13*4]=bad;for(unsigned i=0;i<128;i++)m->memory[destination+i*4]=0xa5a5a5a5;
  assert(Call(*m,pack,{source,destination}) && !m->regs[10]);
  for(unsigned i=0;i<128;i++)assert(m->memory.at(destination+i*4)==0xa5a5a5a5);
 }
 if(argc==5){
  std::array<uint32_t,288> observed={};std::ifstream stream(argv[4],std::ios::binary);
  stream.read(reinterpret_cast<char*>(observed.data()),sizeof(observed));assert(stream.gcount()==sizeof(observed) && stream.peek()==EOF);
  const unsigned proof=0x90000,before=0xa1000,after=0xa1100;
  for(unsigned i=0;i<64;i++){m->memory[out+i*4]=observed[i];m->memory[peer+i*4]=observed[64+i];m->memory[proof+i*4]=observed[128+i];}
  for(unsigned i=0;i<8;i++){m->memory[before+i*4]=observed[256+i];m->memory[after+i*4]=observed[264+i];}
  const unsigned receipt=Symbol(argv[1],"PioneerQ38HybridMatrixReceipt::Matrix(");
  assert(Call(*m,receipt,{proof,out,peer,before,after}) && m->regs[10]==1);
  for(unsigned i=0;i<64;i++){
   m->memory[proof+i*4]^=1;assert(Call(*m,receipt,{proof,out,peer,before,after}) && !m->regs[10]);m->memory[proof+i*4]^=1;
  }
 }
#ifdef PIONEER_Q38_QGWD_PLAN_SIMULATION
 puts("PASS actual linked QGWD ROOT head shape/job and BF16 packing; QGW8 witness refused; synthetic data only, NO lifecycle/device/TPS");
#else
 puts("PASS actual linked QGW8 ROOT head shape/job and BF16 packing: independent 256-lane rounding, invalid inputs atomic; optional actual worker proof+64 corruptions; synthetic data only, NO lifecycle/device/TPS");
#endif
}
