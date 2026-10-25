/* SPDX-License-Identifier: MIT */
// Actual NEW linked FFN ROM and ALL119 actual current resident peers.
// Component test with SYNTHETIC raw/normalized original INPUT BYTES,not a
// forged completed Conv/GDN producer or full new ROOT/native qualification.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_FFN_GRAPH_COOPERATIVE_FIXTURE_ONLY
#include "pioneer_q38_ffn_graph_cooperative_test.cpp"
#include "pioneer_q38_ffn_residual_graph_fixture.h"
struct LayerFfnPeer:GraphArithmeticPeer {
 std::vector<uint8_t> service,gdn;
 LayerFfnPeer(const char* path,unsigned worker,std::vector<uint32_t>& data)
  :GraphArithmeticPeer(path,worker,data,false,16384),service(Read((std::string(path)+"/service.bin").c_str())),gdn(Read((std::string(path)+"/gdn.bin").c_str())){
  assert(service.size()==10300 && gdn.size()==35396 && BootNew());
 }
 bool Extra(unsigned at)const{return Range(at,0x68000,unsigned(service.size())) || Range(at,0xb8000,unsigned(gdn.size()));}
 uint32_t ExtraWord(unsigned at)const{return Range(at,0x68000,unsigned(service.size()))?Word(service,at-0x68000):Word(gdn,at-0xb8000);}
 unsigned Index(uint32_t at)override{if(Extra(at)){l1[8190]=ExtraWord(at);return 8190;}return GraphArithmeticPeer::Index(at);}
 bool CodeAddress(uint32_t at)const override{return Extra(at) || GraphArithmeticPeer::CodeAddress(at);}
 uint32_t Load(uint32_t at)override{return Extra(at)?ExtraWord(at):GraphArithmeticPeer::Load(at);}
};
struct LayerFfnRoot:ResidualGraphFixture<GraphArithmeticRoot> {
 std::array<std::vector<uint8_t>,7> linked;
 static constexpr unsigned Bases[7]={0x6000,0x72000,0x64000,0x68000,0xb8000,0x158000,0x15c000};
 static constexpr unsigned Return=0x17ff80,Fault=0x72060;
 LayerFfnRoot(const char* image,const char* worker):ResidualGraphFixture<GraphArithmeticRoot>((std::string(image)+"/unchanged-ffn").c_str(),worker,true,0,false,0,0,false),
  linked{}{
  const char* names[]={"boot","text","conv","matrix","cohort","norm","ffn"};
  for(unsigned i=0;i<7;i++){linked[i]=Read((std::string(image)+"/"+names[i]+".bin").c_str());assert(!linked[i].empty() && !(linked[i].size()&3));}
  assert(linked[6].size()<0x22000);
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string line;unsigned entry=0;
  while(std::getline(symbols,line)){
   unsigned at=0;char type=0,name[256]={};
   if(sscanf(line.c_str(),"%x %c %255s",&at,&type,name)==3 && !strcmp(name,"q38_resident_layer_ffn_once"))entry=at;
  }
  assert(Range(entry,0x15c000,unsigned(linked[6].size())));pc=entry;regs[1]=Return;regs[2]=0x70000;
  for(unsigned i=0;i<5120;i++)gddr[i]=Bits(.5f+float((i*17)%127)/256.f);
  CaptureProduced();
  for(unsigned role=0;role<3;role++)expected[role].assign(role==2?5120:17408,0x7fc00000);
  for(unsigned w=1;w<=119;w++)actual.emplace_back(new LayerFfnPeer(worker,w,gddr));
 }
 unsigned LinkedSection(uint32_t at)const{for(unsigned i=0;i<7;i++)if(Range(at,Bases[i],unsigned(linked[i].size())))return i;return 7;}
 uint32_t LinkedWord(uint32_t at)const{unsigned i=LinkedSection(at);assert(i<7 && !(at&3));return Word(linked[i],at-Bases[i]);}
 unsigned Index(uint32_t at)override{
  // ALL actual linked ROM sections,including deduplicated shared functions.
  // Old QFG3 control bytes initialize the base TEST object ONLY;NONE of its
  // instructions may be fetched as a substitute for the new linked image.
  if(LinkedSection(at)<7){l1[2047]=LinkedWord(at);return 2047;}
  return ResidualGraphFixture<GraphArithmeticRoot>::Index(at);
 }
 bool CodeAddress(uint32_t at)const override{return LinkedSection(at)<7;}
 uint32_t Load(uint32_t at)override{return LinkedSection(at)<7?LinkedWord(at):ResidualGraphFixture<GraphArithmeticRoot>::Load(at);}
 void StepBlock()override{DispatchMachine::StepBlock();}
 bool RunComponent(){
  while(pc!=Return && pc!=Fault)StepBlock();
  if(pc==Fault){unknown=true;return false;}
  assert(minStack>=0x6e000 && captures==TT_Q38_FFN_GRAPH_WORDS);return regs[10]==5120;
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 puts("BEGIN ACTUAL new-image FFN component@15c000+ALL119 exact current resident peers;SYNTHETIC normalized/raw/learned INPUT BYTES,not completed-prefix/native/fullROOT proof");
 LayerFfnRoot root(argv[1],argv[2]);
 if(!root.RunComponent()){
  fprintf(stderr,"LAYER_FFN_COMPONENT_FAILURE pc%08x packets%u matrices%u producers%u\n",root.pc,root.packets,root.finishedMatrices,root.finishedProducers);return 1;
 }
 root.VerifyActual();root.VerifyResidual();
 unsigned matrices=0;uint64_t math=0;
 for(const auto& p:root.actual){assert(p->prepared==p->verified && p->pc==TT_Q38_PEER_WAIT_LOAD && !p->dotProbeReads);matrices+=p->matrixJobs;math+=p->arithmetic;}
 assert(root.finishedMatrices==357 && root.finishedProducers==88 && matrices==357);
 printf("PASS ACTUAL NEW full-image FFN ROM+ALL119 current resident peer programs:357matrixjobs/20input+68SiLUproducers/gate17408+up17408+down5120/final5120residual BITEXACT/allprivate sequences preserved/TensixInstructions%llu/maxROOTstack%u;NO CPU intermediate arithmetic,native/fullROOT/original-GGUF/silicon/token-rate grant\n",(unsigned long long)math,0x70000-root.minStack);
}
