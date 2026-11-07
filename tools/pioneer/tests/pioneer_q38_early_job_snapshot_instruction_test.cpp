/* SPDX-License-Identifier: MIT */
// Execute the actual two compiled Gather-check slices with synthetic L1 and
// registers. This is not a complete controller, GPU or performance test.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38PushedWorkerLayout.h"
#include <array>
#include <initializer_list>
#include <string>

namespace M = PioneerQ38PushedWorkerLayout;
struct SnapshotSlice : Machine {
 std::vector<uint8_t> text;
 std::vector<uint32_t> mem;
 bool dense;
 bool dispatch=false;
 unsigned dispatchJob=Job;
 static constexpr unsigned Header=0x76000, Stack=0x70000, Part=0x77000, Nonce=0x123000, Job=0x178000;
 explicit SnapshotSlice(const char* path, bool denseComponent)
  : text(Read((std::string(path)+"/text.bin").c_str())), mem(0x180000/4), dense(denseComponent) {}
 bool CodeAddress(uint32_t at) const override {return at>=0x40000 && at-0x40000<text.size();}
 unsigned Index(uint32_t at) override {
  assert(!(at&3) && CodeAddress(at)); l1[8180]=Word(text,at-0x40000); return 8180;
 }
 uint32_t Load(uint32_t at) override {
  assert(!(at&3));
  if(CodeAddress(at))return Word(text,at-0x40000);
  assert(at < 0x180000);return mem[at/4];
 }
 void Store(uint32_t at,uint32_t value) override {
  assert(!(at&3) && ((at>=Stack-2048 && at<Stack)
   || (dispatch && ((at>=0x78000 && at<0x78100) || (at>=dispatchJob && at<dispatchJob+256)))));
  mem[at/4]=value;
 }
 unsigned StepLimit() const override {return 10000;}
 void Prepare(unsigned w,unsigned type,unsigned sequence) {
  assert(w<119 && (type==8 || type==12 || (dense && type==14)));
  std::fill(regs,regs+32,0);steps=0;regs[8]=Stack;
  const unsigned descriptor=Header, part=Part+w*32, job=Job+w*256;
  for(unsigned i=0;i<64;i++){mem[(descriptor+4*i)/4]=0;mem[(job+4*i)/4]=0;}
  for(unsigned i=0;i<8;i++)mem[(part+4*i)/4]=0x4000000+w*0x10000+i*64;
  mem[(part+8)/4]=1+w%16;mem[(Nonce+4*w)/4]=sequence;
  auto d=[&](unsigned i)->uint32_t&{return mem[(descriptor+4*i)/4];};
  auto j=[&](unsigned i)->uint32_t&{return mem[(job+4*i)/4];};
  d(6)=31;d(14)=type==8?5120:17408;d(15)=type;d(40)=7;d(42)=5;
  for(unsigned i=0;i<8;i++)d(32+i)=0x51000000+i;
  j(0)=M::Magic;j(1)=sequence;j(2)=1;j(3)=w+1;j(4)=M::Peer(w+1);
  j(5)=d(42);j(6)=d(40);j(7)=d(6);
  for(unsigned i=0;i<8;i++)j(8+i)=d(32+i);
  j(16)=0x31483851;j(17)=type;j(18)=type==8?576:type==12?2304:3392;
  j(21)=d(14);j(22)=mem[part/4];j(23)=mem[(part+8)/4];j(24)=mem[(part+16)/4];j(25)=j(26)=1;
  mem[(Stack-788)/4]=4*w;
  if(dense){
   pc=0x53288;regs[25]=descriptor;regs[21]=w;regs[5]=0x170000+w*256;
   mem[(Stack-792)/4]=uint32_t(-(Stack-704)+w*32);mem[(Stack-860)/4]=0x5f93c;
  }else{
   pc=0x51e18;regs[24]=descriptor;regs[25]=w;regs[16]=0x170000+w*256;
   mem[(Stack-800)/4]=uint32_t(-(Stack-704)+w*32);mem[(Stack-860)/4]=M::Magic;
  }
 }
 bool RunSlice() {
  const unsigned success=dense?0x5347c:0x51fdc, fault=dense?0x53324:0x51eb4;
  while(pc!=success && pc!=fault)Step();
  assert(!matrixInstructions && !configWrites && !drains && !inputWrites && !outputReads);
  return pc==success;
 }
 void RunDispatch(unsigned w,unsigned type,unsigned sequence){
  Prepare(w,type,sequence);std::array<uint32_t,64> expected;dispatchJob=Job+w*256;
  for(unsigned i=0;i<64;i++){
   expected[i]=mem[(dispatchJob+4*i)/4];mem[(dispatchJob+4*i)/4]=0xa5a50000+i;
   mem[(0x78000+4*i)/4]=0x5a5a0000+i;
  }
  mem[(Nonce+4*w)/4]=sequence-1;
  std::fill(regs,regs+32,0);steps=0;regs[8]=Stack;regs[18]=Stack-560;
  // Synthetic LOOP-entry register state, not an emulated earlier admission.
  regs[12]=w;regs[20]=Nonce+4*w;regs[26]=1;
  if(dense){
   pc=0x5300c;regs[21]=0x78100;regs[23]=Stack-264;regs[24]=0x78040-(Stack-304);
   regs[25]=Header;regs[28]=w*256;regs[16]=Part+w*32;regs[17]=0xfff88080;
   mem[(Stack-804)/4]=M::Magic;mem[(Stack-808)/4]=0x5f93c;
  }else{
   pc=0x51ba4;regs[19]=0x78100;regs[21]=Stack-264;regs[23]=w*256;regs[25]=Header;
   regs[30]=Part+w*32;regs[29]=0x78020;regs[17]=0x78040-(Stack-304);regs[16]=0xfff88080;
   mem[(Stack-808)/4]=M::Magic;mem[(Stack-812)/4]=0x31483851;
  }
  dispatch=true;const unsigned end=dense?0x53170:0x51cf4;
  while(pc!=end)Step();dispatch=false;
  for(unsigned i=0;i<64;i++)assert(mem[(dispatchJob+4*i)/4]==expected[i] && mem[(0x78000+4*i)/4]==expected[i]);
  assert(mem[(Nonce+4*w)/4]==sequence-1 && !matrixInstructions && !configWrites && !drains && !inputWrites && !outputReads);
 }
 unsigned RunOriginalSlices(unsigned w,unsigned type,unsigned sequence){
  Prepare(w,type,sequence);std::array<uint32_t,64> expected;dispatchJob=Job+w*256;
  for(unsigned i=0;i<64;i++)expected[i]=mem[(dispatchJob+4*i)/4];
  mem[(Nonce+4*w)/4]=sequence-1;
  std::fill(regs,regs+32,0);steps=0;regs[2]=Stack-1024;regs[8]=Stack;
  if(dense){pc=0x53004;regs[18]=Header;regs[21]=w;regs[22]=Nonce+4*w;}
  else{pc=0x51e84;regs[24]=Header;regs[19]=w;regs[20]=Nonce+4*w;}
  dispatch=true;const unsigned dispatchEnd=dense?0x5301c:0x51e9c;
  while(pc!=dispatchEnd)Step();
  for(unsigned i=0;i<64;i++)assert(mem[(0x78000+4*i)/4]==expected[i]);
  const unsigned dispatchSteps=steps;mem[(Nonce+4*w)/4]=sequence;
  std::fill(regs,regs+32,0);steps=0;regs[2]=Stack-1024;regs[8]=Stack;
  if(dense){
   pc=0x530f0;regs[26]=Header;regs[24]=w;regs[29]=Nonce+4*w;regs[6]=uint32_t(-(Stack-304));
   mem[(Stack-788)/4]=w*256;
  }else{
   pc=0x51f60;regs[24]=Header;regs[26]=w;
   mem[(Stack-788)/4]=Nonce+4*w;mem[(Stack-792)/4]=w*256;
  }
  const unsigned gatherEnd=dense?0x53140:0x51fa4;
  while(pc!=gatherEnd)Step();dispatch=false;
  for(unsigned i=0;i<64;i++)assert(mem[(dispatchJob+4*i)/4]==expected[i]);
  return dispatchSteps+steps;
 }
};
int main(int argc,char** argv){
 assert(argc==3);unsigned passed=0,refused=0,dispatches=0,faster=0;uint64_t oldTotal=0,newTotal=0;
 int minimumSaved=1000000,maximumSaved=-1000000;
 for(bool dense:{false,true}){
  SnapshotSlice m(argv[1],dense),old(argv[2],dense);
  for(unsigned w=0;w<119;w++)for(unsigned type:{8U,12U,14U}){
   if(!dense && type==14)continue;
   for(unsigned sequence:{1U,13U,0x7ffffffeU}){
    m.RunDispatch(w,type,sequence);const unsigned dispatchSteps=m.steps;++dispatches;
    m.Prepare(w,type,sequence);assert(m.RunSlice());
    const unsigned candidateSteps=dispatchSteps+m.steps;
    const unsigned originalSteps=old.RunOriginalSlices(w,type,sequence);
    const int saved=int(originalSteps)-int(candidateSteps);
    minimumSaved=std::min(minimumSaved,saved);maximumSaved=std::max(maximumSaved,saved);
    if(saved>0)++faster;
    oldTotal+=originalSteps;newTotal+=candidateSteps;++passed;
    for(unsigned i=0;i<27;i++){
     m.Prepare(w,type,sequence);m.mem[(m.Job+w*256+4*i)/4]^=1;assert(!m.RunSlice());++refused;
    }
    for(unsigned i:{6U,14U,15U,32U,33U,34U,35U,36U,37U,38U,39U,40U,42U}){
     m.Prepare(w,type,sequence);m.mem[(m.Header+4*i)/4]^=1;assert(!m.RunSlice());++refused;
    }
    for(unsigned i:{0U,2U,4U}){
     m.Prepare(w,type,sequence);m.mem[(m.Part+w*32+4*i)/4]^=1;assert(!m.RunSlice());++refused;
    }
    m.Prepare(w,type,sequence);m.mem[(m.Nonce+4*w)/4]^=1;assert(!m.RunSlice());++refused;
   }
  }
 }
 printf("PASS actual RV32 slices: %u dispatch snapshots, %u valid and %u corrupted Gather fixtures; synthetic L1/initial registers; NO device, receipt, math or speed qualification\n",dispatches,passed,refused);
 printf("Synthetic slice instructions old%llu candidate%llu; candidate shorter %u/%u, savings %d..%d instructions per case (no NoC, wait, full controller or hardware timing)\n",
  static_cast<unsigned long long>(oldTotal),static_cast<unsigned long long>(newTotal),faster,passed,minimumSaved,maximumSaved);
}
