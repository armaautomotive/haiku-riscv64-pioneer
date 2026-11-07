/* SPDX-License-Identifier: MIT */
// Actual linked QGW8 Gather and seals; replay a real simulated QGW8 worker's
// proof/partial via synthetic NoC reads. Owner history is explicitly injected.
#define PIONEER_TENSIX_MACHINE_ONLY
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38HybridHeadLayout.h"
#include "../PioneerQ38TokenMetadata.h"
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38GlobalChainPackage.h"
#include <array>
#include <memory>
#include <string>

namespace C=PioneerQ38GlobalChainLayout;namespace R=C::R;namespace T=PioneerQ38TokenMetadata;
namespace H=PioneerQ38HybridHeadLayout;namespace W=PioneerQ38HybridHeadWorkerLayout;
struct GatherMachine:Machine {
 struct Private {uint32_t boot[64],outer[128],state[64],phase;};
 enum {Parent=0x6f800,Header=0x17e000,Best=0x17e400,Stack=0x6e000,Return=0x60004};
 std::vector<std::pair<unsigned,std::vector<uint8_t>>> rom;std::vector<uint32_t> memory;
 std::array<uint32_t,288> peer;std::array<uint32_t,10> command={};std::array<uint32_t,8> counters={};
 std::array<uint32_t,16> source={};unsigned packets=0,minSP=Parent,gather=0;
 static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
 uint32_t& Data(unsigned a){assert(!(a&3) && a>=Stack && a<0x180000);return memory[a/4];}
 GatherMachine(const char* image,const std::array<uint32_t,256>& fixture,const std::array<uint32_t,288>& observed):memory(0x180000/4),peer(observed){
  for(auto part:{std::make_pair("bf16",0x20000U),{"text",0x40000U},{"terminal",0x60000U},{"conv",0x64000U}})
   rom.push_back({part.second,Read((std::string(image)+"/"+part.first+".bin").c_str())});
  std::ifstream symbols(std::string(image)+"/program.demangled");std::string line;unsigned found=0;
  while(std::getline(symbols,line))if(line.find("GlobalChain::Gather(")!=std::string::npos){gather=unsigned(std::stoul(line.substr(0,8),nullptr,16));found++;}
  assert(found==1);
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
  const unsigned bank=0,workspace=canonical->Base+canonical->Workspace(bank)->offset;
  std::array<uint32_t,1920> sources={};for(unsigned w=0;w<120;w++)sources[w*16]=0x14000;
  auto package=std::make_unique<PioneerQ38GlobalChainPackage>();
  assert(package->Build(*canonical,63,bank,{1,2,3,4},sources.data(),bank,workspace+R::FinalOffset));
  Private owner={};std::copy(package->Boot(),package->Boot()+64,owner.boot);std::copy(package->Outer(),package->Outer()+128,owner.outer);
  assert(T::Outer(owner.outer,63,0,0,bank,workspace,owner.boot) && T::Frame(owner.outer,63,0,0,bank,workspace,owner.boot));
  owner.phase=4;owner.state[0]=bank;owner.state[1]=workspace;owner.state[2]=63;owner.state[3]=64;owner.state[6]=64;owner.state[28]=1;
  std::copy(owner.boot,owner.boot+64,memory.begin()+R::Boot/4);std::copy(owner.outer,owner.outer+128,memory.begin()+R::Outer/4);
  Data(R::Doorbell)=owner.outer[48];std::copy(owner.boot+16,owner.boot+32,source.begin());std::memcpy(memory.data()+Parent/4,&owner,sizeof(owner));
  std::array<uint32_t,128> head={};for(unsigned i=0;i<128;i++)head[i]=fixture[64+i];
  head[3]=bank;head[4]=workspace;head[5]=workspace+C::HeadOffset;
  for(unsigned i=0;i<16;i++)head[16+i]=owner.boot[16+i];for(unsigned i=0;i<8;i++)head[32+i]=owner.boot[8+i];
  assert(H::Shape(head.data(),owner.boot,bank,workspace));
  for(unsigned i=0;i<128;i++)Data(C::Head+i*4)=Data(Header+i*4)=head[i];
  for(unsigned i=0;i<64;i++){Data(R::PeerBoots+i*4)=peer[64+i];Data(R::PeerJobs+i*4)=peer[i];}
  for(unsigned i=0;i<8;i++)Data(R::Counters+i*4)=peer[256+i];Data(R::Nonces)=1;
  Data(Best)=248320;Data(Best+4)=0;Data(Best+8)=0;
  regs[1]=Return;regs[2]=Parent;regs[27]=Parent;regs[10]=Header;regs[11]=0;regs[12]=0;
  regs[13]=Best;regs[14]=Best+4;regs[15]=Best+8;pc=gather;
 }
 bool CodeAddress(unsigned a)const override{for(const auto& s:rom)if(a>=s.first && a-s.first<s.second.size())return true;return false;}
 unsigned Index(unsigned a)override{for(const auto& s:rom)if(a>=s.first && a-s.first<s.second.size()){l1[8190]=Word(s.second,a-s.first);return 8190;}assert(false);return 0;}
 uint32_t Load(uint32_t a)override{
  if(CodeAddress(a))return l1[Index(a)];if(a>=Stack && a<0x180000)return Data(a);
  if(a>=0xffb20000 && a<0xffb30000){const unsigned o=a-0xffb20000;
   if(o<40)return command[o/4];for(unsigned i=0;i<16;i++)if(o==Source(i))return source[i];
   for(unsigned i=0;i<8;i++)if(o==Counter(i))return counters[i];
   for(unsigned i=0;i<4;i++)if(o==0x40+i*0x800)return 0;
   if(o>=0x240 && o<0x2c0)return 0;
  }
  fprintf(stderr,"unmodeled ROOT read %08x pc %08x\n",a,pc);assert(false);return 0;
 }
 unsigned Peer(unsigned a)const{
  if(a==0xffb121b0)return 0x46800;if(a==0xffb13140)return 0x2004c;
  if(a>=W::Boot && a<W::Boot+256)return peer[64+(a-W::Boot)/4];
  if(a>=W::Job && a<W::Job+256)return peer[(a-W::Job)/4];
  if(a>=W::Status && a<W::Status+64)return peer[272+(a-W::Status)/4];
  if(a==W::Doorbell)return 1;
  if(a>=W::Partial && a<W::Partial+256)return peer[192+(a-W::Partial)/4];
  if(a>=0x83f00 && a<0x84000)return peer[128+(a-0x83f00)/4];
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+Source(i))return peer[64+16+i];
  for(unsigned i=0;i<8;i++)if(a==0xffb20000+Counter(i))return peer[264+i];
  fprintf(stderr,"unmodeled peer read %08x\n",a);assert(false);return 0;
 }
 void Submit(){
  const unsigned remote=command[0],local=command[3],bytes=command[8];
  assert(!command[1] && command[2]==W::Peer(1) && !command[4] && command[5]==129 && !command[6] && !command[7] && !command[9]
   && bytes && !(bytes&3) && bytes<=16384-(remote&31) && local==R::Scratch+(remote&31));
  for(unsigned i=0;i<bytes/4;i++)Data(local+i*4)=Peer(remote+i*4);
  for(unsigned i=0;i<8;i++)counters[i]+=i==1?(bytes+63)/64:(i==0 || i==2 || i==3 || i==6?1U:0U);
  packets++;
 }
 void Store(uint32_t a,uint32_t v)override{
  if(a>=Stack && a<0x180000){Data(a)=v;return;}
  if(a>=0xffb20000 && a<0xffb20028){command[(a-0xffb20000)/4]=v;return;}
  if(a==0xffb20040){assert(v==1);Submit();return;}
  fprintf(stderr,"unmodeled ROOT write %08x pc %08x\n",a,pc);assert(false);
 }
 unsigned StepLimit()const override{return 10000000;}
 bool Run(){while(pc!=Return && pc!=0x60000){Step();assert(regs[2]>=Stack && regs[2]<=Parent);minSP=std::min(minSP,regs[2]);}return pc==Return;}
};
int main(int argc,char** argv){
 assert(argc==4);std::array<uint32_t,256> metadata={};std::array<uint32_t,288> peer={};
 std::ifstream a(argv[2],std::ios::binary);a.read(reinterpret_cast<char*>(metadata.data()),sizeof(metadata));assert(a.gcount()==sizeof(metadata) && a.peek()==EOF);
 std::ifstream b(argv[3],std::ios::binary);b.read(reinterpret_cast<char*>(peer.data()),sizeof(peer));assert(b.gcount()==sizeof(peer) && b.peek()==EOF);
 {
  auto m=std::make_unique<GatherMachine>(argv[1],metadata,peer);assert(m->Run());
  assert(m->Data(GatherMachine::Best)==peer[192+8] && m->Data(GatherMachine::Best+4)==peer[192+9]
   && m->Data(GatherMachine::Best+8)==256 && m->packets>0);
  for(unsigned i=0;i<8;i++)assert(m->Data(R::Counters+i*4)==peer[264+i]);
 }
 for(unsigned damage:{128U+16,192U+7,272U+2}){
  auto corrupt=peer;corrupt[damage]^=1;auto m=std::make_unique<GatherMachine>(argv[1],metadata,corrupt);assert(!m->Run());
  assert(m->Data(GatherMachine::Best)==248320 && m->Data(GatherMachine::Best+8)==0);
  for(unsigned i=0;i<8;i++)assert(m->Data(R::Counters+i*4)==peer[256+i]);
 }
 puts("PASS actual QGW8 ROOT Gather: synthetic NoC replay of actual simulated worker 256-row receipt, counter rebasing and three failure refusals; injected owner history, NO complete lifecycle/model/device/TPS");
}
