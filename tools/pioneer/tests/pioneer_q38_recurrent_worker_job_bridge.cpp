/* SPDX-License-Identifier: MIT */
// Inspection-only worker endpoint. Boot/job bytes cross a host pipe; no card.
#define main recurrent_worker_helper_main
#include "pioneer_q38_recurrent_out_real_worker_test.cpp"
#undef main
#include <fstream>
#include "PioneerRecurrentWorkerSnapshot.h"

static void SnapshotStage(DenseProducerImage& image,PioneerRecurrentWorkerStage& stage,bool producer){
 const unsigned indices[]={2,3,4,5,1,8,4,10};stage.pc=image.pc;stage.doorbell=image.Get(HW::Doorbell);
 for(unsigned i=0;i<8;i++)stage.counters[i]=image.transfer.counters[indices[i]];
 for(unsigned i=0;i<16;i++)stage.status[i]=image.Get(HW::Status+i*4);
 for(unsigned i=0;i<64;i++)stage.job[i]=image.Get(HW::Job+i*4);
 if(!producer)return;
 for(unsigned i=0;i<64;i++){stage.proof[i]=image.Get(0x83f00+i*4);stage.descriptor[i]=image.descriptor[i];}
 for(unsigned i=0;i<16;i++)stage.inner[i]=image.Get(0x74000+i*4);
 for(unsigned i=0;i<32;i++)stage.line[i]=image.bank.at(image.descriptor[4]+image.descriptor[13]+i*4);
 for(unsigned i=0;i<128;i++)stage.native[i]=image.bank.at(image.descriptor[4]+image.descriptor[12]+i*4);
}
#ifndef PIONEER_Q38_RECURRENT_WORKER_BRIDGE_HELPER_ONLY
int main(int argc,char** argv){
 assert((argc==4 && (!std::strcmp(argv[3],"--boot") || !std::strcmp(argv[3],"--run")))
  || (argc==5 && (!std::strcmp(argv[3],"--run-full") || !std::strcmp(argv[3],"--run-full-input"))));
 const unsigned tensor=PioneerNativeBfpModelBandMetadata::Tensor;
 assert(tensor==3 || tensor==5 || tensor==16);
 const unsigned role=tensor==3?5:tensor==5?6:13,width=role==13?6144:5120,rows=role==5?6144:role==6?10240:5120;
 assert(width==PioneerNativeBfpModelBandMetadata::Width);
 const bool full=argc==5,dynamic=full && !std::strcmp(argv[3],"--run-full-input");std::vector<uint8_t> projection;
 assert(!dynamic || role==13);
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
 assert(fread(jobs.data(),sizeof(uint32_t),119*64,stdin)==119*64);
 std::vector<uint32_t> dynamicInput(dynamic?width:0),dynamicExpected(dynamic?rows:0);
 if(dynamic){
  assert(fread(dynamicInput.data(),sizeof(uint32_t),width,stdin)==width);
  assert(fread(dynamicExpected.data(),sizeof(uint32_t),rows,stdin)==rows);
 }
 assert(fgetc(stdin)==EOF);
 std::vector<uint8_t> input(width*2);
 std::array<std::unique_ptr<DenseProducerImage>,24> producers;
 std::array<PioneerRecurrentWorkerSnapshot,119> snapshots{};
 for(unsigned chunk=0;chunk<width/256;chunk++){
  const unsigned bands=jobs[chunk][20];assert(bands && bands<=8);
  auto producer=std::make_unique<DenseProducerImage>(argv[1],argv[2],width,bands,chunk+1);producer->Boot();
  SnapshotStage(*producer,snapshots[chunk].initial,false);
  for(unsigned i=0;i<256;i++){const unsigned at=chunk*256+i;
   producer->input[i]=dynamic?dynamicInput[at]:uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  producer->StageProducer(role,1,false,0);
  for(unsigned i=0;i<64;i++){
   assert(producer->descriptor[i]==descriptors[chunk][i] && producer->Get(HW::Job+i*4)==producerJobs[chunk][i]);
   producer->WordWrite(BP::Descriptor+i*4,descriptors[chunk][i]);producer->Put(HW::Job+i*4,producerJobs[chunk][i]);
  }
  producer->Check(1);
  SnapshotStage(*producer,snapshots[chunk].producer,true);
  for(unsigned at=0;at<512;at+=4){const unsigned word=producer->bank.at(producer->descriptor[4]+producer->descriptor[12]+at);
   for(unsigned b=0;b<4;b++)input[chunk*512+at+b]=uint8_t(word>>(8*b));}
  producers[chunk]=std::move(producer);
 }
 std::vector<uint8_t> seen(rows,0);std::vector<uint32_t> actualRows(rows,0);
 for(unsigned w=0;w<119;w++){
  const auto& job=jobs[w];const unsigned bands=job[20],first=job[32],previous=w<width/256?1:0;
  assert(bands && bands<=8 && job[2]==1 && job[3]==w+1 && job[18]==width && job[34]==role
   && first<rows && bands*32<=rows-first);
  auto image=previous?std::move(producers[w]):std::make_unique<DenseProducerImage>(argv[1],argv[2],width,bands,w+1);
  assert(image);if(!previous){image->Boot();SnapshotStage(*image,snapshots[w].initial,false);}
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
  const unsigned indices[]={2,3,4,5,1,8,4,10};uint32_t before[8],after[8],proof[64],closed[64];
  for(unsigned i=0;i<8;i++)before[i]=image->transfer.counters[indices[i]];
  for(unsigned i=0;i<64;i++)image->Put(HW::Job+i*4,job[i]);image->Put(HW::Doorbell,previous+1);
  assert(image->FinishCombined(previous+1) && image->native.Clean());
  for(unsigned i=0;i<64;i++){proof[i]=image->Get(0x83f00+i*4);closed[i]=image->Get(HW::Job+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=image->transfer.counters[indices[i]];
  assert(PioneerQ38HybridMatrixReceipt::Matrix(proof,closed,image->boot.data(),before,after));
  auto& snapshot=snapshots[w];snapshot.pc=image->pc;snapshot.doorbell=image->Get(HW::Doorbell);
  for(unsigned i=0;i<8;i++){snapshot.before[i]=before[i];snapshot.after[i]=after[i];}
  for(unsigned i=0;i<16;i++)snapshot.status[i]=image->Get(HW::Status+i*4);
  for(unsigned i=0;i<64;i++){snapshot.job[i]=closed[i];snapshot.proof[i]=proof[i];}
  assert(snapshot.initial.doorbell==0 && snapshot.initial.status[0]==0 && snapshot.initial.status[1]==0
   && snapshot.initial.status[3]==HW::Magic);
  for(unsigned i=0;i<64;i++)assert(!snapshot.initial.job[i]);
  if(previous){
   assert(snapshot.producer.doorbell==1 && snapshot.producer.status[0]==1 && snapshot.producer.status[1]==1
    && snapshot.producer.status[3]==HW::Magic && snapshot.producer.job[1]==1);
   for(unsigned i=0;i<8;i++)assert(snapshot.before[i]==snapshot.producer.counters[i]);
  }else for(unsigned i=0;i<8;i++)assert(snapshot.before[i]==snapshot.initial.counters[i]);
  for(unsigned band=0;band<bands;band++)for(unsigned row=0;row<32;row++){
   const unsigned dest=first+band*32+row;assert(!seen[dest]++);
   uint32_t expected;
   if(dynamic)expected=dynamicExpected[dest];
   else if(full){const size_t weightBytes=size_t(width/32)*1088;
    const size_t source=32+size_t(first/32+band)*(weightBytes+128)+weightBytes+row*4;
    std::memcpy(&expected,projection.data()+source,4);}
   else expected=PioneerNativeBfpModelBandMetadata::Golden((w+band)%3)[row];
   actualRows[dest]=image->Get(W::Output+(band*32+row)*4);assert(actualRows[dest]==expected);
   snapshot.output[band*32+row]=actualRows[dest];
  }
 }
 for(auto value:seen)assert(value==1);
 assert(fwrite(actualRows.data(),sizeof(uint32_t),rows,stdout)==rows);
 assert(fwrite(snapshots.data(),sizeof(PioneerRecurrentWorkerSnapshot),119,stdout)==119 && !fflush(stdout));
}
#endif
