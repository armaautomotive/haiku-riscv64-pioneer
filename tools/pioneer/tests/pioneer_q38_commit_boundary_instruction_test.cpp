/* SPDX-License-Identifier: MIT */
// ACTUAL linked RV32 CommitState and its seal/transport bodies. The private
// predecessor, numeric pairs, histories and NoC completion below are explicitly
// SYNTHETIC unit inputs, NOT a genuine completed model/native owner or timing.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include "../PioneerQ38TokenMetadata.h"
#include "../PioneerQ38TokenLayout.h"
#include "../PioneerQ38TokenStatePlan.h"
#include "../PioneerQ38GlobalHeadPlan.h"
#include "../PioneerQ38TokenCommitBoundaryLayout.h"
namespace C=PioneerQ38GlobalChainLayout;namespace R=C::R;
namespace T=PioneerQ38TokenLayout;namespace S=PioneerQ38TokenStateLayout;
namespace F=PioneerQ38GlobalFeedbackLayout;namespace M=PioneerQ38TokenMetadata;
struct CommitMachine:Machine {
#include "actual_token_commit_private.inc"
 enum {Parent=0x6f800,Stack=0x6e000,Return=0x60004,Header=0x17e000};
 struct Image {std::vector<uint8_t> text,conv,terminal,bf16;unsigned commit,input,feedback;bool optimized;};
 struct Segment {unsigned route,address,kind;std::vector<uint32_t> words;};
 struct Numeric {unsigned address,words;};
 static constexpr Numeric Numbers[]={{R::Original,5120},{R::OriginalCopy,5120},
  {R::Input,5120},{R::InputShadow,5120},{R::Native,2560},{R::NativeShadow,2560},
  {R::Scales,32},{R::ScaleShadow,32},{F::Local,5120},{F::Shadow,5120}};
 const Image& image;std::vector<uint32_t> memory;
 std::vector<Segment> dram;std::vector<std::array<uint32_t,10>> commands;
 std::array<uint32_t,10> command={};std::array<uint32_t,16> source={};
 std::array<uint32_t,8> counters={};
 unsigned transfers=0,packets=0,copied=0,inputScans=0,feedbackScans=0,minSp=Parent,minStore=Parent;
 bool feedback,reportFault=false;
 static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
 uint32_t& Data(unsigned address){assert(!(address&3) && address>=Stack && address<0x180000);return memory[address/4];}
 uint32_t& State(unsigned i){assert(i<64);return Data(Parent+(192+i)*4);}
 uint32_t& Phase(){return Data(Parent+256*4);}
 void Add(unsigned route,unsigned address,unsigned kind,const uint32_t* words,unsigned count){
  assert(count && !(address&3));
  for(const auto& s:dram)assert(s.route!=route || address+count*4<=s.address || s.address+s.words.size()*4<=address);
  dram.push_back({route,address,kind,std::vector<uint32_t>(words,words+count)});
 }
 CommitMachine(const Image& code,bool feed):image(code),memory(0x180000/4,0),feedback(feed){
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
  std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
  const unsigned bank=3,position=33,token=17,workspace=canonical->Base+canonical->Workspace(bank)->offset;
  auto package=std::make_unique<PioneerQ38GlobalChainPackage>();
  assert(package->Build(*canonical,63,bank,{1,2,3,4},sources.data(),bank,workspace+R::FinalOffset));
  Private owner={};std::copy(package->Boot(),package->Boot()+64,owner.boot);
  std::copy(package->Outer(),package->Outer()+128,owner.outer);
  assert(M::Outer(owner.outer,63,position,token,bank,workspace,owner.boot));
  assert(M::Frame(owner.outer,63,position,token,bank,workspace,owner.boot));
  owner.phase=4;owner.state[Bank]=bank;owner.state[Workspace]=workspace;owner.state[Layer]=63;
  owner.state[Completed]=64;owner.state[Profile]=64;owner.state[Position]=position;owner.state[Token]=token;
  owner.state[Committed]=position;owner.state[Slot]=position&1;owner.state[FrameReady]=1;
  owner.state[SessionReady]=1;owner.state[Limit]=feedback?35:34;owner.state[PromptCount]=2;owner.state[Initial]=248319;
  owner.state[Eos0]=248318;owner.state[Eos1]=248317;
  for(unsigned i=0;i<8;i++)owner.state[Counts+i]=counters[i]=i==6?9:7+i;
  std::copy(owner.boot,owner.boot+64,memory.begin()+R::Boot/4);
  std::copy(owner.outer,owner.outer+128,memory.begin()+R::Outer/4);Data(R::Doorbell)=owner.outer[48];
  std::copy(owner.boot+16,owner.boot+32,source.begin());
  memcpy(memory.data()+Parent/4,&owner,sizeof(owner));
  auto head=std::make_unique<PioneerQ38GlobalHeadPlan>();assert(head->Build(*canonical,bank,owner.boot));
  for(unsigned i=0;i<128;i++)Data(C::Head+i*4)=Data(Header+i*4)=head->Data()[i];
  uint32_t session[128]={};const unsigned prefix[]={T::Magic,1,128,bank,workspace,owner.state[Limit],2,248319};
  std::copy(prefix,prefix+8,session);std::copy(owner.boot+16,owner.boot+32,session+8);
  std::copy(owner.boot+8,owner.boot+16,session+24);session[33]=248318;session[34]=248317;session[35]=8192;
  assert(T::Session(session,owner.boot,bank,workspace));
  for(unsigned i=0;i<128;i++)Data(T::SessionLocal+i*4)=session[i];
  for(unsigned pair=0;pair<10;pair+=2)for(unsigned i=0;i<Numbers[pair].words;i++)
   Data(Numbers[pair].address+i*4)=Data(Numbers[pair+1].address+i*4)=0x3f000000U+(pair*17+i)%65536;
  for(unsigned layer=0;layer<64;layer++)if(!C::Dense(layer)){
   auto plan=std::make_unique<PioneerQ38TokenStatePlan>();assert(plan->Build(*canonical,layer,bank));
   const auto* d=plan->Data();unsigned address=0;assert(C::Address(workspace,layer,address));
   Add(PioneerQ38ProjectionRoute(bank),address+S::Offset,0,d,64);
   std::vector<uint32_t> history(d[6]/4);for(unsigned i=0;i<history.size();i++)history[i]=0x3e000000U+(layer*257+i)%65536;
   Add(PioneerQ38ProjectionRoute(bank),d[5],1,history.data(),history.size());
   for(unsigned n=0;n<7;n++){
    const auto* span=d+8+n*8;std::vector<uint32_t> empty(span[5]/4,0xa5a50000U+n);
    Add(PioneerQ38ProjectionRoute(span[0]),span[1],2,empty.data(),empty.size());
   }
  }
  for(unsigned reg=1;reg<32;reg++)regs[reg]=0x13570000U+reg*0x147U;
  regs[1]=Return;regs[2]=Parent;regs[27]=Parent;regs[10]=Header;regs[11]=unsigned(feed);pc=image.commit;
 }
 bool CodeAddress(unsigned address)const override{
  return !(address&3) && ((address>=0x20000 && address-0x20000<image.bf16.size())
   || (address>=0x40000 && address-0x40000<image.text.size())
   || (address>=0x64000 && address-0x64000<image.conv.size()) || (address>=0x60000 && address<0x60008));
 }
 uint32_t CodeWord(unsigned address)const{
  assert(CodeAddress(address));return address>=0x64000?Word(image.conv,address-0x64000):
   address>=0x60000?Word(image.terminal,address-0x60000):
   address>=0x40000?Word(image.text,address-0x40000):Word(image.bf16,address-0x20000);
 }
 unsigned Index(unsigned address)override{l1[8180]=CodeWord(address);return 8180;}
 uint32_t Load(uint32_t address)override{
  assert(!(address&3));if(address>=Stack && address<0x180000)return Data(address);
  if(address>=0xffb20000 && address<0xffb30000){
   const unsigned offset=address-0xffb20000;if(offset<40)return command[offset/4];
   for(unsigned i=0;i<16;i++)if(offset==Source(i))return source[i];
   for(unsigned i=0;i<8;i++)if(offset==Counter(i))return counters[i];
   for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return 0;
   if(offset>=0x240 && offset<0x2c0)return 0;
  }
  if(CodeAddress(address))return CodeWord(address);
  fprintf(stderr,"Unexpected synthetic-unit load %08x pc%08x\n",address,pc);assert(false);return 0;
 }
 void CompleteNoC(){
  const bool write=command[7]==0x212;const unsigned remote=command[write?3:0],local=command[write?0:3];
  const unsigned route=command[write?5:2],bytes=command[8];
  assert(!command[1] && !command[4] && !command[6] && !command[9] && bytes && !(bytes&3)
   && bytes<=16384-(remote&31) && command[write?2:5]==129 && local==R::Scratch+(remote&31));
  Segment* found=nullptr;
  for(auto& s:dram)if(s.route==route && remote>=s.address && bytes<=s.words.size()*4
   && remote-s.address<=s.words.size()*4-bytes){assert(!found);found=&s;}
  assert(found && (!write || found->kind==2));
  if(found->kind==0){assert(!write && bytes==256);transfers++;}else packets++;
  const unsigned at=(remote-found->address)/4;
  for(unsigned i=0;i<bytes/4;i++){
   if(write)found->words[at+i]=Data(local+i*4);else Data(local+i*4)=found->words[at+i];
  }
  if(write)copied+=bytes;
  for(unsigned i=0;i<8;i++)counters[i]+=i==(write?5U:1U)?(bytes+63)>>6:
   i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U;
  commands.push_back(command);
 }
 void Store(uint32_t address,uint32_t value)override{
  assert(!(address&3));
  if(address>=Stack && address<0x180000){
   // The only legal stores are the real private stack/frame, scratch,
   // readback shadow and descriptor. No numeric/header/progress mutation.
   assert((address<Parent+sizeof(Private)) || (address>=R::Scratch && address<R::Scratch+16384)
    || (address>=R::Weight && address<R::Weight+16384) || (address>=S::Local && address<S::Local+256));
   if(address<Parent && address<minStore)minStore=address;Data(address)=value;return;
  }
  if(address>=0xffb20000 && address<0xffb20028){command[(address-0xffb20000)/4]=value;return;}
  if(address==0xffb20040){assert(value==1);CompleteNoC();return;}
  fprintf(stderr,"Unexpected synthetic-unit store %08x pc%08x\n",address,pc);assert(false);
 }
 unsigned StepLimit()const override{return 1500000000U;}
 bool Run(std::unique_ptr<CommitMachine>* first=nullptr,std::unique_ptr<CommitMachine>* last=nullptr){
  unsigned prior=pc;
  std::array<unsigned,16> trail={};
  while(pc!=Return && pc!=0x60000){
   if(pc==image.input)inputScans++;if(pc==image.feedback)feedbackScans++;
   prior=pc;trail[steps%trail.size()]=pc;Step();assert(regs[2]>=Stack && regs[2]<=Parent);minSp=std::min(minSp,regs[2]);
   if(first && !*first && packets==1)*first=std::make_unique<CommitMachine>(*this);
   if(last && !*last && packets==2016)*last=std::make_unique<CommitMachine>(*this);
  }
  if(pc==0x60000){
   if(reportFault){
    fprintf(stderr,"Unexpected positive-unit fault from pc%08x steps%u packets%u transfers%u phase%u\n",prior,steps,packets,transfers,Phase());
    for(unsigned i=0;i<trail.size();i++)fprintf(stderr," %08x",trail[(steps+i)%trail.size()]);
    fprintf(stderr,"\n");
   }
   assert(Phase()==4 && State(Committed)==33 && State(Slot)==1);return false;
  }
  assert(Phase()==6 && State(Committed)==34 && !State(Slot) && transfers==48 && packets==2016
   && commands.size()==2064 && copied==5898240 && minSp>=Stack && minStore>=Stack);
  for(unsigned reg:{2U,3U,4U,8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})
   assert(regs[reg]==(reg==2 || reg==27?unsigned(Parent):0x13570000U+reg*0x147U));
  for(unsigned i=0;i<64;i++)assert(!Data(C::Progress+i*4)); // NO token publication
  for(unsigned i=0;i<2048;i++)Step();assert(pc==Return);return true;
 }
 void VerifyCopies()const{
  unsigned histories=0,destinations=0;
  for(unsigned n=0;n<dram.size();n++)if(dram[n].kind==0){
   const auto& descriptor=dram[n];const auto& history=dram[n+1];assert(history.kind==1);histories++;
   for(unsigned span=0;span<7;span++){
    const auto& d=dram[n+2+span];const unsigned first=descriptor.words[8+span*8+2]*3;
    assert(d.kind==2 && std::equal(d.words.begin(),d.words.end(),history.words.begin()+first));destinations++;
   }
  }
  assert(histories==48 && destinations==336);
 }
 void Evidence(const std::string& path)const{
  // Exact A/B equality data,NOT a model result or device-produced proof.
  std::ofstream out(path,std::ios::binary);assert(out.good());
  auto write=[&](const uint32_t* p,size_t n){out.write(reinterpret_cast<const char*>(p),n*4);assert(out.good());};
  for(const auto& c:commands)write(c.data(),c.size());
  for(const auto& s:dram){const uint32_t d[]={s.route,s.address,s.kind,unsigned(s.words.size())};write(d,4);write(s.words.data(),s.words.size());}
  write(memory.data()+Parent/4,sizeof(Private)/4);
  for(const auto& n:Numbers)write(memory.data()+n.address/4,n.words);
  for(unsigned address:{unsigned(C::Head),unsigned(Header),unsigned(T::SessionLocal)})write(memory.data()+address/4,128);
  for(unsigned address:{unsigned(R::Scratch),unsigned(R::Weight)})write(memory.data()+address/4,4096);
  write(memory.data()+S::Local/4,64);out.close();assert(out.good());
 }
};
int main(int argc,char** argv){
 // Metadata's shared header also declares generators outside this unit's
 // scope. Keep warnings enabled without claiming to exercise those bodies.
 (void)&M::Recipe;(void)&M::Conv;(void)&M::Gdn;(void)&M::Dense;
 const bool qgw8=argc==9 && std::string(argv[8])=="--qgw8-bf16";
 assert((argc==8 || qgw8) && std::string(argv[6])=="--synthetic-predecessor-and-noc");const std::string directory=argv[1];
 CommitMachine::Image image={Read((directory+"/text.bin").c_str()),Read((directory+"/conv.bin").c_str()),
  Read((directory+"/terminal.bin").c_str()),qgw8?Read((directory+"/bf16.bin").c_str()):std::vector<uint8_t>{},unsigned(std::stoul(argv[2],nullptr,16)),
  unsigned(std::stoul(argv[3],nullptr,16)),unsigned(std::stoul(argv[4],nullptr,16)),std::string(argv[5])=="1"};
 assert(std::string(argv[5])==(image.optimized?"1":"0"));unsigned valid=0,bad=0;
 for(bool feedback:{false,true}){
  auto initial=std::make_unique<CommitMachine>(image,feedback),run=std::make_unique<CommitMachine>(*initial);
  run->reportFault=true;
  std::unique_ptr<CommitMachine> first,last;assert(run->Run(&first,&last));run->VerifyCopies();valid++;
  run->Evidence(std::string(argv[7])+"-feedback"+std::to_string(unsigned(feedback))+".bin");
  first->reportFault=last->reportFault=false;
  assert(first && last && first->packets==1 && last->packets==2016);
  assert(run->inputScans==(image.optimized?(feedback?4U:2U):(feedback?2786U:1393U))
   && run->feedbackScans==(feedback?(image.optimized?2U:1393U):0U));
  printf("OBSERVED actual RV32 %s CommitState feedback%u instructions%u minSP%08x minStore%08x/"
   "2064 modeled commands/5898240 copied bytes/336 exact destinations/InputSeal%u FeedbackSeal%u/"
   "ABI+return+slotflip/no publication;SYNTHETIC predecessor+NoC,NOT cycles_or_TPS\n",
   image.optimized?"new":"old",unsigned(feedback),run->steps,run->minSp,run->minStore,run->inputScans,run->feedbackScans);fflush(stdout);
  // Full positive execution above creates these snapshots. Faults resume
  // the exact captured CPU/stack/CSR state; no fabricated function returns.
  for(unsigned family=0;family<(feedback?10U:8U);family++)for(bool edge:{false,true}){
   const auto n=CommitMachine::Numbers[family];const unsigned address=n.address+(edge?n.words-1:0)*4;
   for(const CommitMachine* base:{initial.get(),last.get()}){
    auto fault=std::make_unique<CommitMachine>(*base);fault->Data(address)^=1;assert(!fault->Run());bad++;
    if(base==initial.get())assert(!fault->packets && fault->transfers==(image.optimized?0U:1U));
   }
  }
  if(feedback)for(bool edge:{false,true})for(unsigned value:{0x7fc00000U,0x7f800000U}){
   for(const CommitMachine* base:{initial.get(),last.get()}){
    auto fault=std::make_unique<CommitMachine>(*base);const unsigned at=(edge?5119:0)*4;
    fault->Data(F::Local+at)=fault->Data(F::Shadow+at)=value;
    assert(!fault->Run());bad++; // equal mirrors STILL reject NaN/Inf
   }
  }
  // Metadata/counter/source/command/readback damage at actual final copy.
  for(unsigned kind=0;kind<7;kind++){
   auto fault=std::make_unique<CommitMachine>(*last);
   switch(kind){
    case 0:fault->Data(C::Head)^=1;break;
    case 1:fault->Data(S::Local)^=1;break;
    case 2:fault->source[15]^=1;break;
    case 3:fault->counters[0]^=1;break;
    case 4:fault->command[9]^=1;break;
    case 5:fault->Data(R::Scratch)^=1;break;
    case 6:fault->Data(R::Outer)^=1;break;
   }
   assert(!fault->Run());bad++;
  }
  if(image.optimized){
   auto fault=std::make_unique<CommitMachine>(*first);fault->Data(R::Original)^=1;
   assert(!fault->Run() && fault->packets==2016 && fault->transfers==48);bad++;
  }
 }
 printf("PASS ACTUAL linked RV32 CommitState+StateSeal+InputSeal+FeedbackSeal+HeadSeal+Scope+Packet+Transfer/"
  "%u complete transactions/%u entry+final-copy+mid-copy faults/real readback loops+counter guards+ABI+slotflip/"
  "SYNTHETIC canonical private predecessor+finite pairs+histories+immediate NoC completion;"
  "NOT full-model/numerical/native-lifetime/physical/Work/cycles/TPS qualification\n",valid,bad);
}
