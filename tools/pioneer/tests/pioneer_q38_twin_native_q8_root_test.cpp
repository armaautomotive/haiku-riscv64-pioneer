/* SPDX-License-Identifier: MIT */
// Run the real dispatch and receipt predicates from the linked v4 ROOT.
// Arguments and mailboxes below are synthetic; this is not a NoC round trip.
#define PIONEER_Q38_FLAT_MATRIX_EMITTED_MACHINE_ONLY
#include "pioneer_q38_flat_matrix_emitted_test.cpp"
#include "../PioneerQ38MixedProjectionRootLayout.h"
#include "../PioneerQ38TwinNativeQ8Wire.h"
namespace W=PioneerQ38TwinNativeQ8Wire;
namespace Layout=PioneerQ38MixedProjectionRootLayout;

struct RootWireMachine:GrammarMachine {
 enum { Part=Layout::PartsShadow, Scratch=Layout::Scratch, Mail=0x80200, Before=0x80300 };
 Words scratch={},receipt={};std::array<uint32_t,8> part={},counts={};
 RootWireMachine(const GrammarRom& rom,const Words& b,const Words& j):GrammarMachine(rom,1,b,j,0,0){}
 uint32_t Load(uint32_t at)override{
  if(at>=Part && at<Part+32)return part[(at-Part)/4];
  if(at>=Scratch && at<Scratch+256)return scratch[(at-Scratch)/4];
  if(at>=Mail && at<Mail+256)return receipt[(at-Mail)/4];
  if(at>=Before && at<Before+32)return counts[(at-Before)/4];
  return GrammarMachine::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  if(at>=Scratch && at<Scratch+256){assert(!(at&3));scratch[(at-Scratch)/4]=value;return;}
  GrammarMachine::Store(at,value);
 }
 Words Dispatch(unsigned entry,unsigned type,unsigned k,unsigned groups){
  boot.fill(0);boot[6]=2025;boot[14]=k;boot[15]=type;boot[40]=5;boot[42]=0;
  for(unsigned i=0;i<8;i++)boot[32+i]=job[8+i];
  part[0]=0x04001000;part[2]=groups;part[4]=786;scratch.fill(0xdeadbeef);
  pc=entry;regs[10]=Boot;regs[11]=0;regs[12]=1;
  while(pc!=Return)Step();
  assert(regs[2]==Top && !matrixInstructions && !inputWrites && !outputReads && !configWrites);
  return scratch;
 }
 void ReceiptFrame(){
  const unsigned header[]={Old::ReceiptMagic,64,boot[3],job[1],job[5],job[6],boot[4],Old::Root};
  for(unsigned i=0;i<8;i++)receipt[i]=header[i];
  for(unsigned i=0;i<32;i++)receipt[8+i]=job[i];
  for(unsigned i=0;i<16;i++)receipt[40+i]=boot[16+i];
  for(unsigned i=0;i<8;i++)receipt[56+i]=counts[i]=0xfffffff0U+i;
  receipt[62]=counts[6]=counts[2];
 }
 bool Receipt(unsigned entry){
  pc=entry;regs[10]=Mail;regs[11]=Boot;regs[12]=Job;regs[13]=Before;
  return Run();
 }
};

int main(int argc,char** argv){
 assert(argc==5);GrammarRom old(argv[1],argv[3]),next(argv[2],argv[4]);
 unsigned oldDispatch[2]={},newDispatch[2]={},receipt=0;
 std::ifstream oldIn(argv[3]),newIn(argv[4]);unsigned skipped=0;
 for(auto* file:{&oldIn,&newIn}) *file>>std::hex>>skipped>>skipped;
 for(unsigned i=0;i<2;i++)oldIn>>oldDispatch[i],newIn>>newDispatch[i];
 newIn>>receipt;assert(oldIn.good() && newIn.good() && next.Contains(receipt));
 unsigned cases=0,negative=0;
 for(unsigned root=0;root<2;root++)for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})
 for(unsigned type:{8U,12U,14U}){
  if(!root && type==14)continue; // recurrent projection ROOT does not dispatch Q6
  const auto [rawBoot,unused]=Seed(1);
  const auto a=RootWireMachine(old,rawBoot,unused).Dispatch(oldDispatch[root],type,k,groups);
  const auto b=RootWireMachine(next,rawBoot,unused).Dispatch(newDispatch[root],type,k,groups);
  Words expected={};
  const unsigned header[]={Old::Magic,1,1,1,Old::Peer(1),0,5,2025};
  const unsigned capsule[]={0x31483851,type,type==8?576U:type==12?2304U:3392U,0,0,k,0x04001000,groups,786,1,1};
  for(unsigned i=0;i<8;i++){expected[i]=header[i];expected[8+i]=rawBoot[8+i];}
  for(unsigned i=0;i<11;i++)expected[16+i]=capsule[i];
  assert(a==expected);expected[26]=type==8?2:1;assert(b==expected);
  auto boot=rawBoot;boot[W::BootIndex]=W::BootTag;
  assert(GrammarMachine(old,1,rawBoot,a,0,0).Run());
  assert(!GrammarMachine(old,1,boot,b,0,0).Run());
  assert(!GrammarMachine(next,1,rawBoot,b,0,0).Run());
  assert(GrammarMachine(next,1,boot,b,0,0).Run());
  RootWireMachine positive(next,boot,b);positive.ReceiptFrame();assert(positive.Receipt(receipt));cases++;
  for(unsigned field=0;field<64;field++){
   RootWireMachine changed(next,boot,b);changed.ReceiptFrame();changed.receipt[field]^=0x80000000;
   assert(!changed.Receipt(receipt));negative++;
  }
  for(unsigned wrong:{0U,1U,2U,3U})if(wrong!=b[26]){
   auto wrongJob=b;wrongJob[26]=wrong;
   RootWireMachine changed(next,boot,wrongJob);changed.ReceiptFrame();
   assert(!changed.Receipt(receipt));negative++;
  }
 }
 assert(cases==45 && negative==3015);
 printf("PASS emitted paired ROOT dispatch+receipt: %u exact jobs, %u corrupt receipts/mismatched modes; no live board, no model run\n",cases,negative);
}
