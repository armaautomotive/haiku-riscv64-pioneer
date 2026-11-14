/* SPDX-License-Identifier: MIT */
// Executes the actual offline RV32 candidate; no device or model IO.
#define PIONEER_Q38_RECURRENT_ACTUAL_ROOT_HELPER_ONLY 1
#include "pioneer_q38_recurrent_actual_root_test.cpp"

int main(int argc,char** argv){
 assert(argc==2);std::ifstream f(std::string(argv[1])+"/program.demangled");std::string line;std::vector<unsigned> functions;
 while(std::getline(f,line))if(line.find("Projection::BfFfnInputReuseCandidate(")!=std::string::npos)
  functions.push_back(unsigned(std::stoul(line.substr(0,8),nullptr,16)));
 assert(!functions.empty());
 for(unsigned function:functions){
  auto m=std::make_unique<ActualRoot>(argv[1]);const unsigned d=0x74e00,input=0x140000;
  for(unsigned i=0;i<64;i++)m->memory[d+i*4]=0;
  m->memory[d+6*4]=2026;m->memory[d+14*4]=5120;m->memory[d+15*4]=12;
  m->memory[d+40*4]=3;m->memory[d+42*4]=0;m->memory[d+43*4]=3;m->memory[d+53*4]=input;
  for(unsigned i=0;i<8;i++)m->memory[d+(32+i)*4]=i+1;
  for(unsigned w=0;w<119;w++){
   const unsigned job=0x178000+w*256;m->memory[0x123000+w*4]=2;
   for(unsigned i=0;i<64;i++)m->memory[job+i*4]=0;
   m->memory[job]=Q::Magic;m->memory[job+4]=2;m->memory[job+8]=1;m->memory[job+12]=w+1;
   m->memory[job+16]=Q::Peer(w+1);m->memory[job+20]=0;m->memory[job+24]=3;m->memory[job+28]=2025;
   for(unsigned i=0;i<8;i++)m->memory[job+(8+i)*4]=i+1;
   m->memory[job+16*4]=PioneerQ38Bfp4WorkerJob::Magic;m->memory[job+18*4]=5120;
   m->memory[job+33*4]=17408;m->memory[job+34*4]=2;m->memory[job+35*4]=4;
  }
  for(unsigned i=0;i<5120;i++)m->memory[input+i*4]=m->memory[0xe0000+i*4]=0x3f000000U+i;
  for(unsigned i=0;i<2560;i++)m->memory[0xa0000+i*4]=m->memory[0xf2000+i*4]=i*31;
  assert(m->Run(function,d) && m->regs[10]==1);
  for(unsigned layer=0;layer<64;layer++)if(layer%4!=3){
   m->memory[d+42*4]=layer;for(unsigned w=0;w<119;w++)m->memory[0x178000+w*256+5*4]=layer;
   assert(m->Run(function,d) && m->regs[10]==1);
  }
  m->memory[d+42*4]=0;for(unsigned w=0;w<119;w++)m->memory[0x178000+w*256+5*4]=0;
  for(unsigned field:{6U,14U,15U,42U,43U}){
   const unsigned at=d+field*4;const uint32_t old=m->memory[at];m->memory[at]=field==6?1:field==42?3:0;
   assert(m->Run(function,d) && m->regs[10]==0);m->memory[at]=old;
  }
  for(unsigned w=0;w<119;w++){
   const unsigned at=0x178000+w*256+34*4;m->memory[at]=3;
   assert(m->Run(function,d) && m->regs[10]==0);m->memory[at]=2;
  }
  for(unsigned w:{0U,59U,118U})for(unsigned field:{0U,1U,2U,3U,4U,5U,6U,7U,8U,15U,16U,18U,33U,35U}){
   const unsigned at=0x178000+w*256+field*4;m->memory[at]^=1;
   assert(m->Run(function,d) && m->regs[10]==0);m->memory[at]^=1;
  }
  for(unsigned index:{0U,2559U,5119U}){
   m->memory[input+index*4]^=1;assert(!m->Run(function,d));m->memory[input+index*4]^=1;
  }
  m->memory[0xa0000+2559*4]^=1;assert(!m->Run(function,d));
 }
 puts("PASS offline FFN reuse RV32 predicate/119 prior jobs/negative provenance and shadow checks; NO DEVICE OR SPEED CLAIM");
}
