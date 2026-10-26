/* SPDX-License-Identifier: MIT */
// Actual emitted capture/recipe-loader instructions. ALL predecessor,
// peer-status/proof and recipe setup is synthetic DATA, NOT execution or
// native producer/lifetime qualification. Full combined test is separate.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_graph_math_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentContinuePlan.h"
#include <array>
#include <memory>
struct ContinueAdmission:ScalarMachine {
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::unique_ptr<PioneerQ38ResidentLayerPlan> parent=std::make_unique<PioneerQ38ResidentLayerPlan>();
 PioneerQ38ResidentLayerLinkPlan link;PioneerQ38ResidentAdvancePlan advance;
 std::unique_ptr<PioneerQ38ResidentContinuePlan> plan=std::make_unique<PioneerQ38ResidentContinuePlan>();
 std::array<std::vector<uint8_t>,12> rom;
 std::vector<uint32_t> local=std::vector<uint32_t>(0x180000/4,0xdeadbeef),recipe;
 std::array<uint32_t,1920> sources;std::array<uint32_t,119> actual;
 uint32_t counts[8],source[16],command[10]={};unsigned capture=0,load=0,packets=0,binds=0,section=0,offset=0,recipeBytes=0;
 unsigned failPacket=0,sourceFault=0,counterFault=0,headerFault=0,sequenceFault=0;bool pending=false;
 unsigned badPeer=0,badRecipe=0;
 static unsigned Base(unsigned i){const unsigned b[]={0x6000,0x72000,0x64000,0x68000,0xb8000,0x158000,0x159600,0x15c000,0x40000,0x46000,0x4e000,0x30000};return b[i];}
 static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
 static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 uint32_t& Mem(unsigned at){assert(!(at&3) && at<0x180000);return local.at(at/4);}
 ContinueAdmission(const char* image,unsigned trial=0):ScalarMachine(5120,0){
  assert(canonical->Build());std::array<uint32_t,119> zero={};
  for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)sources[w*16+i]=(i?0x12340000+i:0x16005)+w*256;
  for(unsigned i=0;i<16;i++)source[i]=sources[i];for(unsigned i=0;i<8;i++)counts[i]=0xfffffff8;
  assert(parent->Build(*canonical,trial?61:0,trial?8191:0,trial?6:0,trial,248319,17,{1,2,3,4},sources.data(),zero.data()));
  assert(link.Build(*canonical,*parent) && advance.Build(*canonical,*parent,link) && plan->Build(*canonical,*parent,link,advance));
  const char* names[]={"boot","text","conv","matrix","cohort","norm","link","ffn","advance_matrix","advance","advance_gather","continue"};
  for(unsigned i=0;i<12;i++)rom[i]=Read((std::string(image)+"/"+names[i]+".bin").c_str());
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string line;
  while(std::getline(symbols,line)){unsigned at=0;char type=0,name[256]={};if(sscanf(line.c_str(),"%x %c %255s",&at,&type,name)==3){
   if(!strcmp(name,"q38_resident_continue_capture"))capture=at;if(!strcmp(name,"q38_resident_continue_load_recipe"))load=at;}}
  assert(capture && load);
  for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++)Mem(TT_Q38_COHORT_MANIFEST+i*4)=Mem(TT_Q38_COHORT_SHADOW+i*4)=parent->Manifest()[i];
  for(unsigned i=0;i<TT_Q38_ADVANCE_WORDS;i++)Mem(TT_Q38_ADVANCE_MANIFEST+i*4)=advance.Descriptor()[i];
  for(unsigned i=0;i<128;i++)Mem(TT_Q38_CONTINUE_MANIFEST+i*4)=plan->Descriptor()[i];
  for(unsigned w=0;w<119;w++){
   actual[w]=w<3?14:w<20?12:w<24?10:w<68?9:w<96?8:7;Mem(TT_Q38_COHORT_SEQUENCE+w*4)=actual[w];
   unsigned proof=0x76000+w*256;for(unsigned i=0;i<64;i++)Mem(proof+i*4)=0;
   for(unsigned i=0;i<8;i++)Mem(proof+(8+i)*4)=0xfffffff8+w*100;
   Mem(proof+17*4)=actual[w];Mem(proof+18*4)=w+1;Mem(proof+19*4)=TT_Q38_FFN_PERSISTENT_MARKER;Mem(proof+31*4)=PioneerQ38ConvPeer(w+1);
   for(unsigned i=0;i<16;i++)Mem(proof+(32+i)*4)=sources[(w+1)*16+i];
  }
  const auto* next=plan->Next();
  recipe.insert(recipe.end(),next->Manifest(),next->Manifest()+TT_Q38_COHORT_WORDS);
  recipe.insert(recipe.end(),next->PostManifest(),next->PostManifest()+TT_Q38_OUTPUT_COHORT_WORDS);
  recipe.insert(recipe.end(),next->LayerManifest(),next->LayerManifest()+TT_Q38_RESIDENT_LAYER_WORDS);
 }
 unsigned Code(unsigned at)const{for(unsigned i=0;i<12;i++)if(Range(at,Base(i),unsigned(rom[i].size())))return i;return 12;}
 bool CodeAddress(uint32_t at)const override{return Code(at)<12;}
 unsigned Index(uint32_t at)override{unsigned i=Code(at);assert(i<12 && !(at&3));l1[1]=Word(rom[i],at-Base(i));return 1;}
 uint32_t Load(uint32_t at)override{
  if(CodeAddress(at))return Word(rom[Code(at)],at-Base(Code(at)));
  if(Range(at,0xffb20000,0x2000)){
   unsigned o=at-0xffb20000;if(o<40)return command[o/4];
   for(unsigned i=0;i<4;i++)if(o==0x40+i*0x800)return i?0:unsigned(pending);
   if(o>=0x240 && o<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(o==Source(i))return source[i];for(unsigned i=0;i<8;i++)if(o==Counter(i))return counts[i];
   assert(!"unqualified continuation CSR");
  }
  assert(!(at&3) && (Range(at,0x60000,0x2400) || Range(at,0x6e000,8192) || Range(at,0x75000,2048)
   || Range(at,0x76000,119*256) || Range(at,TT_Q38_COHORT_MANIFEST,TT_Q38_COHORT_WORDS*8)
   || Range(at,TT_Q38_COHORT_SEQUENCE,119*4) || Range(at,TT_Q38_OUTPUT_COHORT_MANIFEST,TT_Q38_OUTPUT_COHORT_WORDS*4)
   || Range(at,TT_Q38_RESIDENT_LAYER_MANIFEST,TT_Q38_RESIDENT_LAYER_WORDS*4) || Range(at,TT_Q38_ADVANCE_MANIFEST,TT_Q38_ADVANCE_WORDS*4)));
  return Mem(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3));if(Range(at,0xffb20000,40)){assert(!pending);command[(at-0xffb20000)/4]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending);Packet();return;}
  assert(Range(at,0x6e000,8192) || Range(at,TT_Q38_CONTINUE_SHADOW,512) || Range(at,TT_Q38_CONTINUE_RECEIPT,8192)
   || Range(at,TT_Q38_COHORT_MANIFEST+64*4,119*4));
  if(Range(at,TT_Q38_COHORT_MANIFEST+64*4,119*4)){
   assert(!pending && packets==3351 && at==TT_Q38_COHORT_MANIFEST+(64+binds)*4 && value==actual[binds]);binds++;
  }
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,at);Mem(at)=value;
 }
 void Packet(){
  packets++;assert(!command[1] && !command[4] && !command[6] && !command[7] && !command[9] && command[5]==129);
  unsigned remote=command[0],route=command[2],target=command[3],bytes=command[8];
  if(packets==failPacket){pending=true;return;}
  if(packets<=3332){
   unsigned w=(packets-1)/28+1,p=(packets-1)%28;
   unsigned expected=p==0?0x74140:p<3?0xffb13140:p==3?0xffb121b0:p<20?0xffb20000+Source(p-4):0xffb20000+Counter(p-20);
   assert(remote==expected && route==PioneerQ38ConvPeer(w) && target==0x75000+(remote&31) && bytes==(p?4U:64U));
   if(!p){for(unsigned i=0;i<16;i++)Mem(target+i*4)=i<2?actual[w-1]:i==3?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):0;if(badPeer==1 && w==1)Mem(target)^=1;}
   else Mem(target)=p<3?0x7204c:p==3?0x46800:p<20?sources[w*16+p-4]:0xfffffff8+(w-1)*100;
   if(badPeer==2 && w==1 && p==4)Mem(target)^=1;if(badPeer==3 && w==1 && p==20)Mem(target)^=1;
  }else {
   unsigned length=PioneerQ38ResidentContinueWords(section)*4-offset;length=std::min(length,16384U);
   assert(section<3 && remote==plan->Descriptor()[7]+TT_Q38_CONTINUE_RECIPE_OFFSET+recipeBytes && route==plan->Descriptor()[8]
    && target==PioneerQ38ResidentContinueTarget(section)+offset && bytes==length);
   for(unsigned i=0;i<bytes/4;i++)Mem(target+i*4)=recipe[recipeBytes/4+i];recipeBytes+=bytes;offset+=bytes;
   if(offset==PioneerQ38ResidentContinueWords(section)*4){section++;offset=0;}
   if(badRecipe && packets==3351)Mem(TT_Q38_COHORT_MANIFEST+3*4)^=1;
  }
  counts[0]++;counts[1]+=(bytes+63)/64;counts[2]++;counts[3]++;counts[6]++;
  if(packets==sourceFault)source[11]^=1;if(packets==counterFault)counts[0]++;
  if(packets==headerFault)Mem(TT_Q38_CONTINUE_MANIFEST+7*4)^=1;
  if(packets==sequenceFault)Mem(TT_Q38_COHORT_SEQUENCE)^=1;
 }
 bool Call(unsigned entry){pc=entry;regs[1]=0x17ff80;regs[2]=0x70000;while(pc!=0x17ff80 && pc!=0x72060)StepScalar();return pc==0x17ff80;}
 void Capture(){assert(Call(capture));for(unsigned w=0;w<119;w++)Mem(TT_Q38_CONTINUE_RECEIPT+(992+w)*4)=actual[w];}
 void Verify(){
  assert(regs[10]==119 && packets==3351 && binds==119 && recipeBytes==285184 && !pending && !Mem(TT_Q38_CONTINUE_RECEIPT));
  unsigned at=0;for(unsigned s=0;s<3;s++)for(unsigned i=0;i<PioneerQ38ResidentContinueWords(s);i++,at++)
   assert(Mem(PioneerQ38ResidentContinueTarget(s)+i*4)==(s==0 && i>=64 && i<183?actual[i-64]:recipe[at]));
  assert(Mem(TT_Q38_CONTINUE_RECEIPT+1664*4)==119 && Mem(TT_Q38_CONTINUE_RECEIPT+1665*4)==19
   && Mem(TT_Q38_CONTINUE_RECEIPT+1666*4)==3351 && Mem(TT_Q38_CONTINUE_RECEIPT+1667*4)==7788 && minStack>=0x6e000);
 }
};
int main(int argc,char** argv){
 assert(argc==2 || (argc==3 && !strcmp(argv[2],"--smoke")));setvbuf(stdout,nullptr,_IOLBF,0);unsigned faults=0;
 for(unsigned variant=0;variant<(argc==3?1U:2U);variant++){
  auto m=std::make_unique<ContinueAdmission>(argv[1],variant);m->Capture();bool passed=m->Call(m->load);
  if(!passed)fprintf(stderr,"ADMISSION_FAILURE pc%08x ra%08x packets%u binds%u recipeBytes%u r1664=%u r1665=%u r1666=%u r1667=%u\n",m->pc,m->regs[1],m->packets,m->binds,m->recipeBytes,
   m->Mem(TT_Q38_CONTINUE_RECEIPT+1664*4),m->Mem(TT_Q38_CONTINUE_RECEIPT+1665*4),m->Mem(TT_Q38_CONTINUE_RECEIPT+1666*4),m->Mem(TT_Q38_CONTINUE_RECEIPT+1667*4));
  assert(passed);m->Verify();
  printf("PASS ACTUAL recipe admission instructions boundary%u:119peer DATA identity/sequence/counter audits+19fixed recipe reads+119LIVE sequence substitutions/3351packets+7788flits/maxStack%u;synthetic predecessor/peer DATA ONLY,NOT actual-prefix/native/full-layer qualification\n",variant,0x70000-m->minStack);
 }
 if(argc==3)return 0;
 for(unsigned kind=0;kind<12;kind++){
  auto m=std::make_unique<ContinueAdmission>(argv[1]);m->Capture();
  if(kind<2)m->failPacket=kind?3351:1;
  else if(kind<4)m->sourceFault=kind==2?1:3351;
  else if(kind<6)m->counterFault=kind==4?1:3351;
  else if(kind<9)m->badPeer=kind-5;
  else if(kind==9)m->badRecipe=1;
  else if(kind==10)m->sequenceFault=3332;
  else m->headerFault=3333;
  assert(!m->Call(m->load) && !m->binds && !m->Mem(TT_Q38_CONTINUE_RECEIPT));faults++;
  printf("PASS ACTUAL admission DATA fault%u packets%u/no binding/publication/reset or following packet\n",kind,m->packets);
 }
 printf("PASS continuation DATA-only subroutine instruction faults%u;NO fake native permission or predecessor-execution claim\n",faults);
}
