/* SPDX-License-Identifier: MIT */
// Compiled experimental worker: producer -> bytes -> BFP8 matrix leaf.
// Synthetic NoC/FPU, private real-weight sample; not hardware or full-model validation.
#define PIONEER_TEST_BFP8 1
#define PIONEER_Q38_DENSE_Q8_CONNECTED_TEST 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_INSPECTION 1
#include "pioneer_q38_dense_q8_producer_image_test.cpp"
#include "../PioneerQ38DenseQ8MatrixRootFrame.h"
#include "../PioneerQ38RecurrentQ8MatrixRootJobs.h"
#include <memory>
#include "PioneerNativeBfpModelBandData.h"
#include "PioneerNativeBfpModelBandMetadata.h"
#include <cstdlib>
struct SchedulerIO {
 uint32_t table[7][3]={},tuple[119][4]={};unsigned fields=0;
 unsigned Table(unsigned b,unsigned f){return table[b][f];}
 void Output(unsigned w,unsigned f,unsigned v){tuple[w][f]=v;fields++;}
};
static void SetPrivateWeights(DenseProducerImage& image,unsigned sample){
 const unsigned tiles=image.native.tiles;assert(tiles==PioneerNativeBfpModelBandMetadata::Width/32);
 const unsigned bands=unsigned(image.transfer.weights.size()/(tiles*1088));assert(bands && bands*tiles*1088==image.transfer.weights.size());
 for(unsigned band=0;band<bands;band++)for(unsigned tile=0;tile<tiles;tile++)for(unsigned byte=0;byte<1088;byte+=4){
  const uint32_t word=PioneerNativeBfpModelBandData::Word(sample,PioneerNativeBfpModelBandMetadata::Width,0x8fff0+tile*1104+16+byte);
  for(unsigned j=0;j<4;j++)image.transfer.weights[(band*tiles+tile)*1088+byte+j]=uint8_t(word>>(8*j));
 }
}
int main(int argc,char** argv){
 assert(argc==4);const unsigned sample=unsigned(std::atoi(argv[3]));assert(sample<3);
 const unsigned tensor=PioneerNativeBfpModelBandMetadata::Tensor;
 assert((tensor==16 && PioneerNativeBfpModelBandMetadata::Width==6144)
  || ((tensor==3 || tensor==5) && PioneerNativeBfpModelBandMetadata::Width==5120));
 for(unsigned role:{tensor==16?13U:tensor==3?5U:6U}){
  const unsigned width=role==13?6144:5120,units=role==5?192:role==6?320:160;
  SchedulerIO scheduler;unsigned first=0;
  for(unsigned b=0;b<7;b++){
   unsigned n=units/7+unsigned(b<units%7);scheduler.table[b][0]=0x04000000;
   scheduler.table[b][1]=first;scheduler.table[b][2]=n*32;first+=n*32;
  }
  assert(PioneerQ38RecurrentQ8MatrixRootJobs::Build(scheduler,0,role) && scheduler.fields==119*4);
  const auto* tuple=scheduler.tuple[0];assert(!tuple[1] && !tuple[2] && tuple[3]);
  DenseProducerImage image(argv[1],argv[2],width,tuple[3]);image.Boot();SetPrivateWeights(image,sample);
  for(unsigned i=0;i<256;i++)image.input[i]=uint32_t(uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0)))<<16;
  image.StageProducer(role,1,false,0);image.Check(1);
  std::fill(image.transfer.root.begin(),image.transfer.root.end(),0);
  for(unsigned at=0;at<512;at+=4){
   unsigned word=image.bank.at(image.descriptor[4]+image.descriptor[12]+at);
   for(unsigned b=0;b<4;b++)image.transfer.root[at+b]=uint8_t(word>>(8*b));
  }
  // Every producer runs its compiled program on a separate simulated worker;
  // only its resulting bytes are copied into the matrix input.
  for(unsigned chunk=1;chunk<width/256;chunk++){
   auto producer=std::make_unique<DenseProducerImage>(argv[1],argv[2],width,tuple[3],chunk+1);producer->Boot();
   for(unsigned i=0;i<256;i++){unsigned at=chunk*256+i;producer->input[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
   producer->StageProducer(role,1,false,0);producer->Check(1);
   for(unsigned at=0;at<512;at+=4){
    unsigned word=producer->bank.at(producer->descriptor[4]+producer->descriptor[12]+at);
    for(unsigned b=0;b<4;b++)image.transfer.root[chunk*512+at+b]=uint8_t(word>>(8*b));
   }
  }
  const unsigned indices[]={2,3,4,5,1,8,4,10};uint32_t before[8],after[8],proof[64],closed[64];
  for(unsigned i=0;i<8;i++)before[i]=image.transfer.counters[indices[i]];
  auto& leaf=image.transfer.job;leaf.words[11]=2;uint32_t job[64]={};
  assert(PioneerQ38DenseQ8MatrixRootFrame::Build(tuple,image.boot.data(),0,role,0,2025,1,job));
  for(unsigned i=0;i<16;i++)assert(job[16+i]==leaf.words[i]);
  for(unsigned i=0;i<64;i++)image.Put(HW::Job+i*4,job[i]);image.Put(HW::Doorbell,2);
  assert(image.FinishCombined(2) && image.native.Clean());
  for(unsigned i=0;i<64;i++){proof[i]=image.Get(0x83f00+i*4);closed[i]=image.Get(HW::Job+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=image.transfer.counters[indices[i]];
  assert(PioneerQ38HybridMatrixReceipt::Matrix(proof,closed,image.boot.data(),before,after));
  uint32_t expected[32]={};unsigned nonzero=0;
  for(unsigned tile=0;tile<width/32;tile++){
   float decoded[1024];assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,
    image.transfer.weights.data()+tile*1088,1088,decoded,1024));
   for(unsigned half=0;half<2;half++)for(unsigned phase=0;phase<4;phase++)for(unsigned row=0;row<32;row++){
    uint16_t weights[16],inputs[16];
    for(unsigned c=0;c<16;c++){
     unsigned at=(tile*32+half*16+c)*2;inputs[c]=image.transfer.root[at]|unsigned(image.transfer.root[at+1])<<8;
     if(!row && !phase)nonzero+=inputs[c]!=0;
     unsigned bits=NativeBfpMachine::Bits(decoded[row*32+half*16+c]);assert(!(bits&65535));weights[c]=uint16_t(bits>>16);
    }
    assert(PioneerBlackholeBf16Arithmetic::Instruction(weights,inputs,phase,expected[row],expected[row]));
   }
  }
  assert(nonzero>width*9/10);for(unsigned row=0;row<32;row++)assert(expected[row]==PioneerNativeBfpModelBandMetadata::Golden(sample)[row]);
  for(unsigned band=0;band<tuple[3];band++)for(unsigned row=0;row<32;row++)assert(image.Get(W::Output+(band*32+row)*4)==expected[row]);
  // Exercise every remaining scheduled worker with the full produced input,
  // including its own address, row offset, band count and route.
  for(unsigned ordinal=1;ordinal<119;ordinal++){
   const auto* assigned=scheduler.tuple[ordinal];
   auto peer=std::make_unique<DenseProducerImage>(argv[1],argv[2],width,assigned[3],ordinal+1);peer->Boot();SetPrivateWeights(*peer,sample);
   peer->transfer.root=image.transfer.root;
   peer->transfer.job.words[3]=assigned[0];peer->transfer.job.words[5]=PioneerQ38ProjectionRoute(assigned[1]);peer->transfer.job.words[11]=1;
   uint32_t next[64]={};assert(PioneerQ38DenseQ8MatrixRootFrame::Build(assigned,peer->boot.data(),0,role,0,2025,0,next));
   for(unsigned i=0;i<16;i++)assert(next[16+i]==peer->transfer.job.words[i]);
   for(unsigned i=0;i<64;i++)peer->Put(HW::Job+i*4,next[i]);peer->Put(HW::Doorbell,1);
   assert(peer->FinishCombined(1) && peer->native.Clean());
   for(unsigned band=0;band<assigned[3];band++)for(unsigned row=0;row<32;row++)
    assert(peer->Get(W::Output+(band*32+row)*4)==expected[row]);
  }
  std::printf("PASS real recurrent tensor%u sample%u role%u K%u actual producer and 119 matrix-worker simulation; NO DEVICE\n",tensor,sample,role,width);
 }
}
