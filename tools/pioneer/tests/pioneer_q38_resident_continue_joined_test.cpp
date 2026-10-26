/* SPDX-License-Identifier: MIT */
// First staged combined test: actual full predecessor+advance+admission.
// Stops at the REAL second-layer entry, NOT a completed second-layer claim.
// The emitted program contains that actual following-layer call, which the
// next full two-layer observer must qualify without rebuilding its peers.
#define PIONEER_Q38_ADVANCE_JOINED_MACHINE_ONLY
#include "pioneer_q38_resident_advance_joined_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentContinuePlan.h"
struct ContinueJoined:AdvanceFabric {
 std::unique_ptr<PioneerQ38ResidentContinuePlan> continuation=std::make_unique<PioneerQ38ResidentContinuePlan>();
 std::vector<uint8_t> continueCode;std::vector<uint32_t> recipe;
 std::array<uint32_t,119> initialSequence={};std::array<uint32_t,8> admissionBefore={};
 std::array<uint32_t,5120> incoming={};
 unsigned headerCopies=0,continueEntry=0,secondEntry=0,admissionPackets=0,admittedWords=0,bindings=0;
 unsigned failedPacket=0,counterPacket=0,sourcePacket=0,headerPacket=0,recipePacket=0;
 bool admitting=false,continueForks=false,continueForked=false,badStatus=false;
 ContinueJoined(const char* image,const char* peer,const OutputParts& geometry):AdvanceFabric(image,peer,geometry){
  assert(continuation->Build(*canonical,*plan,link,advancePlan));
  for(unsigned i=0;i<128;i++)Mem(TT_Q38_CONTINUE_MANIFEST+i*4)=continuation->Descriptor()[i];
  const auto* next=continuation->Next();
  recipe.insert(recipe.end(),next->Manifest(),next->Manifest()+TT_Q38_COHORT_WORDS);
  recipe.insert(recipe.end(),next->PostManifest(),next->PostManifest()+TT_Q38_OUTPUT_COHORT_WORDS);
  recipe.insert(recipe.end(),next->LayerManifest(),next->LayerManifest()+TT_Q38_RESIDENT_LAYER_WORDS);
  const auto* d=continuation->Descriptor();
  for(unsigned i=0;i<recipe.size();i++)Put(d[8],d[7]+TT_Q38_CONTINUE_RECIPE_OFFSET+i*4,recipe[i]);
  continueCode=Read((std::string(image)+"/continue.bin").c_str());assert(continueCode.size()==31992);
  continueEntry=Symbol(image,"q38_resident_continue_once");secondEntry=Symbol(image,"q38_resident_cohort_main");
 }
 bool ContinueCode(unsigned at)const{return Range(at,TT_Q38_CONTINUE_ROM,unsigned(continueCode.size()));}
 bool CodeAddress(uint32_t at)const override{return ContinueCode(at) || AdvanceFabric::CodeAddress(at);}
 unsigned Index(uint32_t at)override{if(ContinueCode(at)){l1[8173]=Word(continueCode,at-TT_Q38_CONTINUE_ROM);return 8173;}return AdvanceFabric::Index(at);}
 bool RangeData(unsigned at)const override{return AdvanceFabric::RangeData(at) || Range(at,0x60000,0x2400);}
 uint32_t Load(uint32_t at)override{return ContinueCode(at)?Word(continueCode,at-TT_Q38_CONTINUE_ROM):AdvanceFabric::Load(at);}
 void Store(uint32_t at,uint32_t value)override{
  if(Range(at,TT_Q38_CONTINUE_SHADOW,512)){
   assert(!packets && headerCopies<128 && at==TT_Q38_CONTINUE_SHADOW+headerCopies*4 && value==Mem(TT_Q38_CONTINUE_MANIFEST+headerCopies*4));
   headerCopies++;Mem(at)=value;return;
  }
  if(Range(at,TT_Q38_CONTINUE_RECEIPT,8192)){assert(!pending && !unknown);Mem(at)=value;return;}
  if(at==0x17ffc8 && value==TT_Q38_CONTINUE_FAULT){unknown=true;Mem(at)=value;return;}
  if(admitting && Range(at,TT_Q38_COHORT_MANIFEST+64*4,119*4)){
   assert(!pending && !unknown && admissionPackets==3351 && bindings<119
    && at==TT_Q38_COHORT_MANIFEST+(64+bindings)*4 && value==initialSequence[bindings]
    && value==NewPeer(bindings).memory.at(0x74144));bindings++;Mem(at)=value;return;
  }
  AdvanceFabric::Store(at,value);
 }
 void Submit()override{
  if(!admitting){AdvanceFabric::Submit();return;}
  packets++;admissionPackets++;if(admissionPackets==failedPacket){pending=true;return;}
  assert(!command[1] && !command[4] && !command[6] && !command[7] && !command[9] && command[5]==129);
  unsigned remote=command[0],route=command[2],target=command[3],bytes=command[8];const auto* d=continuation->Descriptor();
  if(route==d[8]){
   assert(admissionPackets>3332 && admissionPackets<=3351);
   unsigned section=admittedWords<TT_Q38_COHORT_WORDS?0:admittedWords<TT_Q38_COHORT_WORDS+TT_Q38_OUTPUT_COHORT_WORDS?1:2;
   unsigned start=section==0?0:section==1?TT_Q38_COHORT_WORDS:TT_Q38_COHORT_WORDS+TT_Q38_OUTPUT_COHORT_WORDS;
   unsigned length=std::min(16384U,(PioneerQ38ResidentContinueWords(section)-(admittedWords-start))*4);
   assert(remote==d[7]+TT_Q38_CONTINUE_RECIPE_OFFSET+admittedWords*4 && target==PioneerQ38ResidentContinueTarget(section)+(admittedWords-start)*4 && bytes==length);
   for(unsigned i=0;i<bytes/4;i++)Mem(target+i*4)=Gddr(route,remote+i*4);admittedWords+=bytes/4;
  }else {
   unsigned w=(admissionPackets-1)/28+1,p=(admissionPackets-1)%28;assert(w<=119 && route==PioneerQ38ConvPeer(w));auto& peer=NewPeer(w-1);
   unsigned expected=p==0?0x74140:p<3?0xffb13140:p==3?0xffb121b0:p<20?0xffb20000+SrcOffset(p-4):0xffb20000+Offset(p-20);
   assert(remote==expected && target==0x75000+(remote&31) && bytes==(p?4U:64U) && !peer.active && !peer.pending);
   if(!p){for(unsigned i=0;i<16;i++)Mem(target+i*4)=peer.memory.at(remote+i*4);if(badStatus && w==1)Mem(target)^=1;}
   else Mem(target)=p<3?peer.pc:p==3?0x46800:p<20?peer.source[p-4]:peer.Count(p-20);
  }
  Retire(bytes,false);
  if(admissionPackets==counterPacket)counts[0]++;
  if(admissionPackets==sourcePacket)source[11]^=1;
  if(admissionPackets==headerPacket)Mem(TT_Q38_CONTINUE_MANIFEST+7*4)^=1;
  if(admissionPackets==recipePacket)Mem(TT_Q38_COHORT_MANIFEST+3*4)^=1;
 }
 void EnterContinue(){
  assert(!admitting && headerCopies==128 && pc==continueEntry && !Mem(0x17ffc8));VerifyAdvance(false);admitting=true;
  for(unsigned w=0;w<119;w++)initialSequence[w]=NewPeer(w).memory.at(0x74144);
  for(unsigned i=0;i<8;i++)admissionBefore[i]=counts[i];
  for(unsigned i=0;i<5120;i++)incoming[i]=Gddr(manifest[9],manifest[8]+i*4);
  puts("ACTUAL_CONTINUE_ENTER:complete real layer+link+20QPD1+244matrices retained/ALL119WAIT+private sequences;NO ready flags/host products or relaunch");
  if(continueForks)ForkContinueFaults();
 }
 void ForkContinueFaults(){
  assert(admitting && !continueForked && !admissionPackets);continueForked=true;fflush(nullptr);
  for(unsigned kind=0;kind<9;kind++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    continueForks=false;
    if(kind==0)source[11]^=1;if(kind==1)Mem(TT_Q38_ADVANCE_RECEIPT)^=1;if(kind==2)Mem(TT_Q38_COHORT_SEQUENCE)^=1;
    if(kind==3)failedPacket=1;if(kind==4)counterPacket=1;if(kind==5)sourcePacket=1;
    if(kind==6)headerPacket=3333;if(kind==7)recipePacket=3351;if(kind==8)badStatus=true;
    assert(!RunAdmission() && unknown && !bindings && !Mem(TT_Q38_CONTINUE_RECEIPT));
    unsigned reads=nocReads,n=packets;Inert();assert(reads==nocReads && n==packets);
    printf("PASS ACTUAL_CONTINUE_PREFIX_FORK_FAULT%u packets%u/binding0/publication0/inert/no following ROOT IO/reset;actual predecessor retained,NOT native recovery\n",kind,admissionPackets);
    fflush(nullptr);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 }
 bool RunAdmission(){
  while(!(admitting && pc==secondEntry) && pc!=parkPc && !(unknown && l1[Index(pc)]==0x0000006f)){
   if(pc==linkEntry)EnterLink();
   if(pc==advanceEntry && !advancing){VerifyLink(false);advancing=true;}
   if(pc==continueEntry && !admitting)EnterContinue();
   StepRoot();
  }
  return !unknown && admitting && pc==secondEntry;
 }
 void VerifyAdmission(){
  assert(admitting && pc==secondEntry && !pending && !unknown && admissionPackets==3351 && admittedWords==TT_Q38_CONTINUE_RECIPE_WORDS
   && bindings==119 && !Mem(TT_Q38_CONTINUE_RECEIPT));
  for(unsigned i=0;i<8;i++)assert(uint32_t(counts[i]-admissionBefore[i])==(i==1?7788:i==0 || i==2 || i==3 || i==6?3351U:0U));
  unsigned index=0;for(unsigned s=0;s<3;s++)for(unsigned i=0;i<PioneerQ38ResidentContinueWords(s);i++,index++){
   unsigned expected=s==0 && i>=64 && i<183?initialSequence[i-64]:recipe[index];assert(Mem(PioneerQ38ResidentContinueTarget(s)+i*4)==expected);
  }
  for(unsigned i=0;i<512;i++)assert(Mem(TT_Q38_CONTINUE_RECEIPT+(128+i)*4)==Mem(TT_Q38_ADVANCE_RECEIPT+i*4));
  for(unsigned w=0;w<119;w++)assert(!NewPeer(w).active && NewPeer(w).pc==TT_Q38_PEER_WAIT_LOAD
   && NewPeer(w).memory.at(0x74144)==initialSequence[w] && Mem(TT_Q38_COHORT_SEQUENCE+w*4)==initialSequence[w]);
  for(unsigned role=0;role<4;role++){const auto* r=PioneerQ38ResidentAdvanceRecord(advancePlan.Descriptor(),role);
   for(unsigned row=0;row<r[4];row++)assert(Gddr(manifest[9],manifest[8]+r[6]+row*4)==observed[role][row]);}
  for(unsigned i=0;i<5120;i++)assert(Gddr(manifest[9],manifest[8]+i*4)==incoming[i]);
  assert(minStack>=0x6e000);
 }
};
#ifndef PIONEER_Q38_CONTINUE_JOINED_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 puts("BEGIN STAGED actual full predecessor+advance+ONCARD next-recipe admission;stops at second-layer ENTRY,NOT completed second layer/native/model/rate");
 OutputParts geometry(0,0);ContinueJoined f(argv[1],argv[2],geometry);f.continueForks=argc==4;
 if(!f.RunAdmission()){f.Diagnostic();fprintf(stderr,"CONTINUE_FAILURE ra%08x admissionPackets%u bindings%u admittedWords%u\n",f.regs[1],f.admissionPackets,f.bindings,f.admittedWords);return 1;}
 f.VerifyAdmission();
 printf("PASS STAGED actual full layer->link->next projections->ONCARD canonical recipe+119LIVE sequences/ALL16480products+5120input preserved/3351NoCreads+7788flits/maxStack%u;NO host products/reset/relaunch. Stopped at REAL second-layer ENTRY;following-layer execution/native/full64/token-rate qualification STILL OWED\n",0x70000-f.minStack);
}
#endif
