/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_BFP4_BAND_OWNER_ONLY
#include "pioneer_native_bfp4_band_owner_test.cpp"
using TimedLease=PioneerNativeBfp4TimedLease;
struct RoundedTimedImage:Band4RV32 {
 bool unstable=false;unsigned reads=0;
 RoundedTimedImage(const char* folder,unsigned width):Band4RV32(folder,width,false){expectedMarker=0x5442464e;}
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(csr==0xb00 || csr==0xb80);reads++;
  if(unstable && csr==0xb80)return reads;
  uint64_t clock=UINT64_C(0xffff0000)+uint64_t(steps)*37;
  return csr==0xb00?uint32_t(clock):uint32_t(clock>>32);
 }
 void Instruction(uint32_t op)override{
  if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{Band4RV32::Instruction(op);});
  else Band4RV32::Instruction(op);
 }
};
struct TimedIO:IO4 {
 using IO4::IO4;
 bool badClock=false,oldCompletion=false,badTiming=false;
 void Execute(){
  RoundedTimedImage image(directory.c_str(),width);image.unstable=badClock;
  assert(memory[0x6000/4]==image.l1[0]);
  for(unsigned i=0;i<1024;i++)assert(memory[(0x70000+i*4)/4]==(i<image.code.size()/4?::Word(image.code,i*4):0));
  for(unsigned i=0;i<8192/4;i++)image.l1[image.Index(0x6e000+i*4)]=memory[(0x6e000+i*4)/4];
  for(unsigned i=0;i<64;i++)image.l1[image.Index(0x130000+i*4)]=memory[(0x130000+i*4)/4];
  for(unsigned tile=0;tile<width/32;tile++)for(unsigned stream=0;stream<2;stream++){
   unsigned first=(stream?0x124000:0x90000)-16+tile*(stream?80:592),bytes=stream?80:592;
   for(unsigned b=0;b<bytes;b++)image.native.l1[first+b]=uint8_t(memory[(first+b)/4]>>(((first+b)%4)*8));
  }
  assert(image.Execute() && image.minSP>=0x6e040);
  for(unsigned i=0;i<64;i++)memory[(0x130000+i*4)/4]=image.l1[image.Index(0x130000+i*4)];
  for(unsigned i=0;i<3;i++)memory[(0x17ffc0+i*4)/4]=image.l1[image.Index(0x17ffc0+i*4)];
  for(unsigned i=0;i<8192/4;i++)memory[(0x6e000+i*4)/4]=image.l1[image.Index(0x6e000+i*4)];
  if(oldCompletion)memory[0x17ffc8/4]=0x3442464e;
  if(badTiming)memory[(0x6e000+8)/4]^=1;
  executed=true;
 }
 void BfpResetWrite(uint32_t v){
  Window();assert(v==0x87ffff || v==0x46800 || v==0x47800);if(++resets==dropReset)return;
  if(v==0x46800){assert(reset==0x87ffff && !started);started=true;reset=v;if(!timeout)Execute();
   if(faultMarker)memory[0x17ffc8/4]=0x4542464e;if(badCount)memory[0x17ffc4/4]^=1;
  }else{if(started)assert(executed && !timeout && !badPC && !sourceChanged && !faultMarker && !oldCompletion && !badCount && !badClock && !badTiming);reset=v;}
 }
};
template<unsigned K>static void NoMoreTimed(TimedLease& lease,TimedIO& io,tt_dram_samples_result& baseline){
 auto reads=io.reads,writes=io.writes,resets=io.resets,tlb=io.tlbWrites;
 assert((!lease.End(io,baseline) && !lease.Run<TimedIO,Image4<K>>(io,baseline,K)));lease.Abandon();
 assert(io.reads==reads && io.writes==writes && io.resets==resets && io.tlbWrites==tlb);
}
template<unsigned K>static void PositiveTimed(const char* path){
 TimedIO io(K,path);auto baseline=io.Baseline();auto lease=std::make_unique<TimedLease>();
 assert((lease->Run<TimedIO,Image4<K>>(io,baseline,K)));const auto result=lease->Result();const auto* golden=PioneerNativeBfp4BandGolden(K);
 assert(result.state==PIONEER_NATIVE_BFP_RESULT && !result.cold && result.completed && golden);
 for(unsigned pass=0;pass<2;pass++){
  assert(PioneerNativeBfp4BandTiming::State(lease->Timing()+pass*8,K,pass)==1);
  for(unsigned row=0;row<32;row++)assert(result.output[pass*32+row]==golden[row]);
 }
 assert(lease->End(io,baseline) && io.memory==io.original && lease->Result().state==PIONEER_NATIVE_BFP_CLOSED);
 NoMoreTimed<K>(*lease,io,baseline);printf("PASS timed BFP4 owner K%u: 64 rounded outputs, valid cycle records, full L1/TLB restoration; synthetic clock only\n",K);
}
int main(int argc,char** argv){
 assert(argc==4);Prepare4<5120>(argv[1]);Prepare4<6144>(argv[2]);Prepare4<17408>(argv[3]);
 PositiveTimed<5120>(argv[1]);PositiveTimed<6144>(argv[2]);PositiveTimed<17408>(argv[3]);
 for(unsigned fault=0;fault<5;fault++){
  TimedIO io(5120,argv[1]);auto baseline=io.Baseline();auto lease=std::make_unique<TimedLease>();
  if(fault==0)io.badClock=true;if(fault==1)io.oldCompletion=true;if(fault==2)io.badTiming=true;if(fault==3)io.badPC=true;
  bool ran=lease->Run<TimedIO,Image4<5120>>(io,baseline,5120);
  if(fault==4){assert(ran);io.memory[0x6e000/4]^=1;assert(!lease->End(io,baseline));}
  else assert(!ran);
  assert(lease->Result().state==PIONEER_NATIVE_BFP_UNKNOWN && lease->Result().cold && io.resets==2);
  NoMoreTimed<5120>(*lease,io,baseline);
 }
 puts("PASS timed BFP4 owner rejects missing/altered clock, wrong marker, bad PC and altered END timing; NO device/cleanup/retry");
}
