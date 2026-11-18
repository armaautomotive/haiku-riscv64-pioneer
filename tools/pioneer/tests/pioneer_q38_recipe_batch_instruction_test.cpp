/* SPDX-License-Identifier: MIT */
// Execute the actual linked v4 and recipe-batch RV32 senders. Only the
// predecessor frame, GDDR and NoC completions are synthetic. No card access.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include "../PioneerQ38TokenMetadata.h"
#include "../PioneerQ38TokenStatePlan.h"

namespace G=PioneerQ38GlobalChainLayout;
namespace R=G::R;
namespace M=PioneerQ38TokenMetadata;

namespace {
static_assert(PioneerQ38MixedRecurrentCohortLayout::GdnWords==96*192,"use frozen v4 fixture geometry");
constexpr unsigned Recipe=5120, Scratch=16384, Parent=0x6f800, Stack=0x6e000;
constexpr unsigned TextPC=0x40000, FaultPC=0x60000, ReturnPC=0x60004, ConvPC=0x64000;
constexpr unsigned Csr=0xffb20000, Sentinel=0x5a5a5a5a;
// Common pinned v4 private frame; registers and offsets are exercised by
// the linked Owner and Scope on every command, not intercepted here.
enum State {Bank=0, Workspace=1, Layer=2, Completed=3, Profile=6, Counts=8,
 Position=16, Token=17, Committed=18, Slot=19, FrameReady=28};
struct OwnerFrame {uint32_t boot[64],outer[128],state[64],phase;};
static_assert(sizeof(OwnerFrame)==1028);
struct Image {
 std::vector<uint8_t> text,conv,terminal;
 unsigned recipes;
 bool batched;
 Image(const std::string& path,unsigned entry,bool batch):text(Read((path+"/text.bin").c_str())),
  conv(Read((path+"/conv.bin").c_str())),terminal(Read((path+"/terminal.bin").c_str())),
  recipes(entry),batched(batch){
  assert(!(recipes&3) && recipes>=ConvPC && recipes-ConvPC<conv.size());
  assert(terminal.size()==8 && Word(terminal,0)==0x810e006f && Word(terminal,4)==0x6f);
 }
};

struct Case {
 OwnerFrame owner={};
 std::vector<uint32_t> original,expected;
 unsigned from,output,first,count,route;
 bool gddr;
 Case(const PioneerQ38GlobalChainPackage& package,unsigned layer,unsigned bank,unsigned workspace,
  unsigned position,unsigned part){
  const bool dense=G::Dense(layer);
  assert(!part || dense);
  first=part?4:0;count=!dense?8:part?3:4;gddr=!dense || part;
  route=PioneerQ38ProjectionRoute(bank);
  const unsigned offset=!dense?unsigned(G::P::RecipesOffset):part?unsigned(G::DenseFfn):unsigned(G::DenseRecipes);
  unsigned root=0;assert(G::Address(workspace,layer,root));from=root+offset;
  std::copy_n(package.Boot(),64,owner.boot);std::copy_n(package.Outer(),128,owner.outer);
  const unsigned token=248319-position;
  assert(M::Outer(owner.outer,layer,position,token,bank,workspace,owner.boot));
  assert(M::Frame(owner.outer,layer,position,token,bank,workspace,owner.boot));
  output=!dense?owner.outer[57]:part?owner.outer[68]:unsigned(G::D::Recipes);
  owner.phase=2;owner.state[Bank]=bank;owner.state[Workspace]=workspace;
  owner.state[Layer]=owner.state[Completed]=layer;owner.state[Profile]=64;
  owner.state[Position]=owner.state[Committed]=position;owner.state[Token]=token;
  owner.state[Slot]=position&1;owner.state[FrameReady]=1;
  for(unsigned i=0;i<8;i++)owner.state[Counts+i]=0xfffffff0U+(i==6?2:i);
  original.resize(count*Recipe/4);
  for(unsigned i=0;i<count;i++)assert(package.Read(offset+i*Recipe,
   reinterpret_cast<uint8_t*>(original.data()+i*Recipe/4),Recipe));
  expected=original;
  for(unsigned i=0;i<count;i++)assert(M::Recipe(expected.data()+i*Recipe/4,layer,first+i,position,token));
 }
};

struct RecipeMachine:Machine {
 enum Fault {None, BadOwner, BadFrame, BadSource, Pending, PreReadback, NoCompletion,
  PostReadback, PostSource, BadRecipe, Misaligned, OutsideWorkspace};
 const Image& image;
 const Case& fixture;
 Fault injection;
 std::vector<uint32_t> input, output, local;
 std::array<uint32_t,8> counters={};
 std::array<uint32_t,10> command={};
 unsigned submitted=0,readBytes=0,writeBytes=0,localBytes=0,readWordTrace=0,commandWords=0;
 unsigned minSP=Parent,lostPolls=0;
 static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:
  i==13?0x150:i==14?0x154:0x16c;}
 static unsigned Counter(unsigned i){constexpr unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  assert(i<8);return offsets[i];}
 static unsigned Seed(unsigned r){return 0x13570000U+r*0x147U;}
 uint32_t& Data(unsigned address){assert(!(address&3) && address>=Stack && address<0x180000);return local[(address-Stack)/4];}
 bool Range(unsigned address,unsigned base,unsigned bytes)const{return address>=base && address-base<bytes;}
 uint32_t& PrivateWord(unsigned i){return Data(Parent+4*i);}
 RecipeMachine(const Image& code,const Case& c,Fault fault=None,unsigned badSlot=0):image(code),fixture(c),injection(fault),
  input(c.original),output(c.original.size(),Sentinel),local((0x180000-Stack)/4,Sentinel){
  memcpy(&Data(Parent),&c.owner,sizeof(c.owner));
  std::copy_n(c.owner.boot,64,&Data(R::Boot));
  std::copy_n(c.owner.outer,128,&Data(R::Outer));
  std::copy_n(c.owner.state+Counts,8,counters.begin());Data(R::Doorbell)=c.owner.outer[48];
  for(unsigned i=1;i<32;i++)regs[i]=Seed(i);
  regs[1]=ReturnPC;regs[2]=regs[27]=Parent;
  regs[10]=c.from;regs[11]=c.output;regs[12]=c.first;regs[13]=c.count;regs[14]=c.gddr;
  if(fault==BadOwner)regs[27]=0x70000;
  if(fault==BadFrame)Data(R::Outer)^=1;
  if(fault==BadRecipe){assert(badSlot<c.count);input[badSlot*Recipe/4]^=1;}
  if(fault==Misaligned)regs[10]++;
  if(fault==OutsideWorkspace)regs[10]=c.owner.state[Workspace]-64;
  pc=image.recipes;
 }
 unsigned StepLimit()const override{return injection==NoCompletion?6500000:3000000;}
 bool CodeAddress(unsigned a)const override{
  return !(a&3) && (Range(a,TextPC,image.text.size()) || Range(a,ConvPC,image.conv.size()) ||
   Range(a,FaultPC,image.terminal.size()));
 }
 unsigned Index(unsigned a)override{
  assert(CodeAddress(a));l1[8180]=a>=ConvPC?Word(image.conv,a-ConvPC):
   a>=FaultPC?Word(image.terminal,a-FaultPC):Word(image.text,a-TextPC);return 8180;
 }
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));
  if(Range(a,Csr,40)){
   const unsigned i=(a-Csr)/4;
   assert(commandWords==10 && i==readWordTrace%10);readWordTrace++;
   return command[i]^unsigned(i==9 && ((injection==PreReadback && !submitted) ||
    (injection==PostReadback && submitted==1)));
  }
  for(unsigned i=0;i<16;i++)if(a==Csr+Source(i))
   return fixture.owner.boot[16+i]^unsigned(i==15 && (injection==BadSource || (injection==PostSource && submitted)));
  for(unsigned i=0;i<8;i++)if(a==Csr+Counter(i))return counters[i];
  for(unsigned i=0;i<4;i++)if(a==Csr+0x40+i*0x800){
   if(!i && injection==NoCompletion && submitted){lostPolls++;return 1;}
   return 0;
  }
  if(Range(a,Csr+0x240,128))return unsigned(a==Csr+0x240 && injection==Pending);
  if(Range(a,Stack,Parent+sizeof(OwnerFrame)-Stack) || Range(a,R::Boot,256) || Range(a,R::Outer,512) ||
   Range(a,R::Scratch,Scratch) || a==R::Doorbell)return Data(a);
  if(CodeAddress(a))return l1[Index(a)];
  fprintf(stderr,"Unexpected recipe unit load %08x at %08x\n",a,pc);assert(false);return 0;
 }
 void Complete(){
  assert(readWordTrace==submitted*20+10 && commandWords==10);
  const bool write=command[7]==0x212;
  const unsigned address=command[write?3:0],bytes=command[8];
  assert(!command[1] && !command[4] && !command[6] && !command[9] &&
   (write || !command[7]) && bytes && bytes<=Scratch && !((bytes|address)&63));
  assert(command[write?2:5]==129 && command[write?5:2]==fixture.route && command[write?0:3]==R::Scratch);
  const unsigned total=fixture.count*Recipe;
  if(write){
   assert(fixture.gddr && address==fixture.output+writeBytes && readBytes==writeBytes+bytes);
   assert(bytes<=total-writeBytes);
   std::copy_n(&Data(R::Scratch),bytes/4,output.begin()+writeBytes/4);writeBytes+=bytes;
  }else{
   assert((fixture.gddr?writeBytes:localBytes)==readBytes && address==fixture.from+readBytes);
   const unsigned perBatch=(image.batched?3U:1U)*Recipe;
   assert(bytes==std::min(perBatch,total-readBytes));
   std::copy_n(input.begin()+readBytes/4,bytes/4,&Data(R::Scratch));readBytes+=bytes;
  }
  submitted++;
  if(injection!=NoCompletion)for(unsigned i=0;i<8;i++)counters[i]+=i==(write?5U:1U)?bytes/64:
   i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U;
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(Range(a,Stack,Parent-Stack) || Range(a,Parent+(192+Counts)*4,8*4) || Range(a,R::Scratch,Scratch)){
   Data(a)=value;return;
  }
  if(!fixture.gddr && Range(a,fixture.output,fixture.count*Recipe)){
   assert(a==fixture.output+localBytes && localBytes<readBytes);
   output[localBytes/4]=value;Data(a)=value;localBytes+=4;return;
  }
  if(Range(a,Csr,40)){
   const unsigned i=(a-Csr)/4;
   if(i==0){assert(readWordTrace==submitted*20);commandWords=0;}
   assert(i==commandWords++);command[i]=value;return;
  }
  if(a==Csr+0x40){assert(value==1);Complete();return;}
  fprintf(stderr,"Unexpected recipe unit store %08x at %08x\n",a,pc);assert(false);
 }
 bool Run(){
  std::array<unsigned,8> trail={};
  while(pc!=ReturnPC && pc!=FaultPC){
   trail[steps%trail.size()]=pc;Step();assert(regs[2]>=Stack && regs[2]<=Parent);minSP=std::min(minSP,regs[2]);
  }
  const bool returned=pc==ReturnPC;
  if(returned!=(injection==None)){
   fprintf(stderr,"Unexpected linked recipe exit pc=%08x arm%u layer%u bank%u pos%u slot%u fault%u commands%u steps%u:",
    pc,image.batched,fixture.owner.state[Layer],fixture.owner.state[Bank],fixture.owner.state[Position],fixture.first,
    unsigned(injection),submitted,steps);
   for(unsigned i=0;i<trail.size();i++)fprintf(stderr," %08x",trail[(steps+i)%trail.size()]);
   fputc('\n',stderr);
  }
  assert(returned==(injection==None));
  assert(lostPolls==(injection==NoCompletion?65537U:0U));
  for(unsigned i=0;i<sizeof(OwnerFrame)/4;i++)if(i<192+Counts || i>=192+Counts+8){
   uint32_t expected;memcpy(&expected,reinterpret_cast<const unsigned char*>(&fixture.owner)+i*4,4);
   assert(PrivateWord(i)==expected);
  }
  if(returned){
   assert(output==fixture.expected && input==fixture.original && readBytes==fixture.count*Recipe &&
    (fixture.gddr?writeBytes:localBytes)==readBytes && readWordTrace==submitted*20);
   for(unsigned i=0;i<8;i++)assert(PrivateWord(192+Counts+i)==counters[i]);
   for(unsigned r:{2U,3U,4U,8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})
    assert(regs[r]==(r==2 || r==27?Parent:Seed(r)));
  }
  // The real fault PC enters a global reporter outside this unit's ownership.
  // Normal return reaches the pinned inert fixture sentinel instead.
  if(returned){const unsigned at=steps;for(unsigned i=0;i<32;i++)Step();assert(steps==at+32 && pc==ReturnPC);}
  return returned;
 }
 void Published(unsigned recipes)const{
  const auto end=recipes*Recipe/4;
  assert((fixture.gddr?writeBytes:localBytes)==end*4 &&
   std::equal(output.begin(),output.begin()+end,fixture.expected.begin()) &&
   std::all_of(output.begin()+end,output.end(),[](uint32_t v){return v==Sentinel;}));
 }
};
}

