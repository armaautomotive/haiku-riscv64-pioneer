/* SPDX-License-Identifier: MIT */
// One retained actual QGWF worker: producer -> alpha -> beta. CPU-only
// modeled NoC/tensor endpoints, not card execution or native scheduling.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_RECURRENT_GATE_QKV_HELPER_ONLY 1
#include "pioneer_q38_recurrent_gate_qkv_worker_bridge.cpp"
#include "PioneerQ38BfpConvWorker.h"
int main(int argc,char** argv){
 assert(argc==7);
 std::array<std::vector<uint8_t>,2> projection;
 for(unsigned kind=0;kind<2;kind++){
  std::ifstream f(argv[3+kind],std::ios::binary);
  projection[kind].assign(std::istreambuf_iterator<char>(f),{});
  assert(projection[kind].size()==32+2*(160*1088+128));
  uint32_t header[8];std::memcpy(header,projection[kind].data(),sizeof(header));
  assert(header[0]==0x31525051 && header[1]==1 && header[3]==5120 && header[4]==64
   && header[5]==2 && header[6]==160 && header[7]==1088);
 }
 PioneerQ38BfpConvWorker image(argv[1],argv[2],5120,2,1);image.Boot();
 std::vector<uint8_t> input(5120*2);uint32_t raw[256],packed[128];
 for(unsigned chunk=0;chunk<20;chunk++){
  for(unsigned i=0;i<256;i++){unsigned at=chunk*256+i;raw[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  assert(PioneerQ38Bf16Input::Pack(raw,256,packed,128));std::memcpy(input.data()+chunk*512,packed,512);
  if(!chunk)for(unsigned i=0;i<256;i++)image.input[i]=raw[i];
 }
 image.StageProducer(10,1,false,0);image.Check(1);
 for(unsigned kind=0;kind<2;kind++){
  const unsigned seq=kind+2,role=kind+10;std::array<uint32_t,64> job{};
  job[0]=HW::Magic;job[1]=seq;job[2]=1;job[3]=image.boot[3];job[4]=image.boot[4];job[5]=0;job[7]=2025;
  for(unsigned i=0;i<8;i++)job[8+i]=image.boot[8+i];
  for(unsigned i=0;i<16;i++)job[16+i]=image.transfer.job.words[i];
  job[20]=2;job[27]=seq;job[32]=0;job[33]=64;job[34]=role;job[35]=8;
  assert(HW::JobShape(job.data(),image.boot.data(),seq-1,0));
  auto bad=job;bad[32]=32;assert(!HW::JobShape(bad.data(),image.boot.data(),seq-1,0));
  bad=job;bad[34]=12;assert(!HW::JobShape(bad.data(),image.boot.data(),seq-1,0));
  bad=job;bad[5]=3;assert(!HW::JobShape(bad.data(),image.boot.data(),seq-1,0));
  std::vector<uint32_t> rows(64);std::vector<uint8_t> seen(64);
  Matrix(image,job,projection[kind],input,rows,seen,nullptr);
  for(unsigned row=0;row<64;row++){assert(seen[row]==1);if(row>=48)assert(!rows[row]);}
 }
 assert(image.Get(HW::Doorbell)==3 && image.Get(HW::Status+4)==3);
 std::ifstream learnedFile(argv[5],std::ios::binary);std::vector<uint32_t> learned(41184);
 assert(learnedFile.read(reinterpret_cast<char*>(learned.data()),learned.size()*4) && learnedFile.peek()==EOF);
 std::ifstream qkvFile(argv[6],std::ios::binary);std::vector<uint8_t> qkvBytes{std::istreambuf_iterator<char>(qkvFile),{}};
 assert(qkvBytes.size()==32+320*(160*1088+128));std::vector<uint32_t> qkv(10240);
 for(unsigned band=0;band<320;band++)for(unsigned row=0;row<32;row++)
  std::memcpy(&qkv[band*32+row],qkvBytes.data()+32+size_t(band)*(160*1088+128)+160*1088+row*4,4);
 image.steps=0;image.StageConv(4,qkv,learned);assert(image.FinishCombined(4));image.VerifyConv(4);
 image.steps=0;image.StageConv(5,qkv,learned);
 const unsigned corrupted=TT_Q38_CONV_BATCH_TABLE+383*4;
 image.WordWrite(corrupted,image.WordRead(corrupted)^1);
 assert(!image.FinishCombined(5) && !image.packets && !image.writes);
 DenseProducerImage rejected(argv[1],argv[2],5120,2,1);rejected.Boot();
 std::array<uint32_t,64> malformed{};malformed[0]=HW::Magic;malformed[1]=1;malformed[2]=1;
 malformed[3]=rejected.boot[3];malformed[4]=rejected.boot[4];malformed[5]=0;malformed[7]=2025;
 for(unsigned i=0;i<8;i++)malformed[8+i]=rejected.boot[8+i];
 for(unsigned i=0;i<16;i++)malformed[16+i]=rejected.transfer.job.words[i];
 malformed[20]=2;malformed[27]=1;malformed[32]=0;malformed[33]=64;malformed[34]=12;malformed[35]=8;
 assert(!HW::JobShape(malformed.data(),rejected.boot.data(),0,0));
 for(unsigned i=0;i<64;i++)rejected.Put(HW::Job+i*4,malformed[i]);rejected.Put(HW::Doorbell,1);
 assert(!rejected.FinishCombined(1) && !rejected.transfer.issues && !rejected.stores);
 puts("PASS one retained actual worker producer -> BFP8 alpha -> BFP8 beta -> Conv, model-weight rows and QKV/Conv inputs; NO ROOT/DEVICE/WORK/SPEED");
}
