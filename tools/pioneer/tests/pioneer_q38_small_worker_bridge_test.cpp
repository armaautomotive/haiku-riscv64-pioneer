/* SPDX-License-Identifier: MIT */
// Separate worker role. ROOT frames and results use an explicit host pipe.
#define PIONEER_Q38_RECURRENT_GATE_QKV_HELPER_ONLY 1
#include "pioneer_q38_recurrent_gate_qkv_worker_bridge.cpp"
int main(int argc,char** argv){
 assert(argc==6 && (!std::strcmp(argv[3],"--boot") || !std::strcmp(argv[3],"--run")));
 const bool bootOnly=!std::strcmp(argv[3],"--boot");std::array<std::unique_ptr<DenseProducerImage>,2> workers;
 for(unsigned w=0;w<2;w++){workers[w]=std::make_unique<DenseProducerImage>(argv[1],argv[2],5120,1,w+1);workers[w]->Boot();}
 if(bootOnly){for(auto& w:workers)assert(fwrite(w->boot.data(),4,64,stdout)==64);return fflush(stdout)?1:0;}
 std::vector<uint8_t> input(5120*2);uint32_t raw[256],packed[128];
 for(unsigned chunk=0;chunk<20;chunk++){
  for(unsigned i=0;i<256;i++){const unsigned at=chunk*256+i;raw[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  assert(PioneerQ38Bf16Input::Pack(raw,256,packed,128));std::memcpy(input.data()+chunk*512,packed,512);
  if(chunk<2)for(unsigned i=0;i<256;i++)workers[chunk]->input[i]=raw[i];
 }
 for(unsigned kind=0;kind<2;kind++){
  std::ifstream file(argv[4+kind],std::ios::binary);std::vector<uint8_t> weights{std::istreambuf_iterator<char>(file),{}};
  assert(weights.size()==32+2*(160*1088+128));std::vector<uint32_t> rows(64);std::vector<uint8_t> seen(64);
  for(unsigned w=0;w<2;w++){
   std::array<uint32_t,64> descriptor,producer,job;
   assert(fread(descriptor.data(),4,64,stdin)==64 && fread(producer.data(),4,64,stdin)==64 && fread(job.data(),4,64,stdin)==64);
   const unsigned sequence=1+kind*2;workers[w]->StageProducer(10+kind,sequence,false,0);
   for(unsigned i=0;i<64;i++){
    if(descriptor[i]!=workers[w]->descriptor[i] || producer[i]!=workers[w]->Get(HW::Job+i*4))
     std::fprintf(stderr,"frame mismatch role%u worker%u word%u desc%08x/%08x job%08x/%08x\n",
      10+kind,w,i,descriptor[i],workers[w]->descriptor[i],producer[i],workers[w]->Get(HW::Job+i*4));
    assert(descriptor[i]==workers[w]->descriptor[i] && producer[i]==workers[w]->Get(HW::Job+i*4));
   }
   workers[w]->Check(sequence);Matrix(*workers[w],job,weights,input,rows,seen,nullptr);
   assert(fwrite(rows.data()+w*32,4,32,stdout)==32);
  }
  for(unsigned i=0;i<64;i++){assert(seen[i]==1);if(i>=48)assert(!rows[i]);}
 }
 assert(fgetc(stdin)==EOF);return fflush(stdout)?1:0;
}
