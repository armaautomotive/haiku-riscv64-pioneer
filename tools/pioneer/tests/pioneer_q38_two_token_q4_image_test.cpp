/* SPDX-License-Identifier: MIT */
// Compare actual issued RV32 and Tensix commands with two runs of the
// SHA-pinned independent single-row Q4 image. All inputs are synthetic.
#define PIONEER_Q38_BLOCK_MACHINE_ONLY
#include "pioneer_qwen38_block_image_test.cpp"
#include <memory>

struct PairImage:BlockMachine {
 unsigned aUnpack=0,bUnpack=0,publications=0;
 bool duplicate=false;
 explicit PairImage(const char* root,const BlockMachine& first,const BlockMachine& second,unsigned seed)
  :BlockMachine(root,12,seed,first.scale,first.tile.data(),true){
  for(unsigned w=0;w<16;w++)memory[0x74000+w*4]=0;
  memory[0x74000]=0x32543851;memory[0x74004]=12;memory[0x74008]=2304;
  memory[0x74010]=Bits(first.scale);memory[0x74014]=Bits(second.scale);
  for(unsigned w=0;w<128;w++){buffers[w]=first.buffers[w];buffers[128+w]=second.buffers[w];}
 }
 uint32_t Load(uint32_t at)override{
  if(Range(at,0x74000,64))return memory.at(at);
  if(Range(at,0x80200,512))return RuntimeMachine::Load(at);
  return BlockMachine::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  if(Range(at,0x84000,0x4000)){
   assert(Range(at,0x84000,128) && matrixInstructions==64 && drained);
   publications++;RuntimeMachine::Store(at,value);return;
  }
  BlockMachine::Store(at,value);
 }
 void Instruction(uint32_t op)override{
  BlockMachine::Instruction(op);
  if(op==0x420000c1)aUnpack++;
  if(op==0x428000c1){
   bUnpack++;
   for(unsigned row=2;row<8;row++)for(unsigned k=0;k<16;k++)assert(b[row][k]==0);
   if(duplicate)memcpy(b[1],b[0],sizeof(b[0]));
  }
 }
 bool RunPair(){
  while(pc!=0x72038 && pc!=0x72058)StepBlock();
  assert(!requests && !responses && minStack>=0x6e000);
  if(pc==0x72058){assert(l1[Index(0x17ffc8)]==0x46543851);return false;}
  assert(l1[Index(0x17ffc0)]==0x7200c && l1[Index(0x17ffc8)]==0x32543851);
  return l1[Index(0x17ffc4)]==16;
 }
 bool Match(const BlockMachine& first,const BlockMachine& second)const{
  if(publications!=32 || aUnpack!=16 || bUnpack!=16 || matrixInstructions!=64 || outputReads!=512)return false;
  for(unsigned n=0;n<16;n++){
   if(buffers[4096+n]!=first.buffers[4096+n] || buffers[4096+16+n]!=second.buffers[4096+n])return false;
  }
  for(unsigned n=32;n<4096;n++)assert(buffers[4096+n]==0x7fc00000);
  return true;
 }
};

int main(int argc,char** argv){
 assert(argc==3);
 const float scales[]={0.0f,-0.0f,.012345f,-.03125f,1.0f,0x1p-32f,0x1p16f};
 for(unsigned seed=0;seed<24;seed++){
  auto first=std::make_unique<BlockMachine>(argv[1],12,seed,scales[seed%7]);
  auto second=std::make_unique<BlockMachine>(argv[1],12,seed+149,scales[(seed+3)%7],first->tile.data());
  auto candidate=std::make_unique<PairImage>(argv[2],*first,*second,seed);
  assert(first->RunBlock() && second->RunBlock() && first->CorrectBlock() && second->CorrectBlock());
  assert(candidate->RunPair() && candidate->Match(*first,*second));
  for(unsigned w=0;w<128;w++){
   assert(candidate->buffers[w]==first->buffers[w]);
   assert(candidate->buffers[128+w]==second->buffers[w]);
  }
  for(const auto& word:first->memory)if(BlockMachine::Range(word.first,0x90000,2304))
   assert(candidate->memory.at(word.first)==word.second);
 }
 auto first=std::make_unique<BlockMachine>(argv[1],12,1,.012345f);
 auto second=std::make_unique<BlockMachine>(argv[1],12,7,-.03125f,first->tile.data());
 unsigned refused=0;
 for(unsigned word=0;word<16;word++){
  auto bad=std::make_unique<PairImage>(argv[2],*first,*second,1);
  bad->memory[0x74000+word*4]=word==4 || word==5?0x7f800000:bad->memory[0x74000+word*4]^1;
  assert(!bad->RunPair() && !bad->matrixInstructions && !bad->publications);refused++;
 }
 for(unsigned w=0;w<256;w++){
  auto bad=std::make_unique<PairImage>(argv[2],*first,*second,1);bad->buffers[w]=0;
  assert(!bad->RunPair() && !bad->matrixInstructions && !bad->publications);refused++;
 }
 auto bad=std::make_unique<PairImage>(argv[2],*first,*second,1);
 bad->memory[0x90000]&=~255U;
 bad->memory[0x90010]=(bad->memory[0x90010]&~255U)|0x7c;
 assert(!bad->RunPair() && !bad->publications);refused++;
 assert(first->RunBlock() && second->RunBlock());
 auto duplicate=std::make_unique<PairImage>(argv[2],*first,*second,1);duplicate->duplicate=true;
 assert(duplicate->RunPair() && !duplicate->Match(*first,*second));refused++;
 printf("PASS actual two-row Q4 RV32 24 blocks, 768 bitwise exact outputs, %u malformed/cross-talk cases; "
  "one weight unpack/4 fidelity phases per panel for both inputs; synthetic, no card or throughput result\n",refused);
}
