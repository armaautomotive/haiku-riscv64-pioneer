/* SPDX-License-Identifier: MIT */
// Actual producer -> byte transport -> BFP8 matrix on the same worker.
// Synthetic model history, NoC, weights and FPU; no hardware claim.
#define PIONEER_TEST_BFP8 1
#define PIONEER_Q38_DENSE_Q8_CONNECTED_TEST 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#include "pioneer_q38_dense_q8_producer_image_test.cpp"
#include "../PioneerQ38DenseQ8MatrixRootFrame.h"
#include "../PioneerQ38DenseQ8MatrixRootJobs.h"
struct SchedulerIO {
 uint32_t table[7][3]={},tuple[119][4]={};unsigned fields=0;
 unsigned Table(unsigned bank,unsigned field){return table[bank][field];}
 void Output(unsigned worker,unsigned field,unsigned value){tuple[worker][field]=value;fields++;}
};
int main(int argc,char** argv){
 assert(argc==3);for(unsigned role=5;role<=7;role++){
 // A complete synthetic seven-bank table drives the same scheduling helper
 // used by the planned ROOT, including its real worker count and band sizes.
 SchedulerIO scheduler;const unsigned units=role==5?384:32;unsigned first=0;
 for(unsigned b=0;b<7;b++){
  const unsigned n=units/7+unsigned(b<units%7);
  scheduler.table[b][0]=0x04000000;scheduler.table[b][1]=first;scheduler.table[b][2]=n*32;first+=n*32;
 }
 assert(PioneerQ38DenseQ8MatrixRootJobs::Build(scheduler,3,role));
 assert(scheduler.fields==(role==5?119U:32U)*4);
 const auto* tuple=scheduler.tuple[0];assert(tuple[1]==0 && tuple[2]==0 && tuple[3]);
 DenseProducerImage image(argv[1],argv[2],5120,tuple[3]);image.Boot();
 assert(image.regs[8]==0x70000);image.Put(image.regs[8]-1136,3);
 for(unsigned i=0;i<256;i++)image.input[i]=PioneerAttentionBits(float(int(i%37)-18)*.03125f);
 image.StageProducer(role,1);image.Check(1);
 // Synthetic ROOT performs byte transport only: no host-computed activations.
 std::fill(image.transfer.root.begin(),image.transfer.root.end(),0);
 for(unsigned at=0;at<512;at+=4){
  const unsigned word=image.bank.at(image.descriptor[4]+image.descriptor[12]+at);
  for(unsigned b=0;b<4;b++)image.transfer.root[at+b]=uint8_t(word>>(8*b));
 }
 const unsigned indices[]={2,3,4,5,1,8,4,10};uint32_t before[8],after[8],proof[64],closed[64];
 for(unsigned i=0;i<8;i++)before[i]=image.transfer.counters[indices[i]];
 auto& leaf=image.transfer.job;leaf.words[11]=2;
 uint32_t job[64]={};
 assert(PioneerQ38DenseQ8MatrixRootFrame::Build(tuple,image.boot.data(),3,role,0,2025,1,job));
 for(unsigned i=0;i<16;i++)assert(job[16+i]==leaf.words[i]);
 assert(HW::JobShape(job,image.boot.data(),1,0));
 for(unsigned i=0;i<64;i++)image.Put(HW::Job+i*4,job[i]);image.Put(HW::Doorbell,2);
 assert(image.FinishCombined(2) && image.native.Clean());
 for(unsigned i=0;i<64;i++){proof[i]=image.Get(0x83f00+i*4);closed[i]=image.Get(HW::Job+i*4);}
 for(unsigned i=0;i<8;i++)after[i]=image.transfer.counters[indices[i]];
 assert(PioneerQ38HybridMatrixReceipt::Matrix(proof,closed,image.boot.data(),before,after));
 uint32_t expected[32]={};unsigned nonzero=0;
 // Post-hoc independent source decode and arithmetic; no answer feeds worker.
 for(unsigned tile=0;tile<160;tile++){
  float decoded[1024];assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,
   image.transfer.weights.data()+tile*1088,1088,decoded,1024));
  for(unsigned half=0;half<2;half++)for(unsigned phase=0;phase<4;phase++)for(unsigned row=0;row<32;row++){
   uint16_t weights[16],inputs[16];
   for(unsigned c=0;c<16;c++){
    const unsigned at=(tile*32+half*16+c)*2;inputs[c]=image.transfer.root[at]|unsigned(image.transfer.root[at+1])<<8;
    if(!row && !phase)nonzero+=inputs[c]!=0;
    const unsigned bits=NativeBfpMachine::Bits(decoded[row*32+half*16+c]);assert(!(bits&65535));weights[c]=uint16_t(bits>>16);
   }
   assert(PioneerBlackholeBf16Arithmetic::Instruction(weights,inputs,phase,expected[row],expected[row]));
  }
 }
 assert(nonzero>200);
 for(unsigned band=0;band<tuple[3];band++)for(unsigned row=0;row<32;row++)assert(image.Get(W::Output+(band*32+row)*4)==expected[row]);
 }
 puts("PASS actual QGWB/QGWD Q/G,K,V BF16 producer -> byte-only transport -> scheduled ROOT BFP8 frame -> matrix on SAME worker; synthetic history/NoC/FPU; NO HARDWARE OR SPEED");
}
