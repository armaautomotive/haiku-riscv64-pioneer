/* SPDX-License-Identifier: MIT */
// Actual QGW4 frontend CALLS the linked returning Q4 FPU capsule. No PC
// shortcut or fake product. ROOT producer/coordinator integration is NOT
// claimed:the separate actual packing image provides this test's inputs.
#define PIONEER_CACHED_FPU_IMAGE_ONLY
#include "pioneer_q38_cached_fpu_image_test.cpp"
#include "../PioneerQ38Bf16WorkerLayout.h"
#include "pioneer_q38_paired_projection_probe.h"
namespace BW=PioneerQ38Bf16WorkerLayout;
struct FullFpuPeer:CachedMatrixRV32 {
 std::vector<std::pair<unsigned,std::vector<uint8_t>>> rom;
 unsigned waitPC=0,stopPC=0,faultPC=0,seal=0;std::array<uint32_t,64> boot={};std::array<uint32_t,16> source={};
 FullFpuPeer(const char* image,const char* leaf,CachedPipeline& engine,const CL::Job& job,const std::vector<uint8_t>& root)
  :CachedMatrixRV32(leaf,engine,job,root){
  for(auto part:{std::make_pair("boot",0x6000U),{"globalworker",0x20000U},{"bf16producer",0x30000U},
   {"dense",0x40000U},{"cachedfpu",0x50000U},{"ffn",0x60000U},{"service",0x68000U},{"affine",0x70b00U},{"text",0x72000U},{"gdn",0xb8000U}})
   rom.push_back({part.second,Read((std::string(image)+"/"+part.first+".bin").c_str())});
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string address,kind,name;
  while(symbols>>address>>kind>>name){unsigned a=unsigned(std::stoul(address,nullptr,16));if(name=="block_peer_wait_load")waitPC=a;if(name=="block_peer_stopped")stopPC=a;if(name=="block_fault_stopped")faultPC=a;}
  assert(waitPC && stopPC && faultPC);regs[27]=0;
  // Initial cold NIU return-coordinate fixture,BEFORE the very first
  // instruction. Never normalized or replaced at a job transition.
  bus.command.fill(0);bus.command[5]=job.words[7];
  source[0]=0x14000;for(unsigned i=0;i<3;i++)for(unsigned lane=0;lane<6;lane++)source[1+i]|=(i*6+lane)<<(lane*5);
  for(unsigned i=0;i<2;i++)for(unsigned lane=0;lane<6;lane++)source[7+i]|=(i*6+lane)<<(lane*5);
  boot[0]=BW::Magic;boot[1]=1;boot[2]=64;boot[3]=job.words[6];boot[4]=job.words[7];boot[5]=source[0];boot[6]=8192;boot[7]=64;
  for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;for(unsigned i=0;i<16;i++)boot[16+i]=source[i];
  for(unsigned i=40;i<49;i++)boot[i]=BW::BootWord(i);assert(BW::BootShape(boot.data()));
  for(unsigned i=0;i<64;i++){Put(BW::Boot+i*4,boot[i]);Put(BW::Job+i*4,0);}Put(BW::Doorbell,0);
  for(unsigned i=0;i<16;i++)Put(BW::Status+i*4,0xdeadbeef);
  l1[0]=::Word(rom[0].second,0);
 }
 const std::vector<uint8_t>* Rom(unsigned a,unsigned& base)const{
  for(const auto& p:rom)if(Range(a,p.first,unsigned(p.second.size()))){base=p.first;return &p.second;}return nullptr;
 }
 bool CodeAddress(unsigned a)const override{unsigned base=0;return Rom(a,base);}
 unsigned Index(unsigned a)override{unsigned base=0;if(auto* p=Rom(a,base)){assert(!(a&3));l1[8190]=::Word(*p,a-base);return 8190;}return ServiceMachine::Index(a);}
 unsigned Source(unsigned i)const{return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 unsigned Load(unsigned a)override{
  unsigned base=0;if(auto* p=Rom(a,base))return ::Word(*p,a-base);
  if(Range(a,BW::Boot,512) || a==BW::Doorbell || Range(a,BW::Status,64) || a==0x74100)return Get(a);
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+Source(i))return source[i];
  return CachedMatrixRV32::Load(a);
 }
 void Store(unsigned a,unsigned value)override{
  if(Range(a,BW::Status,64) || Range(a,0x74000,96) || a==0x74100 || Range(a,0x83f00,256)){Put(a,value);return;}
  CachedMatrixRV32::Store(a,value);
 }
 void StepWorker(){
  const unsigned instruction=l1[Index(pc)];Step();if(instruction==0x0ff0000f)FenceObserved();Progress();
  if(regs[27]){if(!seal)seal=regs[27];assert(regs[27]==seal);}
  if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}
 }
 void Boot(){while(pc!=waitPC && pc!=faultPC)StepWorker();assert(pc==waitPC && seal && Get(BW::Status+12)==BW::Magic && !Get(BW::Status+8));}
 void Stage(const CL::Job& job,unsigned phase,unsigned firstRow){
  assert(pc==waitPC && native.Clean() && !awaiting && bus.queued.empty() && bus.responses.empty());
  if(job.words[11]>bus.job.words[11]){bus.Select(job);native.blocks=job.words[2]/256;native.inputBlockBase=native.inputLoads/8;}
  std::array<uint32_t,64> outer={};const unsigned h[]={BW::Magic,job.words[11],1,job.words[6],job.words[7],0,0,2025};
  for(unsigned i=0;i<8;i++){outer[i]=h[i];outer[8+i]=boot[8+i];}
  for(unsigned i=0;i<16;i++)outer[16+i]=job.words[i];
  outer[32]=firstRow;outer[33]=job.words[2]==5120?17408:5120;outer[34]=phase;outer[35]=12;
  assert(BW::JobShape(outer.data(),boot.data(),job.words[11]-1,0));
  for(unsigned i=0;i<64;i++)Put(BW::Job+i*4,outer[i]);Put(BW::Doorbell,job.words[11]);
 }
 void Finish(unsigned request){
  while(pc!=faultPC && !(pc==waitPC && Get(BW::Status+4)==request))StepWorker();
  assert(pc==waitPC && !Get(BW::Status+8) && native.Clean() && !awaiting && bus.reader==2 && bus.queued.empty() && bus.responses.empty());
 }
 void Stop(unsigned request,unsigned layer){
  std::array<uint32_t,64> j={};const unsigned h[]={BW::Magic,request,BW::Stop,boot[3],boot[4],layer,0,2026};
  for(unsigned i=0;i<8;i++){j[i]=h[i];j[8+i]=boot[8+i];}assert(BW::JobShape(j.data(),boot.data(),request-1,0));
  for(unsigned i=0;i<64;i++)Put(BW::Job+i*4,j[i]);Put(BW::Doorbell,request);
  while(pc!=stopPC && pc!=faultPC)StepWorker();assert(pc==stopPC && Get(BW::Status+4)==request);
 }
};
int main(int argc,char** argv){
 assert(argc==4);unsigned outputs=0,jobs=0,maxStack=0;
 const unsigned metadataJobs=ProbePairedCoverage();
 assert(metadataJobs==22848);
 for(unsigned worker:{1U,119U}){
  std::vector<uint8_t> rootBytes(0x180000,0xa7);auto root=std::make_unique<ServiceMachine>(argv[2],rootBytes);
  auto engine=std::make_unique<CachedPipeline>();std::fill(engine->l1.begin(),engine->l1.end(),0xa7);
  PioneerPairedPartProbe first;assert(ProbePairedPart(0,2,worker,first));
  auto job=Job(5120,first.groups,worker,PioneerQ38ProjectionRoute(first.bank));job.words[3]=first.address;job.words[11]=1;
  auto peer=std::make_unique<FullFpuPeer>(argv[1],argv[3],*engine,job,rootBytes);peer->Boot();
  unsigned sequence=0;
  for(unsigned phase:{2U,3U,4U}){
   if(phase==4 && worker<=68)continue;const unsigned width=phase==4?17408:5120;
   PioneerPairedPartProbe part;assert(ProbePairedPart(0,phase,worker,part));
   job=Job(width,part.groups,worker,PioneerQ38ProjectionRoute(part.bank));job.words[3]=part.address;
   job.words[11]=++sequence;CachedData data(width,job.words[4],0);
   const unsigned frame[]={PioneerQ38Bf16Input::Magic,1,width,0xb0000,0xa0000,sequence,0,0,0,0,0,0,0,0,0,0};
   for(unsigned i=0;i<16;i++)root->Put(0x74000+i*4,frame[i]);data.StageFp32(*root);
   if(sequence>1){root->pc=0x6000;for(unsigned i=0;i<3;i++)root->Put(0x17ffc0+i*4,0);}
   assert(root->Execute(PioneerQ38Bf16Input::Magic));
   const auto before=peer->bus.counters;peer->Stage(job,phase,part.firstRow);
   peer->bus.dramBase=job.words[3];data.StageCache(peer->bus);peer->Finish(sequence);
   for(unsigned pair=0;pair<job.words[4]/2;pair++)for(unsigned row=0;row<32;row++){
    assert(peer->Get(CL::Output+(pair*32+row)*4)==data.expected[pair][row]);outputs++;
   }
   const unsigned reads=job.words[4]/2*(width/256)*26,flits=job.words[4]/2*(width/256)*80;
   const unsigned indices[]={2,3,4,5,1,8,4,10};
   for(unsigned i=0;i<8;i++){
    const unsigned expected=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
    assert(peer->Get(0x83f00+i*4)==before[indices[i]] && uint32_t(peer->Get(0x83f20+i*4)-before[indices[i]])==expected);
   }
   assert(peer->Get(0x83f40)==job.words[4]*(width/16) && peer->Get(0x83f44)==sequence && peer->Get(0x83f48)==worker
    && peer->Get(0x83f4c)==CL::Magic && peer->Get(0x83fc0)==BW::Producer::Format && peer->Get(0x83fd8)==part.firstRow);
   maxStack=std::max(maxStack,0x70000-peer->minSP);jobs++;
   printf("PASS ACTUAL full QGW4->linked returning FPU worker%u phase%u K%u rows%u all exact;actual sealed dispatcher,canonical NoC bytes,shared live FPU/NIU/L1 across requests\n",worker,phase,width,job.words[4]*16);fflush(stdout);
  }
  peer->Stop(sequence+1,worker==119?1:0);
 }
 assert(jobs==5 && outputs && maxStack<=8192);
 printf("PASS ACTUAL QGW4/FPU dispatch jobs%u/outputs%u/maxCombinedStack%u/canonical stripe metadata jobs%u/all64layers/real WAIT and STOP;separate actual coordinator pack image supplies typed inputs,synthetic bytes at REAL unchanged stripe addresses;NOT connected live producer/ROOT/native/hardware/full-model quality/TPS proof\n",jobs,outputs,maxStack,metadataJobs);
}
