/* SPDX-License-Identifier: MIT */
// Inspection-only worker endpoint. Boot/job bytes cross a host pipe; no card.
#define main recurrent_worker_helper_main
#include "pioneer_q38_recurrent_out_real_worker_test.cpp"
#undef main
#include <fstream>

int main(int argc,char** argv){
 assert((argc==4 && (!std::strcmp(argv[3],"--boot") || !std::strcmp(argv[3],"--run")))
  || (argc==5 && !std::strcmp(argv[3],"--run-full")));
 const unsigned tensor=PioneerNativeBfpModelBandMetadata::Tensor;
 assert(tensor==3 || tensor==5 || tensor==16);
 const unsigned role=tensor==3?5:tensor==5?6:13,width=role==13?6144:5120,rows=role==5?6144:role==6?10240:5120;
 assert(width==PioneerNativeBfpModelBandMetadata::Width);
 const bool full=argc==5;std::vector<uint8_t> projection;
 if(full){
  std::ifstream f(argv[4],std::ios::binary);
  projection.assign(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());
  assert(projection.size()==32+size_t(rows/32)*((width/32)*1088+128));
  const uint32_t expected[]={0x31525051,1,tensor,width,rows,rows/32,width/32,1088};
  assert(!std::memcmp(projection.data(),expected,sizeof(expected)));
 }
 if(!std::strcmp(argv[3],"--boot")){
  for(unsigned w=0;w<119;w++){
   DenseProducerImage image(argv[1],argv[2],width,1,w+1);
   assert(fwrite(image.boot.data(),sizeof(uint32_t),64,stdout)==64);
  }
  assert(!fflush(stdout));return 0;
 }
 std::array<std::array<uint32_t,64>,24> descriptors,producerJobs;
 for(unsigned w=0;w<width/256;w++){
  assert(fread(descriptors[w].data(),sizeof(uint32_t),64,stdin)==64);
  assert(fread(producerJobs[w].data(),sizeof(uint32_t),64,stdin)==64);
 }
 std::array<std::array<uint32_t,64>,119> jobs;
 assert(fread(jobs.data(),sizeof(uint32_t),119*64,stdin)==119*64 && fgetc(stdin)==EOF);
 std::vector<uint8_t> input(width*2);
 std::array<std::unique_ptr<DenseProducerImage>,24> producers;
 for(unsigned chunk=0;chunk<width/256;chunk++){
  const unsigned bands=jobs[chunk][20];assert(bands && bands<=8);
  auto producer=std::make_unique<DenseProducerImage>(argv[1],argv[2],width,bands,chunk+1);producer->Boot();
  for(unsigned i=0;i<256;i++){const unsigned at=chunk*256+i;
   producer->input[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  producer->StageProducer(role,1,false,0);
  for(unsigned i=0;i<64;i++){
   assert(producer->descriptor[i]==descriptors[chunk][i] && producer->Get(HW::Job+i*4)==producerJobs[chunk][i]);
   producer->WordWrite(BP::Descriptor+i*4,descriptors[chunk][i]);producer->Put(HW::Job+i*4,producerJobs[chunk][i]);
  }
  producer->Check(1);
  for(unsigned at=0;at<512;at+=4){const unsigned word=producer->bank.at(producer->descriptor[4]+producer->descriptor[12]+at);
   for(unsigned b=0;b<4;b++)input[chunk*512+at+b]=uint8_t(word>>(8*b));}
  producers[chunk]=std::move(producer);
 }
 std::vector<uint8_t> seen(rows,0);
 for(unsigned w=0;w<119;w++){
  const auto& job=jobs[w];const unsigned bands=job[20],first=job[32],previous=w<width/256?1:0;
  assert(bands && bands<=8 && job[2]==1 && job[3]==w+1 && job[18]==width && job[34]==role
   && first<rows && bands*32<=rows-first);
  auto image=previous?std::move(producers[w]):std::make_unique<DenseProducerImage>(argv[1],argv[2],width,bands,w+1);
  assert(image);if(!previous)image->Boot();
  if(full){
   for(unsigned band=0;band<bands;band++){
    const size_t weightBytes=size_t(width/32)*1088;
    const unsigned sourceBand=first/32+band;const size_t source=32+size_t(sourceBand)*(weightBytes+128);
    assert(source+weightBytes+128<=projection.size());
    std::memcpy(image->transfer.weights.data()+size_t(band)*weightBytes,projection.data()+source,weightBytes);
   }
  }else SetPrivateWeights(*image,w%3,true);
  image->transfer.root=input;
  image->transfer.job.words[3]=job[19];image->transfer.job.words[5]=job[21];image->transfer.job.words[11]=previous+1;
  assert(HW::JobShape(job.data(),image->boot.data(),previous,0));
  for(unsigned i=0;i<64;i++)image->Put(HW::Job+i*4,job[i]);image->Put(HW::Doorbell,previous+1);
  assert(image->FinishCombined(previous+1) && image->native.Clean());
  for(unsigned band=0;band<bands;band++)for(unsigned row=0;row<32;row++){
   const unsigned dest=first+band*32+row;assert(!seen[dest]++);
   uint32_t expected;
   if(full){const size_t weightBytes=size_t(width/32)*1088;
    const size_t source=32+size_t(first/32+band)*(weightBytes+128)+weightBytes+row*4;
    std::memcpy(&expected,projection.data()+source,4);}
   else expected=PioneerNativeBfpModelBandMetadata::Golden((w+band)%3)[row];
   assert(image->Get(W::Output+(band*32+row)*4)==expected);
  }
 }
 for(auto value:seen)assert(value==1);
 std::printf("PASS tensor%u role%u linked ROOT jobs -> 119 matrix workers with %s and exact %u-row coverage; SIMULATED TRANSPORT NO DEVICE\n",tensor,role,full?"distinct real-weight bands":"interleaved real-weight samples",rows);
}
