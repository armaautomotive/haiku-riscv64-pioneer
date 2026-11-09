/* SPDX-License-Identifier: MIT */
// Full vocabulary DATA proposal and head jobs; no device or execution.
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38HybridHeadLayout.h"
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../PioneerQ38HybridHeadModelPlan.h"
#include <array>
#include <fcntl.h>
#include <memory>
#include <unistd.h>
#include <vector>

namespace H=PioneerQ38HybridHeadLayout;namespace W=PioneerQ38HybridHeadWorkerLayout;
using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridHeadModelPlan;
struct MutatedPlan {
 static constexpr uint64_t Capacity=Canonical::Capacity;static constexpr unsigned WeightKind=Canonical::WeightKind;
 const Canonical& source;unsigned target,fault;mutable PioneerQ38CanonicalPart copy={};
 bool Ready()const{return source.Ready();}unsigned Count()const{return source.Count();}uint64_t Used(unsigned bank)const{return source.Used(bank);}
 const PioneerQ38CanonicalPart* Part(unsigned i)const{
  const auto* p=source.Part(i);if(!p)return nullptr;copy=*p;
  if(i==target){
   if(fault==0)copy.role=0;if(fault==1)copy.type=12;if(fault==2)copy.firstRow+=64;if(fault==3)copy.rows-=64;
   if(fault==4)copy.bank=7;if(fault==5)copy.rowBytes++;if(fault==6)copy.offset+=4096;if(fault==7)copy.allocated+=4096;
  }
  return &copy;
 }
};
static std::array<uint32_t,64> RootBoot(){
 std::array<uint32_t,64>b={};b[0]=H::C::Magic;b[1]=1;b[2]=64;b[3]=129;b[4]=b[16]=0x14000;b[5]=119;b[6]=8192;b[7]=64;
 for(unsigned i=0;i<4;i++)b[8+i*2]=i+1;for(unsigned i=40;i<46;i++)b[i]=H::C::BootWord(i);
 assert(H::C::BootShape(b.data()));return b;
}
static std::array<uint32_t,64> WorkerBoot(const std::array<uint32_t,64>& root,unsigned worker){
 std::array<uint32_t,64>b={};b[0]=W::Magic;b[1]=1;b[2]=64;b[3]=worker;b[4]=W::Peer(worker);b[5]=b[16]=0x14000;b[6]=8192;b[7]=64;
 for(unsigned i=0;i<8;i++)b[8+i]=root[8+i];for(unsigned i=40;i<=50;i++)b[i]=W::BootWord(i);
 assert(W::BootShape(b.data()));return b;
}
int main(int argc,char** argv){
 assert(argc==1 || (argc>=2 && argc<=4 && argv[1][0]=='/' && (argc<3 || argv[2][0]=='/') && (argc<4 || argv[3][0]=='/')));
 auto canonical=std::make_unique<Canonical>();assert(canonical->Build());Plan::Proposal proposal;
 assert(Plan::Build(*canonical,proposal) && proposal.parts.size()==canonical->Count());
 PioneerQ38HybridModelPlan::Proposal reference;
 assert(PioneerQ38HybridModelPlan::Build(*canonical,UINT64_C(0x1ffc000000000000),reference));
 auto root=RootBoot();std::array<uint32_t,128>h={};unsigned work=UINT32_MAX,norm=UINT32_MAX,heads=0;
 uint64_t next[7]={};
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);const auto& q=proposal.parts[i];assert(p && q.original==i && q.bank==p->bank && q.offset==next[q.bank]);
  next[q.bank]+=q.allocated;assert(next[q.bank]<=Canonical::Capacity);
  if(p->kind==Canonical::WorkspaceKind && p->bank==0)work=i;
  if(p->kind==Canonical::WeightKind && p->layer==64 && p->role==2)norm=i;
  if(p->kind==Canonical::WeightKind && p->layer==64 && p->role==1){
   assert(q.encoding==Plan::HeadBfp8 && q.bytes==uint64_t(p->rows)*5440);
   auto* s=h.data()+H::Stripe+p->bank*H::StripeWords;
   const unsigned values[]={p->bank,Canonical::Base+q.offset,p->firstRow,p->rows,5440,q.bytes,q.allocated,139};
   for(unsigned j=0;j<8;j++)s[j]=values[j];heads++;
  }else assert(q.bytes==reference.parts[i].bytes && q.allocated==reference.parts[i].allocated
   && q.encoding==reference.parts[i].encoding);
 }
 assert(work<canonical->Count() && norm<canonical->Count() && heads==7);
 for(unsigned b=0;b<7;b++)assert(next[b]==proposal.used[b]);
 std::vector<uint32_t> manifest(Plan::ManifestWords);assert(Plan::Manifest(*canonical,proposal,manifest.data(),manifest.size()));
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);assert(p);
  if(p->kind==Canonical::WeightKind && p->layer<64 && p->role>=2 && p->role<=4)
   assert(manifest[(p->layer*3+p->role-2)*7+p->bank]==Canonical::Base+proposal.parts[i].offset);
 }
 auto changed=proposal;changed.parts[0].offset+=4096;auto intact=manifest;
 assert(!Plan::Manifest(*canonical,changed,manifest.data(),manifest.size()) && manifest==intact);
 assert(!Plan::Manifest(*canonical,proposal,reinterpret_cast<uint32_t*>(proposal.parts.data()),manifest.size()));
 const auto& wp=proposal.parts[work];const auto& np=proposal.parts[norm];const unsigned workspace=Canonical::Base+wp.offset;
 const unsigned fixed[]={H::Magic,1,128,0,workspace,workspace+H::C::HeadOffset,np.bank,Canonical::Base+np.offset,5120,248320,973,9,119,8,5440,20480};
 for(unsigned i=0;i<16;i++){h[i]=fixed[i];h[16+i]=root[16+i];}for(unsigned i=0;i<8;i++)h[32+i]=root[8+i];
 const unsigned witness[]={W::Magic,H::C::Magic,64,0,32,8,0,1};for(unsigned i=0;i<8;i++)h[40+i]=witness[i];
 assert(H::Shape(h.data(),root.data(),0,workspace));
 std::vector<unsigned char> coverage(248320);unsigned total=0;
 uint64_t bands=0,weightReads=0,inputReads=0,reusedInputReads=0,weightBytes=0,inputBytes=0,reusedInputBytes=0;
 for(unsigned ordinal=0;ordinal<H::Jobs;ordinal++){
  unsigned worker=ordinal%119+1,wave=ordinal/119,bank=ordinal%7;
  auto boot=WorkerBoot(root,worker);std::array<uint32_t,64>job={};
  assert(H::Job(h.data(),ordinal,3,17,boot.data(),job.data()) && W::JobShape(job.data(),boot.data(),3,0));
  assert(job[2]==W::GlobalMatrix && job[3]==worker && job[5]==64 && job[6]==17 && job[7]==2025+17*4096+wave
   && job[34]==wave && job[35]==W::JobWitness && job[33]==248320 && job[20]>=7 && job[20]<=8);
  const auto* stripe=h.data()+H::Stripe+bank*8;const unsigned count=job[20]*32,first=job[32];
  assert(first>=stripe[2] && first+count<=stripe[2]+stripe[3] && job[21]==PioneerQ38ProjectionRoute(bank)
   && job[19]==stripe[1]+((first-stripe[2])/32)*174080);
  PioneerQ38Bfp8WorkerJob::Job leaf={};for(unsigned i=0;i<16;i++)leaf.words[i]=job[16+i];
  assert(PioneerQ38Bfp8WorkerJob::Shape(leaf));
  for(unsigned operand=0;operand<2;operand++){
   PioneerQ38Bfp8WorkerJob::Read firstPacket={},lastPacket={};
   assert(PioneerQ38Bfp8WorkerJob::Packet(leaf,0,operand,0,firstPacket)
    && PioneerQ38Bfp8WorkerJob::Packet(leaf,leaf.words[4]-1,operand,159,lastPacket));
   assert(firstPacket.bytes==(operand?64U:1088U) && lastPacket.bytes==firstPacket.bytes);
  }
  const uint64_t tiles=5120/32;bands+=leaf.words[4];weightReads+=leaf.words[4]*tiles;
  inputReads+=leaf.words[4]*tiles;reusedInputReads+=tiles;
  weightBytes+=leaf.words[4]*tiles*1088;inputBytes+=leaf.words[4]*tiles*64;reusedInputBytes+=tiles*64;
  for(unsigned row=first;row<first+count;row++){assert(!coverage[row]);coverage[row]=1;total++;}
 }
 assert(total==248320);for(auto c:coverage)assert(c==1);
 assert(bands==7760 && weightReads==1241600 && inputReads==1241600 && reusedInputReads==155680
  && weightBytes==1350860800 && inputBytes==79462400 && reusedInputBytes==9963520);
 printf("QGW8_HEAD_STATIC_LOAD_COUNTS jobs%u bands%llu weight_reads%llu input_reads%llu weight_bytes%llu input_bytes%llu "
  "hypothetical_reused_input_reads%llu hypothetical_reused_input_bytes%llu;NO_DEVICE_TIMING_OR_REUSE_AUTHORITY\n",
  unsigned(H::Jobs),(unsigned long long)bands,(unsigned long long)weightReads,(unsigned long long)inputReads,
  (unsigned long long)weightBytes,(unsigned long long)inputBytes,(unsigned long long)reusedInputReads,(unsigned long long)reusedInputBytes);
 auto boot=WorkerBoot(root,1);std::array<uint32_t,64>out;out.fill(0xa5a5a5a5);const auto untouched=out;
 if(argc>=2){
  static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"fixture uses little-endian worker words");
  std::array<uint32_t,64> fixture={};assert(H::Job(h.data(),0,0,0,boot.data(),fixture.data()));
  int fd=open(argv[1],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);assert(fd>=0);
  assert(write(fd,fixture.data(),sizeof(fixture))==sizeof(fixture) && !fsync(fd) && !close(fd));
 }
 if(argc>=3){
  int fd=open(argv[2],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);assert(fd>=0);
  assert(write(fd,manifest.data(),manifest.size()*sizeof(uint32_t))==ssize_t(manifest.size()*sizeof(uint32_t)) && !fsync(fd) && !close(fd));
 }
 if(argc==4){
  int fd=open(argv[3],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);assert(fd>=0);
  assert(write(fd,root.data(),sizeof(root))==sizeof(root) && write(fd,h.data(),sizeof(h))==sizeof(h)
   && write(fd,boot.data(),sizeof(boot))==sizeof(boot) && !fsync(fd) && !close(fd));
 }
 assert(!H::Job(h.data(),973,0,0,boot.data(),out.data()) && !H::Job(h.data(),0,0x7ffffffe,0,boot.data(),out.data())
  && !H::Job(h.data(),0,0,8192,boot.data(),out.data()) && out==untouched);
 const auto savedHead=h;const auto savedBoot=boot;
 assert(!H::Job(h.data(),0,0,0,boot.data(),h.data()) && h==savedHead);
 assert(!H::Job(h.data(),0,0,0,boot.data(),boot.data()) && boot==savedBoot);
 for(unsigned field:{0U,3U,8U,9U,10U,11U,12U,13U,14U,15U,16U,32U,40U,41U,42U,44U,45U,47U,64U,65U,66U,67U,68U,69U,70U,71U,120U}){
  auto bad=h;bad[field]^=1;assert(!H::Shape(bad.data(),root.data(),0,workspace));
 }
 auto empty=std::make_unique<Canonical>();const auto size=proposal.parts.size();const auto used=proposal.used;
 assert(!Plan::Build(*empty,proposal) && proposal.parts.size()==size && proposal.used==used);
 unsigned headIndex=canonical->Count();for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);if(p && p->kind==Canonical::WeightKind && p->layer==64 && p->role==1){headIndex=i;break;}
 }
 assert(headIndex<canonical->Count());
 for(unsigned fault=0;fault<8;fault++){
  MutatedPlan damaged{*canonical,headIndex,fault};assert(!Plan::Build(damaged,proposal) && proposal.parts.size()==size && proposal.used==used);
 }
 puts("PASS QGW8 offline head repack and 973 jobs: exact 248320-row one-time coverage, per-bank BFP8 addresses, boundary and corrupt-header refusals; NO source upload, worker lifecycle, device or TPS");
}
