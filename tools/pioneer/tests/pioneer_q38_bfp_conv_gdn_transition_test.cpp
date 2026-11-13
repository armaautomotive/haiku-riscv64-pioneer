/* SPDX-License-Identifier: MIT */
// Paired actual QGWF workers, one shared BFP/Conv/GDN state per worker.
// Synthetic GDN operands and modeled NoC; no device or throughput claim.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_BFP_GDN_TRANSITION_TEST 1
#define PIONEER_Q38_RECURRENT_GATE_QKV_HELPER_ONLY 1
#include "pioneer_q38_recurrent_gate_qkv_worker_bridge.cpp"
#include "PioneerQ38BfpGdnWorker.h"
int main(int argc,char** argv){
 assert(argc==7);
 std::array<std::vector<uint8_t>,2> projection;
 for(unsigned k=0;k<2;k++){std::ifstream f(argv[3+k],std::ios::binary);projection[k].assign(std::istreambuf_iterator<char>(f),{});assert(projection[k].size()==32+2*(160*1088+128));}
 std::ifstream lf(argv[5],std::ios::binary);std::vector<uint32_t> learned(41184);
 assert(lf.read(reinterpret_cast<char*>(learned.data()),learned.size()*4) && lf.peek()==EOF);
 std::ifstream qf(argv[6],std::ios::binary);std::vector<uint8_t> qb{std::istreambuf_iterator<char>(qf),{}};
 assert(qb.size()==32+320*(160*1088+128));std::vector<uint32_t> qkv(10240);
 for(unsigned b=0;b<320;b++)for(unsigned r=0;r<32;r++)std::memcpy(&qkv[b*32+r],qb.data()+32+size_t(b)*(160*1088+128)+160*1088+r*4,4);
 std::vector<uint8_t> input(5120*2);uint32_t raw[256],packed[128];
 for(unsigned chunk=0;chunk<20;chunk++){
  for(unsigned i=0;i<256;i++){unsigned at=chunk*256+i;raw[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  assert(PioneerQ38Bf16Input::Pack(raw,256,packed,128));std::memcpy(input.data()+chunk*512,packed,512);
 }
 auto a=std::make_unique<PioneerQ38BfpGdnWorker>(argv[1],argv[2],5120,2,1);
 auto b=std::make_unique<PioneerQ38BfpGdnWorker>(argv[1],argv[2],5120,2,2);a->peer=b.get();b->peer=a.get();
 for(auto* image:{a.get(),b.get()}){
  image->Boot();for(unsigned i=0;i<256;i++){unsigned at=(image->boot[3]-1)*256+i;image->input[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  image->StageProducer(10,1,false,0);image->Check(1);
  for(unsigned kind=0;kind<2;kind++){
   unsigned seq=kind+2;std::array<uint32_t,64> job{};job[0]=HW::Magic;job[1]=seq;job[2]=1;job[3]=image->boot[3];job[4]=image->boot[4];job[7]=2025;
   for(unsigned i=0;i<8;i++)job[8+i]=image->boot[8+i];for(unsigned i=0;i<16;i++)job[16+i]=image->transfer.job.words[i];
   job[20]=2;job[27]=seq;job[33]=64;job[34]=kind+10;job[35]=8;
   assert(HW::JobShape(job.data(),image->boot.data(),seq-1,0));std::vector<uint32_t> rows(64);std::vector<uint8_t> seen(64);
   Matrix(*image,job,projection[kind],input,rows,seen,nullptr);for(auto v:seen)assert(v==1);
  }
  image->steps=0;image->StageConv(4,qkv,learned);assert(image->FinishCombined(4));image->VerifyConv(4);image->StageGdn(5);
 }
 auto runPair=[&](unsigned sequence){unsigned rounds=0;
  while((!a->Done(sequence) && a->pc!=a->faultPC) || (!b->Done(sequence) && b->pc!=b->faultPC)){
   assert(++rounds<1000000);for(auto* image:{a.get(),b.get()})for(unsigned i=0;i<128 && !image->Done(sequence) && image->pc!=image->faultPC;i++)image->StepGdn();
  }
  for(auto* image:{a.get(),b.get()})if(image->pc==image->faultPC){
   std::fprintf(stderr,"GDN_FAULT sequence%u worker%u packets%u phase%u steps%u last",sequence,image->boot[3],image->gdnPackets,image->phase,image->steps);
   for(unsigned i=0;i<32;i++)std::fprintf(stderr," %x",image->trace[(image->traceIndex+i)%32]);std::fputc('\n',stderr);
  }
  assert(a->pc!=a->faultPC && b->pc!=b->faultPC);a->VerifyGdn(sequence);b->VerifyGdn(sequence);
 };
 runPair(5);
 for(auto* image:{a.get(),b.get()}){
  image->gdn=false;std::array<uint32_t,64> job{};job[0]=HW::Magic;job[1]=6;job[2]=1;
  job[3]=image->boot[3];job[4]=image->boot[4];job[7]=2025;
  for(unsigned i=0;i<8;i++)job[8+i]=image->boot[8+i];for(unsigned i=0;i<16;i++)job[16+i]=image->transfer.job.words[i];
  job[20]=2;job[27]=6;job[33]=64;job[34]=10;job[35]=8;
  assert(HW::JobShape(job.data(),image->boot.data(),5,0));std::vector<uint32_t> rows(64);std::vector<uint8_t> seen(64);
  Matrix(*image,job,projection[0],input,rows,seen,nullptr);for(auto v:seen)assert(v==1);
 }
 for(auto* image:{a.get(),b.get()}){
  // The real worker cannot advance to token position 1 until all 64 model
  // layers have closed. Exercise nonzero state at position 0 only; do not
  // fabricate the private model-layer frontier or call this two tokens.
  image->StageGdn(7,0,1,true);bool nonzero=false;for(auto bits:image->prior)nonzero|=(bits&0x7fffffff)!=0;assert(nonzero);
 }
 runPair(7);
 auto rejected=std::make_unique<PioneerQ38BfpGdnWorker>(argv[1],argv[2],5120,2,1);rejected->Boot();
 rejected->StageGdn(1);const unsigned bad=TT_Q38_GDN_INPUT_DESCRIPTOR+63*4;
 rejected->WordWrite(bad,rejected->WordRead(bad)^1);
 while(!rejected->Done(1) && rejected->pc!=rejected->faultPC)rejected->StepGdn();
 assert(rejected->pc==rejected->faultPC && !rejected->gdnPackets && !rejected->nextWrites && !rejected->gatedWrites);
 puts("PASS two retained actual QGWF workers: BF16 -> BFP8 alpha/beta -> Conv -> paired GDN -> BFP8 -> nonzero-state GDN; shared Dst/config/register/counters; synthetic GDN inputs; NO DEVICE/ROOT/WORK/SPEED");
}
