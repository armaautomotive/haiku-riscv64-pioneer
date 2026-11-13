/* SPDX-License-Identifier: MIT */
// Actual RV32 predicate in an isolated word-memory simulator, never device IO.
#define PIONEER_Q38_RECURRENT_ACTUAL_ROOT_HELPER_ONLY 1
#include "pioneer_q38_recurrent_actual_root_test.cpp"

int main(int argc,char** argv){
 assert(argc==2);std::ifstream f(std::string(argv[1])+"/program.demangled");std::string line;std::vector<unsigned> functions;
 while(std::getline(f,line))if(line.find("Projection::BfRecurrentGateReuseCandidate(")!=std::string::npos)
  functions.push_back(unsigned(std::stoul(line.substr(0,8),nullptr,16)));
 assert(!functions.empty());
 for(unsigned function:functions){
  auto m=std::make_unique<ActualRoot>(argv[1]);const unsigned d=0x74e00,input=0x140000;
  for(unsigned i=0;i<64;i++)m->memory[d+i*4]=0;
  m->memory[d+6*4]=2025;m->memory[d+14*4]=5120;m->memory[d+15*4]=8;
  m->memory[d+40*4]=3;m->memory[d+42*4]=0;m->memory[d+43*4]=6;m->memory[d+53*4]=input;
  for(unsigned i=0;i<8;i++)m->memory[d+(32+i)*4]=i+1;
  for(unsigned w=0;w<119;w++){
   const unsigned job=0x178000+w*256;m->memory[0x123000+w*4]=2;
   for(unsigned i=0;i<64;i++)m->memory[job+i*4]=0;
   m->memory[job]=Q::Magic;m->memory[job+4]=2;m->memory[job+8]=1;m->memory[job+12]=w+1;
   m->memory[job+16]=Q::Peer(w+1);m->memory[job+20]=0;m->memory[job+24]=3;m->memory[job+28]=2025;
   for(unsigned i=0;i<8;i++)m->memory[job+(8+i)*4]=i+1;
   m->memory[job+16*4]=PioneerQ38Bfp8WorkerJob::Magic;m->memory[job+18*4]=5120;
   m->memory[job+33*4]=6144;m->memory[job+34*4]=5;m->memory[job+35*4]=8;
  }
  for(unsigned i=0;i<5120;i++)m->memory[input+i*4]=m->memory[0xe0000+i*4]=0x3f000000U+i;
  for(unsigned i=0;i<2560;i++)m->memory[0xa0000+i*4]=m->memory[0xf2000+i*4]=i*31;
  assert(m->Run(function,d) && m->regs[10]==1);
  for(unsigned w=0;w<119;w++){
   const unsigned at=0x178000+w*256+34*4;m->memory[at]=6;
   assert(m->Run(function,d) && m->regs[10]==0);m->memory[at]=5;
  }
  for(unsigned index:{0U,2559U,5119U}){
   m->memory[input+index*4]^=1;assert(!m->Run(function,d));m->memory[input+index*4]^=1;
  }
  m->memory[0xa0000+2559*4]^=1;assert(!m->Run(function,d));
 }
 puts("PASS actual RV32 gate-to-QKV reuse predicate and mismatch refusals; NO DEVICE OR SPEED");
}
