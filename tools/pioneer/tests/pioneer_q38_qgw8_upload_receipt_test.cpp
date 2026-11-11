/* SPDX-License-Identifier: MIT */
// Exact QGW8 receipt against synthetic bank proof, ledger, and host plan.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_SIMULATION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_BANKS_MOCK_ONLY 1
#if defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWD_PROGRAMS_INSPECTION)
#define PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC 1
#include "../PioneerQ38TokenSelectedPrograms.h"
#ifdef PIONEER_Q38_QGWE_PROGRAMS_INSPECTION
static_assert(PioneerQ38TokenPrograms::Family==0x48505451,"QGWE inspection must select its exact program family");
#elif defined(PIONEER_Q38_QGWD_PROGRAMS_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x47505451,"QGWD inspection must select its exact program family");
#elif defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x46505451,"QGWC inspection must select its exact program family");
#elif defined(PIONEER_Q38_QGWA_BATCH_PROGRAMS_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x44505451,"QGWA inspection must select its exact program family");
#else
static_assert(PioneerQ38TokenPrograms::Family==0x38505451,"QGW8 inspection must select its exact program family");
#endif
#endif
#include "../PioneerQ38TokenStartupPlan.h"
#include "../PioneerQ38HybridHeadLayout.h"
#include "../PioneerQ38Bf16RootPairs.h"
#include "../PioneerQ38HybridRootPairs.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionMutableBanks.h"
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadReceipt.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RuntimeRanges.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridManifest.h"
#include "../PioneerQ38HybridHeadModelPlan.h"
#include <cstring>
#include <memory>
#include <vector>

struct Qgw8RootProbe {
 const PioneerQ38ProjectionJobs& input;
 unsigned parts[119][8]={},rows[119]={},table[7][4]={};
 unsigned Part(unsigned w,unsigned i)const{
  const auto* p=input.Part(w);assert(p);
  return i==0?PioneerQ38CanonicalModelPlan::Base+unsigned(p->offset):i==1?p->bank:i==2?p->groups:
   i==4?Route(p->bank):i==5?Source(w):0;
 }
 unsigned Row(unsigned w)const{return input.Part(w)->firstRow;}
 unsigned Route(unsigned bank)const{return PioneerQ38ProjectionRoute(bank);}
 unsigned Source(unsigned w)const{return w+1;}
 unsigned Result(unsigned w,unsigned i)const{return parts[w][i];}
 unsigned ResultRow(unsigned w)const{return rows[w];}
 unsigned Table(unsigned bank,unsigned field)const{return table[bank][field];}
 void Output(unsigned w,unsigned i,unsigned value){parts[w][i]=value;}
 void OutputRow(unsigned w,unsigned value){rows[w]=value;}
};

