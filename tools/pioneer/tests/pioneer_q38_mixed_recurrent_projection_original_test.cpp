/* SPDX-License-Identifier: MIT */
// ORIGINAL learned BYTES only feed actual QRP1/QMW1 instructions. Inputs
// here are deliberately supplied test vectors, not a valid model prefix.
#define PIONEER_Q38_MIXED_RECURRENT_PROJECTION_TEST
#define PIONEER_Q38_MIXED_PROJECTION_ROOT_MACHINE_ONLY
#include "pioneer_q38_mixed_projection_root_test.cpp"
namespace {
const char* originalPath=nullptr;
FILE* RecurrentSourceFopen(const char* path,const char* mode){
 assert(originalPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));return fopen(originalPath,mode);
}
int RecurrentSourceExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
}
#define fopen RecurrentSourceFopen
#define execv RecurrentSourceExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
class RecurrentOriginalBytes final:public ProjectionExecutionLearnedBytes {
 struct Span {unsigned route;uint32_t address;std::vector<uint8_t> bytes;};
 PioneerQ38PublicModelSource source;PioneerQwen38PackedPlan plan;std::vector<Span> spans;
 uint64_t epoch=0,total=0;unsigned pages=0;bool ready=false;
public:
 bool Prepare(){
  if(ready || !source.Open() || !source.Ready() || !plan.Build(source.Model(),8192,true,true))return false;
  epoch=source.Model().Epoch();
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];
   if(p.kind!=PIONEER_Q38_WEIGHT || p.layer || (p.role!=10 && p.role!=11 && p.role!=5 && p.role!=6 && p.role!=13
    && p.role!=2 && p.role!=3 && p.role!=4))continue;
   Span span{PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,std::vector<uint8_t>(p.bytes)};
   std::array<uint8_t,4096> page;
   for(unsigned at=0;at<p.bytes;at+=4096){
    if(!source.Ready() || source.Model().Epoch()!=epoch || !source.ReadWeightPage(plan,i,at/4096,page.data(),4096))return false;
    std::memcpy(span.bytes.data()+at,page.data(),std::min(4096U,p.bytes-at));pages++;
   }
   total+=p.bytes;spans.push_back(std::move(span));
  }
  if(spans.size()!=44 || !source.Ready() || source.Model().Epoch()!=epoch)return false;
  ready=true;return Ready();
 }
 bool Ready()const override{return ready && source.Ready() && source.Model().Epoch()==epoch && plan.MatchesSource(source.Model());}
 bool Read(unsigned route,uint32_t address,uint8_t* output,unsigned bytes)override{
  if(!Ready() || !output || !bytes || bytes>3392)return false;
  for(const auto& s:spans)if(s.route==route && address>=s.address && uint64_t(address)-s.address<=s.bytes.size()
   && bytes<=s.bytes.size()-(address-s.address)){
   std::memcpy(output,s.bytes.data()+address-s.address,bytes);return Ready();
  }
  return false;
 }
 uint64_t Bytes()const{return total;}
 unsigned Pages()const{return pages;}
};
int main(int argc,char** argv){
 assert(argc==6);(void)&ProjectionRefusals;(void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;
 setvbuf(stdout,nullptr,_IOLBF,0);originalPath=argv[5];
 puts("START authenticate full SAME original c600 GGUF and cache raw eight-role layer0 weights BEFORE instruction machine");
 RecurrentOriginalBytes original;assert(original.Prepare());
 printf("READY original raw learned bytes%llu/pages%u/spans44;NO numeric reference products uploaded\n",(unsigned long long)original.Bytes(),original.Pages());
 auto m=std::make_unique<ProjectionRootMachine>(argv[1],argv[2],argv[3],argv[4]);assert(m->WaitProjection(0));m->originalLearned=&original;
 unsigned checked=0;
 for(unsigned role:{10U,11U,5U,6U,13U,2U,3U,4U}){
  m->StageProjection(role,0);assert(m->WaitProjection(m->sequence));m->VerifyProjection();checked+=m->descriptor[11];
  assert(original.Ready());
  printf("PASS ORIGINAL actual QRP1 layer0 role%u rows%u/card quantization and matrix outputs vs independent arithmetic/SAME119 lifetime\n",role,m->descriptor[11]);
 }
 assert(checked==61536 && 0x70000-m->minStack<=8192);
 printf("PASS ORIGINAL recurrent matrices8/checked%u/ROOTstack%u;supplied inputs,NOT recurrent chain/native/physical/fullmodel/TPS\n",checked,0x70000-m->minStack);
}
