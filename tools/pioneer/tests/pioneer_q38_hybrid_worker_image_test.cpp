/* SPDX-License-Identifier: MIT */
// Actual experimental worker + linked BFP4/BFP8 leaf instructions, simulated
// NIU/FPU/GDDR. No live ROOT, resource ownership, hardware or model claim.
#ifdef PIONEER_TEST_BFP8
#define PIONEER_BFP8_WORKER_IMAGE_ONLY
#include "pioneer_q38_bfp8_worker_leaf_image_test.cpp"
static constexpr unsigned kBits=8;
#else
#define PIONEER_BFP4_WORKER_IMAGE_ONLY
#include "pioneer_q38_bfp4_worker_leaf_image_test.cpp"
static constexpr unsigned kBits=4;
#endif
#include "../PioneerQ38HybridWorkerLayout.h"
#include "../PioneerQ38HybridMatrixReceipt.h"
#ifdef PIONEER_Q38_TEST_LINKED_RECEIPT
#include "pioneer_q38_hybrid_linked_receipt.h"
#endif
#define PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC 1
#include "../PioneerQ38HybridTokenFootprint.h"
#include <array>
namespace HW=PioneerQ38HybridWorkerLayout;
struct HybridImage:WorkerImage {
 std::vector<std::pair<unsigned,std::vector<uint8_t>>> rom;
 std::array<uint32_t,64> boot={};std::array<uint32_t,16> source={};
 unsigned waitPC=0,faultPC=0,stopPC=0,seal=0,trace[32]={},traceIndex=0;
 HybridImage(const char* worker,const char* leaf,unsigned peer,unsigned width,unsigned bands=2)
#ifdef PIONEER_TEST_BFP8
  :WorkerImage(leaf,peer,width){assert(bands==2);
#else
  :WorkerImage(leaf,peer,width,bands){
#endif
  for(auto part:{std::make_pair("boot",0x6000U),{"globalworker",0x20000U},{"bf16producer",0x30000U},
   {"dense",0x40000U},{"cachedfpu",0x50000U},{"ffn",0x60000U},{"service",0x68000U},
   {"affine",0x70b00U},{"text",0x72000U},{"gdn",0x170000U}})
   rom.push_back({part.second,Read((std::string(worker)+"/"+part.first+".bin").c_str())});
  for(const auto& section:rom){
   assert(!(section.second.size()&3));
   for(unsigned i=0;i<section.second.size();i+=4)
    assert(PioneerQ38HybridTokenL1::SnapshotContains(peer,section.first+i));
  }
  std::ifstream symbols(std::string(worker)+"/program.symbols");std::string a,k,n;
  while(symbols>>a>>k>>n){const unsigned p=unsigned(std::stoul(a,nullptr,16));
   if(n=="block_peer_wait_load")waitPC=p;if(n=="block_fault_stopped")faultPC=p;if(n=="block_peer_stopped")stopPC=p;}
  assert(waitPC && faultPC && stopPC);pc=0x6000;regs[27]=0;
  const unsigned xy=transfer.job.words[7],x=xy&63,y=xy>>6;
  const unsigned xi=x<6?0:x<12?1:2,yi=y<6?0:1;
  source[0]=0x14000;source[1+xi]=x<<((x<6?x:x<12?x-6:x-12)*5);
  source[7+yi]=y<<((y<6?y:y-6)*5);source[13]=source[14]=source[15]=0;
  transfer.command[5]=xy;
  boot[0]=HW::Magic;boot[1]=1;boot[2]=64;boot[3]=peer;boot[4]=xy;boot[5]=source[0];boot[6]=8192;boot[7]=64;
  for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;for(unsigned i=0;i<16;i++)boot[16+i]=source[i];
  for(unsigned i=40;i<=50;i++)boot[i]=HW::BootWord(i);assert(HW::BootShape(boot.data()));
  for(unsigned i=0;i<64;i++){Put(HW::Boot+i*4,boot[i]);Put(HW::Job+i*4,0);}Put(HW::Doorbell,0);
  for(unsigned i=0;i<16;i++)Put(HW::Status+i*4,0);
 }
 const std::vector<uint8_t>* Rom(unsigned a,unsigned& base)const{
  for(const auto& p:rom)if(Range(a,p.first,unsigned(p.second.size()))){base=p.first;return &p.second;}return nullptr;
 }
 unsigned Index(uint32_t a)override{
  unsigned base=0;if(auto* p=Rom(a,base)){l1[8190]=Word(*p,a-base);return 8190;}
  assert(!(a&3));if(Range(a,0x6e000,8192))return (a-0x6e000)/4;
  if(Range(a,HW::Boot,512))return 2048+(a-HW::Boot)/4;
  if(Range(a,HW::Doorbell,4))return 2176;
  if(Range(a,HW::Status,64))return 2177+(a-HW::Status)/4;
  if(Range(a,0x74000,96))return 2200+(a-0x74000)/4;
  if(a==0x74100)return 2224;
  if(Range(a,0x83f00,256))return 2230+(a-0x83f00)/4;
  if(Range(a,W::Output,8*32*4))return 2300+(a-W::Output)/4;
  assert(false);return 0;
 }
 void Put(unsigned a,unsigned v){l1[Index(a)]=v;}
 unsigned Get(unsigned a){return l1[Index(a)];}
 bool CodeAddress(uint32_t a)const override{unsigned base=0;return Rom(a,base)!=nullptr;}
 unsigned Source(unsigned i)const{return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 uint32_t Load(uint32_t a)override{
  unsigned base=0;if(auto* p=Rom(a,base))return Word(*p,a-base);
  if(Range(a,HW::Boot,512)||a==HW::Doorbell||Range(a,HW::Status,64)||a==0x74100||Range(a,0x83f00,256))return Get(a);
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+Source(i))return source[i];
  return WorkerImage::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!CodeAddress(a)); // never write through a simulated immutable code view
  if(a<0x180000)assert(PioneerQ38HybridTokenL1::SnapshotContains(boot[3],a));
  if(Range(a,HW::Status,64)||Range(a,0x74000,96)||a==0x74100||Range(a,0x83f00,256)){Put(a,value);return;}
  WorkerImage::Store(a,value);
 }
 void StepWorker(){
  trace[traceIndex++%32]=pc;
  Step();if(regs[27]){if(!seal)seal=regs[27];assert(regs[27]==seal);}
  if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}
 }
 bool TryBoot(){while(pc!=waitPC && pc!=faultPC)StepWorker();return pc==waitPC && seal && Get(HW::Status+12)==HW::Magic;}
 void Boot(){assert(TryBoot());}
 void Stage(unsigned layer,unsigned sequence,unsigned role,bool corrupt=false){
  auto& leaf=transfer.job;leaf.words[11]=sequence;
  uint32_t j[64]={};j[0]=HW::Magic;j[1]=sequence;j[2]=1;j[3]=boot[3];j[4]=boot[4];j[5]=layer;j[6]=0;j[7]=2025;
  for(unsigned i=0;i<8;i++)j[8+i]=boot[8+i];for(unsigned i=0;i<16;i++)j[16+i]=leaf.words[i];
  j[32]=0;j[33]=leaf.words[2]==5120?17408:5120;j[34]=role;j[35]=kBits;
  assert(HW::JobShape(j,boot.data(),sequence-1,0));if(corrupt)j[35]^=1;
  for(unsigned i=0;i<64;i++)Put(HW::Job+i*4,j[i]);Put(HW::Doorbell,sequence);
 }
 bool Finish(unsigned sequence,bool expectFault=false){
  while(pc!=faultPC && !(pc==waitPC && Get(HW::Status+4)==sequence))StepWorker();
  if(pc==faultPC && !expectFault){fprintf(stderr,"worker fault after %u steps, last PCs:",steps);for(unsigned i=0;i<32;i++)fprintf(stderr," %x",trace[(traceIndex+i)%32]);fprintf(stderr,"\n");}
  return pc!=faultPC;
 }
};
int main(int argc,char** argv){
#ifdef PIONEER_Q38_TEST_LINKED_RECEIPT
 assert(argc==5);
 auto linked=std::make_unique<HybridLinkedReceipt>(argv[3]),oldLinked=std::make_unique<HybridLinkedReceipt>(argv[4]);
#else
 assert(argc==3);
#endif
 for(unsigned width:{5120U,17408U})for(unsigned peer:{1U,119U}){
  if(width==17408 && peer<=68)continue; // requires prior real activation producer
  HybridImage image(argv[1],argv[2],peer,width);image.Boot();
  const unsigned indices[]={2,3,4,5,1,8,4,10};uint32_t before[8],after[8];
  for(unsigned i=0;i<8;i++)before[i]=image.transfer.counters[indices[i]];
  image.Stage(0,1,width==5120?2:4);assert(image.Finish(1));
#ifdef PIONEER_TEST_BFP8
  const auto* expected=PioneerNativeBfpBandGolden(width);
#else
  const auto* expected=PioneerNativeBfp4BandGolden(width);
#endif
  assert(expected);
  for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++)assert(image.Get(W::Output+(band*32+row)*4)==expected[row]);
  assert(image.Get(0x83f40)==64 && image.Get(0x83f44)==1 && image.Get(0x83f4c)==W::Magic);
  uint32_t proof[64],job[64];for(unsigned i=0;i<64;i++){proof[i]=image.Get(0x83f00+i*4);job[i]=image.Get(HW::Job+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=image.transfer.counters[indices[i]];
  assert(PioneerQ38HybridMatrixReceipt::Matrix(proof,job,image.boot.data(),before,after));
#ifdef PIONEER_Q38_TEST_LINKED_RECEIPT
  assert(linked->Run(proof,job,image.boot.data(),before,after));
  assert(!oldLinked->Run(proof,job,image.boot.data(),before,after));
  for(unsigned word:{0U,8U,19U,20U,22U,23U,28U,32U,48U,49U,50U,56U,57U}){
   proof[word]^=1;assert(!linked->Run(proof,job,image.boot.data(),before,after));proof[word]^=1;
  }
  after[0]++;assert(!linked->Run(proof,job,image.boot.data(),before,after));after[0]--;
#endif
  for(unsigned word=0;word<64;word++)for(unsigned bit=0;bit<32;bit++){
   proof[word]^=1U<<bit;assert(!PioneerQ38HybridMatrixReceipt::Matrix(proof,job,image.boot.data(),before,after));proof[word]^=1U<<bit;
  }
  for(unsigned i=0;i<8;i++){
   after[i]++;assert(!PioneerQ38HybridMatrixReceipt::Matrix(proof,job,image.boot.data(),before,after));after[i]--;
  }
  // Original and BFP8 jobs finish on input; a multi-band BFP4 reuse job
  // finishes on the final weight and must reject a changed weight address.
  // Neither receipt alone authenticates the uploaded model or its owner.
  job[19]^=64;
  assert(HW::JobShape(job,image.boot.data(),job[1]-1,0));
#if defined(PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION) && !defined(PIONEER_TEST_BFP8)
  assert(!PioneerQ38HybridMatrixReceipt::Matrix(proof,job,image.boot.data(),before,after));
#else
  assert(PioneerQ38HybridMatrixReceipt::Matrix(proof,job,image.boot.data(),before,after));
#endif
  job[19]^=64;
  printf("PASS actual QGW5 dispatcher -> BFP%u leaf K%u peer%u, 64 exact synthetic words and receipt/2056 mutations, simulated NoC/FPU; NO DEVICE\n",
   kBits,width,peer);
 }
#ifdef PIONEER_Q38_TEST_LINKED_RECEIPT
 for(unsigned bands:{1U,3U,8U}){
  HybridImage image(argv[1],argv[2],119,5120,bands);image.Boot();
  const unsigned indices[]={2,3,4,5,1,8,4,10};uint32_t before[8],after[8];
  for(unsigned i=0;i<8;i++)before[i]=image.transfer.counters[indices[i]];
  image.Stage(0,1,2);assert(image.Finish(1) && image.stores==bands*32 && image.inputPreservationChecked);
  const auto* expected=PioneerNativeBfp4BandGolden(5120);assert(expected);
  for(unsigned band=0;band<bands;band++)for(unsigned row=0;row<32;row++)assert(image.Get(W::Output+(band*32+row)*4)==expected[row]);
  uint32_t proof[64],job[64];for(unsigned i=0;i<64;i++){proof[i]=image.Get(0x83f00+i*4);job[i]=image.Get(HW::Job+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=image.transfer.counters[indices[i]];
  assert(linked->Run(proof,job,image.boot.data(),before,after));
  assert(!oldLinked->Run(proof,job,image.boot.data(),before,after));
 }
 puts("PASS emitted ROOT receipt accepts actual simulated reuse worker for 1/2/3/8 bands; old ROOT rejects the distinct protocol even for one band; corrupted proofs refused; NO DEVICE");
 {HybridImage oldBoot(argv[1],argv[2],119,5120);oldBoot.Put(HW::Boot,0x35574751);
  assert(!oldBoot.TryBoot() && !oldBoot.transfer.issues && !oldBoot.stores);}
#endif
 HybridImage invalid(argv[1],argv[2],119,5120);invalid.Boot();invalid.Stage(0,1,2,true);
 assert(!invalid.Finish(1,true) && !invalid.transfer.issues && !invalid.stores);
 puts("PASS malformed QGW5 job faults before any transfers; NO DEVICE OR WORK");
}