int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--tamper") || !strcmp(argv[1],"--extra")
  || !strcmp(argv[1],"--abort") || !strcmp(argv[1],"--late") || !strcmp(argv[1],"--late-canonical")
  || !strcmp(argv[1],"--emit-startup-fixture"))));
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 BanksMock io;auto proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages);
 auto receipt=std::make_unique<PioneerQ38Qgw8UploadReceipt>();int context=0,cookie=0,foreign=0;
 assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 assert(!receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42));
 PioneerQ38SpanLease span;assert(!receipt->Seal(span) && !receipt->Sealed());
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 PioneerQ38HybridHeadModelPlan::Proposal host;assert(PioneerQ38HybridHeadModelPlan::Build(*canonical,host));
 unsigned bank=0,length=0,spans=0,part=0,page=0;uint64_t offset=0,total=0;
 if(argc==2 && !strcmp(argv[1],"--abort")){
  assert(receipt->Next(bank,offset,length));assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  receipt->Abort();assert(banks.Quarantined() && !receipt->Sealed() && !receipt->Next(bank,offset,length));
  puts("PASS QGW8 uncertain receipt quarantines synthetic bank owner; no device");return 0;
 }
 while(receipt->Next(bank,offset,length)){
  while(part<canonical->Count() && canonical->Part(part)->kind!=canonical->WeightKind)part++;
  assert(part<canonical->Count());const auto& expected=host.parts[part];
  const unsigned n=std::min(256U,expected.allocated/4096-page);
  assert(bank==expected.bank && offset==uint64_t(expected.offset)+uint64_t(page)*4096 && length==n*4096);
  const auto first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
  assert(!receipt->Before(&context,hashes.data(),ledger.data(),&span,(bank+1)%7,offset,length));
  assert(!receipt->Before(&foreign,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(!receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  for(unsigned p=0;p<n;p++){
   const uint32_t value=(first+p)*2654435761U;ledger[first+p]=1;hashes[first+p]=value;
   assert(receipt->RecordPage(first+p,value));
  }
  span.saved=span.ready=span.dirty=true;span.nonce=spans+1;
  assert(receipt->After());total+=length;spans++;page+=n;
  if(page==expected.allocated/4096){page=0;part++;}
 }
 assert(spans==22035 && total==UINT64_C(21167624192) && !receipt->Sealed());
 PioneerQ38SpanLease foreignSpan;foreignSpan.saved=true;foreignSpan.closed=true;foreignSpan.nonce=spans;
 assert(!receipt->Seal(foreignSpan));
 span.saved=true;span.closed=true;span.nonce=spans-1;assert(!receipt->Seal(span));
 span.nonce=spans;span.ready=span.dirty=false;
 if(argc==2 && (!strcmp(argv[1],"--tamper") || !strcmp(argv[1],"--extra"))){
  if(!strcmp(argv[1],"--tamper"))hashes[0]^=1;
  else {unsigned extra=0;while(extra<ledger.size() && ledger[extra])extra++;assert(extra<ledger.size());ledger[extra]=1;}
  assert(!receipt->Seal(span) && !receipt->Sealed() && banks.Quarantined());
  puts("PASS QGW8 changed/extraneous ledger quarantines before binding; no device");return 0;
 }
 assert(receipt->Seal(span) && receipt->Sealed() && receipt->PageDigest() && receipt->Layout());
 auto bound=std::make_unique<PioneerQ38Qgw8KernelLayout>();
 auto native=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(!receipt->DeriveBoundCanonical(*native,&context,&foreign,hashes.data(),ledger.data(),17,42) && !native->Ready());
 if(argc==2 && !strcmp(argv[1],"--late-canonical")){
  hashes[0]^=1;assert(!receipt->DeriveBoundCanonical(*native,&context,&cookie,hashes.data(),ledger.data(),17,42)
   && banks.Quarantined() && !receipt->Sealed() && !native->Ready());
  puts("PASS QGW8 canonical refuses changed post-seal ledger; no device");return 0;
 }
 if(argc==2 && !strcmp(argv[1],"--late")){hashes[0]^=1;assert(!receipt->DeriveBoundLayout(*bound,&context,&cookie,hashes.data(),ledger.data(),17,42)
  && banks.Quarantined() && !receipt->Sealed());
  puts("PASS QGW8 post-seal ledger mutation quarantines; no device");return 0;}
 assert(!receipt->DeriveBoundLayout(*bound,&context,&foreign,hashes.data(),ledger.data(),17,42));
 assert(receipt->DeriveBoundLayout(*bound,&context,&cookie,hashes.data(),ledger.data(),17,42) && bound->Ready()
  && bound->Count()==4678 && !receipt->DeriveBoundLayout(*bound,&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(receipt->DeriveBoundCanonical(*native,&context,&cookie,hashes.data(),ledger.data(),17,42)
  && native->Qgw8InspectionLayout() && !native->OriginalFormatLayout() && !native->HybridFormatLayout());
#if defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWD_PROGRAMS_INSPECTION)
 assert(PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*native)
  && !PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*canonical));
 auto legacy=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(legacy->BuildHybridDiagnostic() && !PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*legacy));
#else
 assert(!PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*native));
#endif
 assert(PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*native,ledger.data(),ledger.size()));
 unsigned recurrent=0;
 for(unsigned i=0;i<native->Count();i++){
  const auto* p=native->Part(i);const auto* old=canonical->Part(i);const auto& expected=host.parts[i];
  assert(p && old && p->kind==old->kind && p->layer==old->layer && p->role==old->role && p->type==old->type
   && p->firstRow==old->firstRow && p->rows==old->rows && p->rowBytes==old->rowBytes
   && p->bank==expected.bank && p->offset==expected.offset && p->bytes==expected.bytes && p->allocated==expected.allocated);
  if(p->kind==PioneerQ38CanonicalModelPlan::RecurrentKind){
   const auto& r=native->RecurrentRegions()[recurrent++];
   assert(r.layer==p->layer && r.slot==p->role && r.firstHead==p->firstRow && r.heads==p->rows
    && r.bank==p->bank && r.offset==p->offset);
  }
 }
 assert(recurrent==672);
 for(unsigned b=0;b<7;b++)assert(native->Workspace(b) && native->Workspace(b)->bank==b && native->Used(b)==bound->Used(b));
 std::array<uint32_t,PioneerQ38HybridManifest::Words> manifest={},expectedManifest={};
 assert(PioneerQ38HybridManifest::Build(*native,manifest.data(),manifest.size())
  && PioneerQ38HybridHeadModelPlan::Manifest(*canonical,host,expectedManifest.data(),expectedManifest.size())
  && manifest==expectedManifest);
 auto refusesManifest=[&]{
  auto untouched=expectedManifest;untouched.fill(0xa5a5a5a5U);const auto before=untouched;
  assert(!PioneerQ38HybridManifest::Build(*native,untouched.data(),untouched.size()) && untouched==before);
 };
 for(unsigned layer:{49U,50U,60U,61U}){
  auto* p=const_cast<PioneerQ38CanonicalPart*>(native->ProjectionStripe(layer,2,0));assert(p);
  const auto saved=*p;p->bytes+=4096;refusesManifest();*p=saved;
  p->type=8;refusesManifest();*p=saved;p->offset++;refusesManifest();*p=saved;
 }
 auto untouched=expectedManifest;assert(!PioneerQ38HybridManifest::Build(*canonical,untouched.data(),untouched.size())
  && untouched==expectedManifest);
 unsigned relocatedJobs=0;
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
  auto jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(jobs->Build(*native,layer,role,8191) && jobs->Count()==119);
  Qgw8RootProbe root{*jobs};const PioneerQ38CanonicalPart* byBank[7]={};
  for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=native->ProjectionStripe(layer,role,stripe);assert(p);
   assert(p->bank<7 && !byBank[p->bank]);byBank[p->bank]=p;
   root.table[p->bank][0]=native->Base+p->offset;
   root.table[p->bank][1]=manifest[(layer*3+role-2)*7+p->bank];
   root.table[p->bank][2]=p->firstRow;root.table[p->bank][3]=p->rows;
  }
  assert(PioneerQ38Bf16RootPairs::Build(root,jobs->Inputs(),jobs->Outputs())
   && PioneerQ38HybridRootPairs::Relocate(root,layer,jobs->Inputs(),jobs->Outputs()));
  for(unsigned w=0;w<119;w++){
   const unsigned bank=root.parts[w][1],row=root.rows[w];assert(bank<7);const auto* stripe=byBank[bank];
   assert(stripe && row>=stripe->firstRow && row<stripe->firstRow+stripe->rows && !((row-stripe->firstRow)&31));
   const unsigned width=role==4?17408:5120;const bool eight=layer>=50 && layer<=60;
   const uint64_t expected=uint64_t(native->Base)+stripe->offset+uint64_t(row-stripe->firstRow)*width*(eight?17:9)/16;
   assert(root.parts[w][0]==expected && root.parts[w][4]==PioneerQ38ProjectionRoute(bank));relocatedJobs++;
  }
 }
 assert(relocatedJobs==64*3*119);
 std::array<uint32_t,1920> sources={};for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
 const PioneerQ38GdnIdentity startupId={1,2,3,42};const uint32_t prompt[]={248319,17};
 for(unsigned bank=0;bank<7;bank++){
  auto startup=std::make_unique<PioneerQ38TokenStartupPlan>();
  assert(startup->Build(*native,bank,startupId,sources.data(),prompt,2,8192,true,248318,248317)
   && startup->Matches(*native) && startup->Workspace()==native->Base+native->Workspace(bank)->offset);
#if defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWD_PROGRAMS_INSPECTION)
  for(unsigned worker=1;worker<120;worker++){
   std::array<uint32_t,64> peer={};
   assert(startup->ReadBoot(worker,peer.data()) && PioneerQ38SelectedGlobalWorker::BootShape(peer.data())
    && peer[0]==
#ifdef PIONEER_Q38_QGWE_PROGRAMS_INSPECTION
       0x45574751
#elif defined(PIONEER_Q38_QGWD_PROGRAMS_INSPECTION)
       0x44574751
#elif defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION)
       0x43574751
#elif defined(PIONEER_Q38_QGWA_BATCH_PROGRAMS_INSPECTION)
       0x41475751
#else
       0x38574751
#endif
    && peer[49]==0 && peer[50]==0x1ffc0000);
#ifdef PIONEER_Q38_QGWE_PROGRAMS_INSPECTION
   if(bank==0 && argc==2 && !strcmp(argv[1],"--emit-startup-fixture")){
    printf("QGWE_STARTUP_WORKER_%u ",worker);
    for(const auto byte:peer)for(unsigned n=0;n<4;n++)printf("%02x",unsigned((byte>>(n*8))&255));
    putchar('\n');
   }
#endif
  }
#endif
  std::array<uint32_t,128> head={};
  assert(startup->ReadHead(0,reinterpret_cast<uint8_t*>(head.data()),sizeof(head))
   && PioneerQ38HybridHeadLayout::Shape(head.data(),startup->Boot(),bank,startup->Workspace()));
  unsigned seen=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);const auto& q=host.parts[i];
   if(p->kind!=canonical->WeightKind || p->layer!=64 || p->role!=1)continue;
   const auto* stripe=head.data()+64+q.bank*8;
   assert(stripe[0]==q.bank && stripe[1]==canonical->Base+q.offset && stripe[2]==p->firstRow
    && stripe[3]==p->rows && stripe[4]==5440 && stripe[5]==q.bytes && stripe[6]==q.allocated && stripe[7]==139);seen++;
  }
  assert(seen==7);
  if(bank==0){
   if(argc==2 && !strcmp(argv[1],"--emit-startup-fixture")){
    static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"simulation fixture uses little endian words");
    namespace W=PioneerQ38HybridHeadWorkerLayout;std::array<uint32_t,64> worker={},job={};
    worker[0]=W::Magic;worker[1]=1;worker[2]=64;worker[3]=1;worker[4]=W::Peer(1);
    worker[5]=worker[16]=0x14000;worker[6]=8192;worker[7]=64;
    for(unsigned i=0;i<8;i++)worker[8+i]=startup->Boot()[8+i];
    for(unsigned i=40;i<=50;i++)worker[i]=W::BootWord(i);
    assert(PioneerQ38HybridHeadLayout::Job(head.data(),0,0,0,worker.data(),job.data()));
    std::array<uint32_t,256> fixture={};
    std::copy(startup->Boot(),startup->Boot()+64,fixture.begin());std::copy(head.begin(),head.end(),fixture.begin()+64);
    std::copy(worker.begin(),worker.end(),fixture.begin()+192);
    auto print=[](const char* label,const void* bytes,size_t count){
     printf("%s ",label);const auto* p=static_cast<const uint8_t*>(bytes);
     for(size_t i=0;i<count;i++)printf("%02x",unsigned(p[i]));putchar('\n');
    };
    print("QGW8_STARTUP_HEAD_FIXTURE",fixture.data(),sizeof(fixture));print("QGW8_STARTUP_HEAD_JOB",job.data(),sizeof(job));
   }
   auto* part=const_cast<PioneerQ38CanonicalPart*>(native->ProjectionStripe(64,1,0));assert(part);
   const auto saved=*part;part->bytes--;
   std::array<uint32_t,128> untouched;untouched.fill(0xa5a5a5a5U);const auto before=untouched;
   assert(!startup->Matches(*native) && !startup->ReadHead(0,reinterpret_cast<uint8_t*>(untouched.data()),sizeof(untouched))
    && untouched==before);*part=saved;
   assert(startup->Matches(*native));
  }
 }
 PioneerResidentMutableRange ranges[7]={};assert(PioneerQ38RuntimeRanges(*native,ranges));
 uint64_t learnedEnd[7]={};
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);const auto& q=host.parts[i];
  if(p->kind==PioneerQ38CanonicalModelPlan::WeightKind)learnedEnd[q.bank]=uint64_t(q.offset)+q.allocated;
 }
 for(unsigned b=0;b<7;b++)assert(ranges[b].bank==b && ranges[b].firstPage==learnedEnd[b]/4096
  && ranges[b].pages==(host.used[b]-learnedEnd[b])/4096);
 // Reject wrong converted physical geometry without publishing any range.
 for(unsigned i=0;i<native->Count();i++){
  auto* p=const_cast<PioneerQ38CanonicalPart*>(native->Part(i));
  if(p->kind!=PioneerQ38CanonicalModelPlan::WeightKind || !((p->layer==64 && p->role==1)
   || (p->layer==50 && p->role==2)))continue;
  const auto saved=*p;
  auto refuses=[&]{
   PioneerResidentMutableRange untouched[7];std::memset(untouched,0xa5,sizeof(untouched));
   PioneerResidentMutableRange before[7];std::memcpy(before,untouched,sizeof(before));
   assert(!PioneerQ38RuntimeRanges(*native,untouched) && !std::memcmp(before,untouched,sizeof(before)));*p=saved;
  };
  p->bytes--;refuses();p->rowBytes++;refuses();p->type=8;refuses();p->firstRow++;refuses();
  p->layer=p->layer==64?63:49;refuses();
 }
 assert(!receipt->DeriveBoundCanonical(*native,&context,&cookie,hashes.data(),ledger.data(),17,42));
 auto graph=std::make_unique<PioneerQ38ProjectionMutableBanks>();PioneerQ38ProjectionIdentity id={1,2,3,42};
 assert(graph->BindCanonical(banks,proof,id,ledger.data(),ledger.size(),*native,0));
 auto* history=graph->ConvHistory(id);auto* recurrentOwner=graph->Recurrent(id);auto* projection=graph->Projection(id);
 assert(history && recurrentOwner && projection && graph->CanonicalModel(id)==native.get());
 unsigned progress=~0U;
 assert(history->InitializationProgress(id,progress) && !progress && !history->InitializationPublished(id));
 assert(recurrentOwner->InitializationProgress(id,progress) && !progress && !recurrentOwner->InitializationPublished(id));
 assert(projection->InitializationProgress(id,progress) && !progress && !projection->InitializationPublished(id));
 for(unsigned b=0;b<7;b++){
  assert(banks.ImmutableRange(proof,b,learnedEnd[b]-4096,4096));
  assert(!banks.ImmutableRange(proof,b,learnedEnd[b],4096));
 }
 auto second=std::make_unique<PioneerQ38ProjectionMutableBanks>();
 assert(!second->BindCanonical(banks,proof,id,ledger.data(),ledger.size(),*native,0));
 assert(!receipt->Seal(span) && !receipt->Next(bank,offset,length));
 assert(receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42));
 printf("PASS QGW8 receipt and actual runtime reservation: %u host-plan-matched extents, 3551 fragments, %llu bytes, %u FFN jobs; synthetic owner/ledger only, no device\n",
  spans,(unsigned long long)total,relocatedJobs);
}