int main(int argc,char** argv){
 assert(argc==5);
 (void)&M::Conv;(void)&M::Gdn;(void)&M::Dense;
 const Image old(argv[1],unsigned(std::stoul(argv[2],nullptr,16)),false);
 const Image next(argv[3],unsigned(std::stoul(argv[4],nullptr,16)),true);
 const auto model=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(model->Build());
 std::array<uint32_t,1920> sources;
 for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
 unsigned cases=0,refusals=0,maxOld=0,maxNew=0,oldPackets=0,newPackets=0;
 uint64_t oldSteps=0,newSteps=0;
 for(unsigned bank=0;bank<7;bank++){
  const unsigned workspace=model->Base+model->Workspace(bank)->offset;
  for(unsigned layer=0;layer<64;layer++){
   // All layers at bank 0. Cover both ends of both layer families and
   // every hardware routing selector at the other banks.
   if(bank && layer!=0 && layer!=3 && layer!=62 && layer!=63)continue;
   const auto package=std::make_unique<PioneerQ38GlobalChainPackage>();
   assert(package->Build(*model,layer,bank,{1,2,3,4},sources.data(),bank,workspace+0x600000));
   for(unsigned position:{0U,31U,8191U})for(unsigned part=0;part<(G::Dense(layer)?2U:1U);part++){
    if(position!=31 && layer!=0 && layer!=3 && layer!=62 && layer!=63)continue;
    const Case c(*package,layer,bank,workspace,position,part);
    const auto a=std::make_unique<RecipeMachine>(old,c),b=std::make_unique<RecipeMachine>(next,c);
    assert(a->Run() && b->Run() && a->output==b->output);
    assert(a->submitted==c.count*(c.gddr?2:1));
    assert(b->submitted==((c.count+2)/3)*(c.gddr?2:1));
    assert(a->counters[1]==b->counters[1] && a->counters[5]==b->counters[5]); // identical byte counts incl wrap
    if(!bank && position==31){oldPackets+=a->submitted;newPackets+=b->submitted;}
    oldSteps+=a->steps-32;newSteps+=b->steps-32;
    maxOld=std::max(maxOld,Parent-a->minSP);maxNew=std::max(maxNew,Parent-b->minSP);
    cases++;
    if(bank!=0 || position!=31 || (layer!=0 && layer!=3))continue;
    for(const Image* image:{&old,&next}){
     for(unsigned slot=0;slot<c.count;slot++){
      const auto m=std::make_unique<RecipeMachine>(*image,c,RecipeMachine::BadRecipe,slot);
      assert(!m->Run());m->Published(image->batched?slot/3*3:slot);refusals++;
     }
     for(auto fault:{RecipeMachine::BadOwner,RecipeMachine::BadFrame,RecipeMachine::BadSource,
       RecipeMachine::Pending,RecipeMachine::PreReadback,RecipeMachine::NoCompletion,
       RecipeMachine::PostReadback,RecipeMachine::PostSource,RecipeMachine::Misaligned,RecipeMachine::OutsideWorkspace}){
      const auto m=std::make_unique<RecipeMachine>(*image,c,fault);
      assert(!m->Run());m->Published(0);
      const bool commandStarted=fault==RecipeMachine::NoCompletion || fault==RecipeMachine::PostReadback || fault==RecipeMachine::PostSource;
      assert(m->submitted==unsigned(commandStarted));refusals++;
     }
    }
   }
  }
  printf("RECIPE_RV32 bank%u cumulative%u cases\n",bank,cases);fflush(stdout);
 }
 assert(cases==200 && refusals==90 && maxOld && maxNew && oldPackets==928 && newPackets==352);
 printf("PASS pinned linked RV32 TokenRecipes+Owner+Scope+Transfer: %u matched cases, %u refusals, "
  "complete-layer-pass commands %u -> %u, instructions %llu -> %llu across all cases, "
  "complete-call stack %u -> %u bytes; synthetic NoC, NOT device timing/TPS\n",
  cases,refusals,oldPackets,newPackets,static_cast<unsigned long long>(oldSteps),static_cast<unsigned long long>(newSteps),maxOld,maxNew);
}
