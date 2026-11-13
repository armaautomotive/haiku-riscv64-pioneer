/* SPDX-License-Identifier: MIT */
// Four actual simulated workers and inspection-only small projection plan.
// Root BF16 bytes are fixture inputs; there is no device or ROOT execution.
#define PIONEER_Q38_RECURRENT_GATE_QKV_HELPER_ONLY 1
#include "pioneer_q38_recurrent_gate_qkv_worker_bridge.cpp"
#include "../PioneerQ38RecurrentSmallMatrixSchedule.h"

int main(int argc,char** argv){
 assert(argc==5);using S=PioneerQ38RecurrentSmallMatrixSchedule;
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
 }
 puts("PASS four inspection worker frames: alpha/beta BFP8, all 96 model rows, padding never copied, invalid plans fail closed; simulated NoC, NO ROOT/DEVICE/WORK/SPEED");
}
