/* SPDX-License-Identifier: MIT */
// Actual producer -> byte transport -> BFP8 matrix on the same worker.
// Synthetic model history, NoC, weights and FPU; no hardware claim.
#define PIONEER_TEST_BFP8 1
#define PIONEER_Q38_DENSE_Q8_CONNECTED_TEST 1
#include "pioneer_q38_dense_q8_producer_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);for(unsigned role=5;role<=7;role++){
 DenseProducerImage image(argv[1],argv[2],5120);image.Boot();
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
 uint32_t job[64]={};job[0]=HW::Magic;job[1]=2;job[2]=1;job[3]=image.boot[3];job[4]=image.boot[4];job[5]=3;job[7]=2025;
 for(unsigned i=0;i<8;i++)job[8+i]=image.boot[8+i];for(unsigned i=0;i<16;i++)job[16+i]=leaf.words[i];
 job[33]=role==5?12288:1024;job[34]=role;job[35]=8;assert(HW::JobShape(job,image.boot.data(),1,0));
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
 for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++)assert(image.Get(W::Output+(band*32+row)*4)==expected[row]);
 }
 puts("PASS actual QGWB Q/G,K,V BF16 producer -> byte-only ROOT transport -> BFP8 matrix on SAME worker; synthetic history/NoC/FPU; NO HARDWARE OR SPEED");
}
