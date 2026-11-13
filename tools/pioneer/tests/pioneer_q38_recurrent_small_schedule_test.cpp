/* SPDX-License-Identifier: MIT */
// Four actual simulated workers and inspection-only small projection plan.
// Root BF16 bytes are fixture inputs; there is no device or ROOT execution.
#define PIONEER_Q38_RECURRENT_GATE_QKV_HELPER_ONLY 1
#include "pioneer_q38_recurrent_gate_qkv_worker_bridge.cpp"
#include "../PioneerQ38RecurrentSmallMatrixSchedule.h"
#include "PioneerQ38SmallPlanMachine.h"
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#include "../PioneerQ38DenseQ8MatrixRootFrame.h"

int main(int argc,char** argv){
 assert(argc==6);using S=PioneerQ38RecurrentSmallMatrixSchedule;static_assert(sizeof(S::Part)==268,"wire fixture part size");
 std::array<std::vector<uint8_t>,2> projection;
 for(unsigned kind=0;kind<2;kind++){
  std::ifstream f(argv[3+kind],std::ios::binary);projection[kind].assign(std::istreambuf_iterator<char>(f),{});
  assert(projection[kind].size()==32+2*(160*1088+128));
 }
 std::array<std::unique_ptr<DenseProducerImage>,4> workers;uint32_t boots[4][64]={};
 for(unsigned w=0;w<4;w++){
  workers[w]=std::make_unique<DenseProducerImage>(argv[1],argv[2],5120,1,w+1);workers[w]->Boot();
  for(unsigned i=0;i<64;i++)boots[w][i]=workers[w]->boot[i];
 }
 std::vector<uint8_t> input(5120*2);uint32_t raw[256],packed[128];
 for(unsigned chunk=0;chunk<20;chunk++){
  for(unsigned i=0;i<256;i++){unsigned at=chunk*256+i;raw[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  assert(PioneerQ38Bf16Input::Pack(raw,256,packed,128));std::memcpy(input.data()+chunk*512,packed,512);
  if(!chunk)for(unsigned i=0;i<256;i++)workers[0]->input[i]=raw[i];
 }
 workers[0]->StageProducer(10,1,false,0);workers[0]->Check(1);
 const uint32_t bases[]={0x10000000,0x11000000},banks[]={0,1},previous[]={1,0,0,0};S::Part parts[4]={};
 assert(S::Build(0,0,2025,bases,banks,boots,previous,parts));
 for(unsigned w=0;w<4;w++){
  const uint32_t tuple[]={bases[w/2]+(w%2)*S::BandBytes,banks[w/2],(w%2)*32,1};
  uint32_t rootFrame[64]={};
  assert(PioneerQ38DenseQ8MatrixRootFrame::Build(tuple,boots[w],0,10+w/2,0,2025,previous[w],rootFrame));
  assert(!std::memcmp(rootFrame,parts[w].frame,sizeof(rootFrame)));
  rootFrame[0]=0x76543210;
  assert(!PioneerQ38DenseQ8MatrixRootFrame::Build(tuple,boots[w],3,10+w/2,0,2025,previous[w],rootFrame)
   && rootFrame[0]==0x76543210);
 }
 PioneerQ38SmallPlanMachine root(argv[5]);
 for(unsigned i=0;i<2;i++){root.memory[0x91000+i*4]=bases[i];root.memory[0x91010+i*4]=banks[i];}
 for(unsigned i=0;i<4;i++)root.memory[0x91020+i*4]=previous[i];
 for(unsigned w=0;w<4;w++)for(unsigned i=0;i<64;i++)root.memory[0x90000+w*256+i*4]=boots[w][i];
 for(unsigned i=0;i<4*268/4;i++)root.memory[0x92000+i*4]=0x7fc00001;
 for(unsigned i=0;i<2*50;i++)root.memory[0x95000+i*4]=0x7fc00001;
 uint32_t args[]={3,0,2025,0x91000,0x91010,0x90000,0x91020,0x92000};
 assert(!root.Call(root.plan,args,8) && !root.writes);
 for(unsigned i=0;i<4*268/4;i++)assert(root.memory.at(0x92000+i*4)==0x7fc00001);
 args[0]=0;
 root.memory[0x91004]=bases[0];root.memory[0x91014]=banks[0];assert(!root.Call(root.plan,args,8) && !root.writes);
 root.memory[0x91004]=bases[1];root.memory[0x91014]=banks[1];
 root.memory[0x90000]^=1;assert(!root.Call(root.plan,args,8) && !root.writes);root.memory[0x90000]^=1;
 root.memory[0x91020]=0x7ffffffe;assert(!root.Call(root.plan,args,8) && !root.writes);root.memory[0x91020]=previous[0];
 args[3]=0;assert(!root.Call(root.plan,args,8) && !root.writes);args[3]=0x91000;
 for(unsigned i=0;i<4*268/4;i++)assert(root.memory.at(0x92000+i*4)==0x7fc00001);
 assert(root.Call(root.plan,args,8)==1 && root.writes==4*268/4);
 for(unsigned w=0;w<4;w++)for(unsigned i=0;i<268/4;i++){
  uint32_t expected;std::memcpy(&expected,reinterpret_cast<const uint8_t*>(&parts[w])+i*4,4);
  assert(root.memory.at(0x92000+w*268+i*4)==expected);
 }
 auto unchanged=std::array<S::Part,4>{};std::memcpy(unchanged.data(),parts,sizeof(parts));
 uint32_t invalidBases[]={bases[0],bases[0]},sameBanks[]={0,0};
 assert(!S::Build(0,0,2025,invalidBases,sameBanks,boots,previous,parts));assert(!std::memcmp(parts,unchanged.data(),sizeof(parts)));
 assert(!S::Build(3,0,2025,bases,banks,boots,previous,parts));assert(!std::memcmp(parts,unchanged.data(),sizeof(parts)));
 std::array<std::vector<uint32_t>,2> rows={std::vector<uint32_t>(64),std::vector<uint32_t>(64)};
 std::array<std::vector<uint8_t>,2> seen={std::vector<uint8_t>(64),std::vector<uint8_t>(64)};
 uint32_t output[2][50];for(auto& a:output)for(auto& x:a)x=0x7fc00001;
 for(unsigned w=0;w<4;w++){
  std::array<uint32_t,64> job;std::memcpy(job.data(),parts[w].frame,sizeof(parts[w].frame));
  Matrix(*workers[w],job,projection[w/2],input,rows[w/2],seen[w/2],nullptr);
  for(unsigned i=0;i<32;i++)root.memory[0x94000+i*4]=rows[w/2][parts[w].first+i];
  const uint32_t copyArgs[]={0x92000+w*268,0x94000,32,0x95000+(w/2)*200,48};
  const unsigned offsetWord=0x92000+w*268+66*4,originalOffset=root.memory.at(offsetWord);
  root.memory[offsetWord]^=4;assert(!root.Call(root.copy,copyArgs,5) && !root.writes);root.memory[offsetWord]=originalOffset;
  assert(root.Call(root.copy,copyArgs,5)==1 && root.writes==parts[w].valid);
  uint32_t before[50];std::memcpy(before,output[w/2],sizeof(before));
  for(unsigned mutation=0;mutation<6;mutation++){
   auto bad=parts[w];
   if(mutation==0)bad.valid++;
   else if(mutation==1)bad.offset+=4;
   else if(mutation==2)bad.frame[20]=2;
   else if(mutation==3)bad.frame[32]^=32;
   else if(mutation==4)bad.frame[33]=48;
   else bad.frame[34]=12;
   assert(!S::CopyRealRows(bad,rows[w/2].data()+parts[w].first,32,output[w/2],48));
   assert(!std::memcmp(before,output[w/2],sizeof(before)));
  }
  assert(S::CopyRealRows(parts[w],rows[w/2].data()+parts[w].first,32,output[w/2],48));
 }
 for(unsigned t=0;t<2;t++){
  for(unsigned row=0;row<64;row++){assert(seen[t][row]==1);if(row<48)assert(output[t][row]==rows[t][row]);else assert(!rows[t][row]);}
  assert(output[t][48]==0x7fc00001 && output[t][49]==0x7fc00001);
  for(unsigned row=0;row<50;row++)assert(root.memory.at(0x95000+t*200+row*4)==output[t][row]);
 }
 printf("PASS actual standalone RV32 plan/copy -> four simulated workers: 96 model rows, padding never copied, bad layer/offset refused, stack%u; NO NoC ROOT LIFECYCLE/DEVICE/WORK/SPEED\n",0x70000-root.minSP);
}
