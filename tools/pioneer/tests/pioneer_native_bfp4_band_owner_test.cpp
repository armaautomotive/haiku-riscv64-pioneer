/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_BFP_BAND_OWNER_ONLY
#include "pioneer_native_bfp_band_owner_test.cpp"
#define PIONEER_NATIVE_BFP4_BAND_IMAGE_ONLY
#include "pioneer_q38_native_bfp4_band_image_test.cpp"
#include "../PioneerNativeBfp4BandGolden.h"

using Lease4=PioneerNativeBfp4BandLease;
static std::map<unsigned,std::vector<uint8_t>> images4;
template<unsigned K>struct Image4 {
 static constexpr unsigned Width=K,Phases=2;
 static inline uint32_t Boot=0;
 static uint32_t Word(unsigned index){const auto& code=images4.at(K);return index<code.size()/4?::Word(code,index*4):0;}
};
struct RoundedImage4:Band4RV32 {
 using Band4RV32::Band4RV32;
 void Instruction(uint32_t op)override{
  if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{Band4RV32::Instruction(op);});
  else Band4RV32::Instruction(op);
 }
};
struct IO4:IO {
 using IO::IO;
 void Execute(){
  RoundedImage4 image(directory.c_str(),width,false);
  assert(memory[0x6000/4]==image.l1[0]);
  for(unsigned i=0;i<1024;i++)assert(memory[(0x70000+i*4)/4]==(i<image.code.size()/4?::Word(image.code,i*4):0));
  for(unsigned i=0;i<8192/4;i++)image.l1[image.Index(0x6e000+i*4)]=memory[(0x6e000+i*4)/4];
  for(unsigned i=0;i<64;i++)image.l1[image.Index(0x130000+i*4)]=memory[(0x130000+i*4)/4];
  for(unsigned tile=0;tile<width/32;tile++)for(unsigned stream=0;stream<2;stream++){
   unsigned first=(stream?0x124000:0x90000)-16+tile*(stream?80:592),bytes=stream?80:592;
   for(unsigned b=0;b<bytes;b++)image.native.l1[first+b]=uint8_t(memory[(first+b)/4]>>(((first+b)%4)*8));
  }
  assert(image.Execute());
  for(unsigned i=0;i<64;i++)memory[(0x130000+i*4)/4]=image.l1[image.Index(0x130000+i*4)];
  for(unsigned i=0;i<3;i++)memory[(0x17ffc0+i*4)/4]=image.l1[image.Index(0x17ffc0+i*4)];
  for(unsigned i=0;i<8192/4;i++)memory[(0x6e000+i*4)/4]=image.l1[image.Index(0x6e000+i*4)];
  executed=true;
 }
 void BfpResetWrite(uint32_t v){
  Window();assert(v==0x87ffff || v==0x46800 || v==0x47800);if(++resets==dropReset)return;
  if(v==0x46800){
   assert(reset==0x87ffff && !started);started=true;reset=v;if(!timeout)Execute();
   if(faultMarker)memory[0x17ffc8/4]=0x4542464e;
   if(oldMarker)memory[0x17ffc8/4]=0x3342464e; // MUST reject prior BFP8 completion
   if(badCount)memory[0x17ffc4/4]^=1;
  }else{if(started)assert(executed && !timeout && !badPC && !sourceChanged && !faultMarker && !oldMarker && !badCount);reset=v;}
 }
};
template<unsigned K>static void NoMore4(Lease4& lease,IO4& io,tt_dram_samples_result& baseline){
 auto reads=io.reads,writes=io.writes,resets=io.resets,tlb=io.tlbWrites;
 assert((!lease.End(io,baseline) && !lease.Run<IO4,Image4<K>>(io,baseline,K)));lease.Abandon();
 assert(io.reads==reads && io.writes==writes && io.resets==resets && io.tlbWrites==tlb);
}
template<unsigned K>static void Prepare4(const char* folder){
 std::string root=folder;images4[K]=::Read((root+"/text.bin").c_str());auto boot=::Read((root+"/boot.bin").c_str());
 assert(boot.size()==4 && !images4[K].empty() && images4[K].size()<=4096 && !(images4[K].size()&3));Image4<K>::Boot=::Word(boot,0);
 std::set<unsigned> addresses;for(unsigned i=0;i<Lease4::SavedWords(K);i++)assert(addresses.insert(Lease4::Address(K,i)).second);
 assert(addresses.size()==Lease4::SavedWords(K) && Lease4::Address(K,Lease4::SavedWords(K))==UINT32_MAX);
}
template<unsigned K>static void Positive4(const char* path){
 IO4 io(K,path);auto baseline=io.Baseline();auto lease=std::make_unique<Lease4>();
 assert((lease->Run<IO4,Image4<K>>(io,baseline,K)));auto result=lease->Result();const auto* golden=PioneerNativeBfp4BandGolden(K);assert(golden);
 assert(result.state==PIONEER_NATIVE_BFP_RESULT && result.started && result.completed && !result.cold && io.resets==2);
 for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)assert(result.output[pass*32+row]==golden[row]);
 assert(lease->End(io,baseline) && lease->Result().state==PIONEER_NATIVE_BFP_CLOSED && io.memory==io.original && io.resets==4);
 for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)assert(io.tlb[s][w]==s*3+w+1);
 NoMore4<K>(*lease,io,baseline);
 printf("PASS OFFBOARD BFP4 owner K%u: actual RV32 image, 64 predicted words, all snapshot L1/TLB restored; NO device/speed claim\n",K);
}
#ifndef PIONEER_NATIVE_BFP4_BAND_OWNER_ONLY
int main(int argc,char** argv){
 assert(argc==4);Prepare4<5120>(argv[1]);Prepare4<6144>(argv[2]);Prepare4<17408>(argv[3]);
 Positive4<5120>(argv[1]);Positive4<6144>(argv[2]);Positive4<17408>(argv[3]);unsigned faults=0;
 for(unsigned fault=0;fault<14;fault++){
  IO4 io(5120,argv[1]);auto baseline=io.Baseline();auto lease=std::make_unique<Lease4>();
  if(fault==0)io.timeout=true;if(fault==1)io.badPC=true;if(fault==2)io.faultMarker=true;if(fault==3)io.sourceChanged=true;
  if(fault==4)io.dropL1=1024;if(fault==5)io.dropReset=1;if(fault==6)io.dropReset=2;if(fault==7)io.dropTlb=2;
  if(fault==12)io.oldMarker=true;if(fault==13)io.badCount=true;
  bool ran=lease->Run<IO4,Image4<5120>>(io,baseline,5120);
  if(fault>=8 && fault<12){
   assert(ran);if(fault==8)io.dropReset=3;if(fault==9)io.dropReset=4;if(fault==10)io.failRestore=true;if(fault==11)io.dropTlb=io.tlbWrites+370;
   assert(!lease->End(io,baseline));
  }else assert(!ran);
  assert(lease->Result().state==PIONEER_NATIVE_BFP_UNKNOWN && lease->Result().cold);
  if(fault<8 || fault>=12)assert(io.resets<3);NoMore4<5120>(*lease,io,baseline);faults++;
 }
 for(unsigned bad:{0U,32U,5119U,6144U,17408U,UINT32_MAX}){
  IO4 io(5120,argv[1]);auto baseline=io.Baseline();auto lease=std::make_unique<Lease4>();
  assert((!lease->Run<IO4,Image4<5120>>(io,baseline,bad)) && !io.reads && !io.writes && !io.tlbWrites && !io.resets);faults++;
 }
 printf("PASS OFFBOARD BFP4 owner %u failure/no-retry cases including old BFP8 marker, unknown completion and restoration; NO hardware authorization\n",faults);
}
#endif
