/* SPDX-License-Identifier: MIT */
// Inspection-only worker endpoint. Boot/job bytes cross a host pipe; no card.
#define main recurrent_worker_helper_main
#include "pioneer_q38_recurrent_out_real_worker_test.cpp"
#undef main

int main(int argc,char** argv){
 assert(argc==4 && (!std::strcmp(argv[3],"--boot") || !std::strcmp(argv[3],"--run")));
 if(!std::strcmp(argv[3],"--boot")){
  for(unsigned w=0;w<119;w++){
   DenseProducerImage image(argv[1],argv[2],6144,1,w+1);
   assert(fwrite(image.boot.data(),sizeof(uint32_t),64,stdout)==64);
  }
  assert(!fflush(stdout));return 0;
 }
 std::array<std::array<uint32_t,64>,119> jobs;
 assert(fread(jobs.data(),sizeof(uint32_t),119*64,stdin)==119*64 && fgetc(stdin)==EOF);
 std::vector<uint8_t> input(6144*2);
 for(unsigned chunk=0;chunk<24;chunk++){
  auto producer=std::make_unique<DenseProducerImage>(argv[1],argv[2],6144,1,chunk+1);producer->Boot();
  for(unsigned i=0;i<256;i++){const unsigned at=chunk*256+i;
   producer->input[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  producer->StageProducer(13,1,false,0);producer->Check(1);
  for(unsigned at=0;at<512;at+=4){const unsigned word=producer->bank.at(producer->descriptor[4]+producer->descriptor[12]+at);
   for(unsigned b=0;b<4;b++)input[chunk*512+at+b]=uint8_t(word>>(8*b));}
 }
 std::vector<uint8_t> seen(5120,0);
 for(unsigned w=0;w<119;w++){
  const auto& job=jobs[w];const unsigned bands=job[20],first=job[32],previous=w?0:1;
  assert(bands && bands<=8 && job[2]==1 && job[3]==w+1 && job[18]==6144 && job[34]==13
   && first<5120 && bands*32<=5120-first);
  auto image=std::make_unique<DenseProducerImage>(argv[1],argv[2],6144,bands,w+1);image->Boot();
  SetPrivateWeights(*image,w%3,true);image->transfer.root=input;
  if(!w){
   // Preserve the producer's actual prior nonce and worker state, not a host receipt.
   for(unsigned i=0;i<256;i++)image->input[i]=uint32_t(uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0)))<<16;
   image->StageProducer(13,1,false,0);image->Check(1);
  }
  image->transfer.job.words[3]=job[19];image->transfer.job.words[5]=job[21];image->transfer.job.words[11]=previous+1;
  assert(HW::JobShape(job.data(),image->boot.data(),previous,0));
  for(unsigned i=0;i<64;i++)image->Put(HW::Job+i*4,job[i]);image->Put(HW::Doorbell,previous+1);
  assert(image->FinishCombined(previous+1) && image->native.Clean());
  for(unsigned band=0;band<bands;band++)for(unsigned row=0;row<32;row++){
   const unsigned dest=first+band*32+row;assert(!seen[dest]++);
   assert(image->Get(W::Output+(band*32+row)*4)==PioneerNativeBfpModelBandMetadata::Golden((w+band)%3)[row]);
  }
 }
 for(auto value:seen)assert(value==1);
 puts("PASS linked ROOT jobs -> 119 matrix workers with interleaved real-weight samples and exact 5120-row coverage; SIMULATED TRANSPORT NO DEVICE");
}
