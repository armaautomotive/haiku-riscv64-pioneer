/* SPDX-License-Identifier: MIT */
// Raw ORIGINAL learned bytes + supplied initial hidden/zero pos0 state.
// Every intermediate is produced by actual emitted instructions. This is
// NOT a valid embedding/full64/token/native/card-speed qualification.
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_MACHINE_ONLY
#include "pioneer_q38_mixed_recurrent_block_root_test.cpp"
namespace {
const char* originalBlockPath=nullptr;
FILE* OriginalBlockFopen(const char* path,const char* mode){
 assert(originalBlockPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));return fopen(originalBlockPath,mode);
}
int OriginalBlockExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
}
#define fopen OriginalBlockFopen
#define execv OriginalBlockExecv
#include "pioneer_q38_mixed_recurrent_block_original_bytes.h"
#undef fopen
#undef execv
struct OriginalRecurrentBlockMachine:RecurrentBlockMachine {
 RecurrentBlockOriginalBytes& original;
 OriginalRecurrentBlockMachine(const char* image,const char* worker,RecurrentBlockOriginalBytes& bytes):RecurrentBlockMachine(image,worker),original(bytes){
  assert(original.Ready());
  // Remove EVERY synthetic learned endpoint. Only raw hidden, canonical
  // metadata, NaN destinations and position0 zero CURRENT state remain.
  for(auto i=dram.begin();i!=dram.end();){
   if(original.Contains(unsigned(i->first>>32),unsigned(i->first),4))i=dram.erase(i);else ++i;
  }
  for(unsigned w=0;w<120;w++)for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++){
   const auto* d=block->Conv()+w*384+j*64;
   for(unsigned i=0;i<48;i++)Put(d[41],d[15]+i*4,0);
  }
  for(unsigned w=0;w<96;w++){
   const auto* st=block->Gdn()+w*192+64;
   for(unsigned i=0;i<8192;i++)Put(st[12],st[5]+i*4,0);
  }
 }
 uint32_t Get(unsigned route,unsigned address)const override{
  uint32_t value;if(original.Contains(route,address,4)){assert(original.Word(route,address,value));return value;}
  // No fallback to the synthetic matrix generator, even on a missing span.
  auto raw=dram.find(Key(route,address));assert(raw!=dram.end());return raw->second;
 }
 void Submit()override{
  // A matrix's byte endpoint snapshots many raw tile words here. Bracket
  // that whole exact packet with fresh authentication/liveness checks;
  // worker Conv/GDN Word reads outside it check the same source directly.
  assert(original.BeginRawPacket());RecurrentBlockMachine::Submit();assert(original.EndRawPacket());
 }
};
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 originalBlockPath=argv[3];
 puts("START full c600 ORIGINAL SHA and raw complete layer0 weights BEFORE instruction context;NO intermediate products or native IO");
 RecurrentBlockOriginalBytes original;assert(original.Prepare());
 printf("READY ORIGINAL raw complete layer0 bytes%llu/pages%u/spans%u;all14tensors/SAME authenticated file\n",(unsigned long long)original.Bytes(),original.Pages(),original.Spans());
 std::array<uint8_t,64> sentinel;sentinel.fill(0xa5);
 assert(!original.Read(~0U,0,sentinel.data(),64) && !original.Read(0,0,sentinel.data(),0) && !original.Read(0,0,nullptr,64));
 for(auto b:sentinel)assert(b==0xa5);
 auto machine=std::make_unique<OriginalRecurrentBlockMachine>(argv[1],argv[2],original);assert(machine->WaitBlock(0));
 machine->memory[RB::Doorbell]=1;assert(machine->WaitBlock(1));machine->VerifyBlock();assert(original.Ready());
 printf("PASS ORIGINAL whole recurrent layer0/8matrices720jobs+640Conv+96GDN+direct6144+RMS/residual/FFN/final5120/ROOTstack%u;supplied hidden,NOT embedding/native/physical/full64/model/TPS\n",0x70000-machine->minStack);
}
