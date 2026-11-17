/* SPDX-License-Identifier: MIT */
// Execute the linked Matrix controller and its actual job writer. The prior
// seals and whole projection are synthetic endpoints, not a complete layer.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38MixedRecurrentBlockRootLayout.h"
#include <array>
#include <memory>
#include <string>

namespace R=PioneerQ38MixedRecurrentBlockRootLayout;
using Recipe=std::array<uint32_t,1280>;

static Recipe MakeRecipe(unsigned index,unsigned layer,unsigned first=5){
 Recipe r{};unsigned role=R::Role(first+index),seq=100+R::PhaseDelta(first+index);
 unsigned type=0,k=0,n=0;assert(PioneerQ38MixedRecurrentGeometry(layer,role,type,k,n));
 const unsigned jobs=n==48?3:119,blocks=k>>(type==8?5:8),tile=type==8?576:2304;
 r[0]=TT_Q38_MIXED_RECURRENT_MATRIX;r[1]=1;r[2]=67108864;r[3]=3;r[4]=0x10000000;
 r[6]=seq;r[7]=jobs;r[8]=PioneerQ38ProjectionRoute(3);r[9]=129;r[10]=PioneerQ38MixedRecurrentOffset(role);
 r[11]=n;r[12]=n*4;r[13]=TT_Q38_DENSE_MATRIX_ROWS;r[14]=k;r[15]=type;
 for(unsigned i=16;i<40;i++)r[i]=0x1000+i;
 r[40]=23;r[41]=248319;r[42]=layer;r[43]=role;r[44]=jobs;r[45]=1;
 const uint32_t h[]={TT_Q38_MIXED_RECURRENT_MATRIX,1,1,type,k,jobs,seq,TT_Q38_DENSE_MATRIX_INPUT,1};
 std::copy(h,h+9,r.begin()+64);
 for(unsigned bank=0;bank<7;bank++)r[1151+bank]=PioneerQ38ProjectionRoute(bank);
 unsigned row=0,address=0x04000000+index*0x04000000;
 for(unsigned w=0;w<jobs;w++){
  unsigned groups=n/16/jobs+unsigned(w<n/16%jobs),i=80+w*8;
  r[i]=address;r[i+2]=groups;r[i+4]=r[1151];r[i+5]=w+1;
  r[1032+w]=row;row+=groups*16;address+=groups*blocks*tile;
 }
 assert(!r[53] && PioneerQ38MixedRecurrentRecipe(r.data(),r.data()+64,r.data()+1151,
  reinterpret_cast<const uint32_t(*)[8]>(r.data()+80),r.data()+1032));
 return r;
}

struct Sites {unsigned job,matrix,phase,seal,run,gather,receipt;bool vector;};
static Sites LoadSites(const std::string& dir,bool control,bool vector){
 if(!vector)return {0x4a5c8,control?0x5672cU:0x56750U,control?0x56700U:0x56724U,
  control?0x56634U:0x56658U,control?0x51934U:0x5193cU,0,0,false};
 Sites s{};std::ifstream source(dir+"/sites.txt");
 assert(source>>std::hex>>s.job>>s.matrix>>s.phase>>s.seal>>s.run>>s.gather>>s.receipt);
 s.vector=true;return s;
}

struct Linked:Machine {
 // Exact addresses in the two SHA-pinned compact V2 ELFs.
 enum {Job=0x4a5c8,Fault=0x60000,Return=0x60004,Private=0x6f800,Sp=0x6f700,Frame=Private+768};
 const bool control;
 const Sites sites;
 const std::array<std::vector<uint8_t>,2> rom;
 std::vector<uint32_t> words=std::vector<uint32_t>(0x180000/4);
 unsigned seals=0,projections=0,phases=0;
 Linked(const std::string& dir,bool refetch=false,bool vector=false):control(refetch),sites(LoadSites(dir,refetch,vector)),
  rom{Read((dir+"/text.bin").c_str()),Read((dir+"/conv.bin").c_str())}{}
 const std::vector<uint8_t>* Section(unsigned address,unsigned& base)const{
  base=address>=0x64000?0x64000:0x40000;
  const auto& r=rom[base==0x64000];return address>=base && address-base+4<=r.size()?&r:nullptr;
 }
 bool CodeAddress(uint32_t address)const override{unsigned base;return Section(address,base);}
 unsigned StepLimit()const override{return 500000;}
 unsigned Index(uint32_t address)override{unsigned base;const auto* r=Section(address,base);assert(r);l1[8190]=Word(*r,address-base);return 8190;}
 uint32_t& Data(unsigned a){assert(!(a&3) && a>=0x6e000 && a<0x180000);return words.at(a/4);}
 uint32_t Load(uint32_t a)override{return Data(a);}
 void Store(uint32_t a,uint32_t v)override{
  auto in=[&](unsigned at,unsigned bytes){return a>=at && a-at<bytes;};
  assert(in(0x6e000,Sp-0x6e000) || in(Frame,256) || in(R::Header,256)
   || in(R::Input,20480) || in(R::MatrixHeader,64) || in(R::Parts,119*32)
   || in(R::Rows,119*4) || in(R::Banks,32) || in(R::Scratch,256)
   || (sites.vector && (a==Private+1024 || a==R::Status+20)));
  Data(a)=v;
 }
 void Prepare(unsigned layer,unsigned first=5,unsigned slots=2){
  for(unsigned i=0;i<slots;i++){
   auto r=MakeRecipe(i,layer,first);
   for(unsigned j=0;j<r.size();j++)Data(R::RecipeCopies+i*R::RecipeStride+j*4)=r[j];
  }
  Data(Private+(64+48)*4)=25;
  for(unsigned i=0;i<5120;i++)Data(R::PostNormalized+i*4)=0x3f800000+(i%128);
 }
 bool Dispatch(unsigned index,unsigned corrupt=64,bool corruptGate=false,unsigned first=5,unsigned count=3){
  const unsigned phase=first==5?10+index:2+index;
  regs[1]=Return;regs[2]=Sp;regs[10]=Private;regs[11]=index;
  regs[12]=count;regs[13]=phase;pc=sites.matrix;
  while(pc!=Return && pc!=Fault){
   if(pc==sites.seal){assert(regs[10]==count);seals++;pc=regs[1];}
   else if(pc==sites.phase){
    if(sites.vector)assert(Data(Private+1024)==phase && Data(R::Status+20)==phase);
    else assert(regs[10]==Private && regs[11]==phase);
    phases++;
    if(corrupt<64)Data((corruptGate?unsigned(R::RecipeCopies):unsigned(Frame))+corrupt*4)^=1;
    pc=regs[1];
   }else if(pc==sites.run){
    assert(regs[10]==Frame && Data(Frame+43*4)==R::Role(first+index));
    assert(Data(Frame+53*4)==R::Input);
    if(first==5)for(unsigned i=0;i<5120;i++)assert(Data(R::Input+i*4)==Data(R::PostNormalized+i*4));
    projections++;regs[10]=Data(Frame+11*4);pc=regs[1];
   }else Step();
  }
  return pc==Return;
 }
 void GatherFrontier(unsigned w,bool reuse){
  assert(sites.vector && sites.gather && sites.receipt);
  // Enter the actual inlined Gather prefix after its job-copy boundary.
  // Only the preceding stack locals and committed receipt marker are
  // supplied. Stop before the actual receipt matcher/dereference.
  regs[8]=Sp;
  if(control){regs[24]=Frame;regs[26]=w;}
  else {regs[18]=Frame;regs[6]=w;}
  Data(Sp-796)=R::PartsShadow+w*32-(Sp-704+0x77000);
  Data(Sp-792)=w*256;
  Data(0x118000+w*256)=0x33504d51;
  const unsigned groups=Data(R::PartsShadow+w*32+8),k=Data(Frame+14*4),type=Data(Frame+15*4);
  const unsigned blocks=k>>(type==8?5:8),tile=type==8?576:2304;
  assert(groups && (type==8 || type==12));
  const unsigned reads=groups*(blocks>>(type==8?2:1))+(reuse?0:2);
  const unsigned flits=groups*blocks*tile/64+(reuse?0:(k*2+((blocks*4+63)&~63U))/64);
  const uint32_t old[]={0xfffffffd,0xffffffdf,0xfffffffe,123,43,0xfffffffe,0xfffffffe,53};
  for(unsigned i=0;i<8;i++)Data(R::Counters+w*32+i*4)=old[i];
  pc=sites.gather;
  for(unsigned step=0;pc!=sites.receipt;step++){
   assert(pc!=Fault && step<1000);Step();
  }
  assert(regs[10]==Frame && regs[11]==w && regs[12]==Sp-704);
  const unsigned delta[]={reads,flits,reads+2,reads,2,5,reads+2,2};
  for(unsigned i=0;i<8;i++)assert(Data(regs[12]+4*i)==old[i]+delta[i]);
 }
 std::vector<uint32_t> Jobs(unsigned index,unsigned first=5){
  std::vector<uint32_t> jobs;
  jobs.reserve(119*64);
  for(unsigned word=0;word<119*8;word++)Data(R::PartsShadow+word*4)=Data(R::Parts+word*4);
  const unsigned workers=Data(Frame+7*4);
  for(unsigned w=0;w<workers;w++){
   regs[1]=Return;regs[2]=Sp;regs[10]=Frame;regs[11]=w;regs[12]=110+w;pc=sites.job;
   while(pc!=Return){assert(pc!=Fault);Step();}
   const unsigned field=R::Scratch+16*4;
   assert(Data(R::Scratch+4)==110+w && Data(R::Scratch+3*4)==w+1);
   assert(Data(field)==0x31483851 && Data(field+4)==Data(Frame+15*4) && Data(field+5*4)==Data(Frame+14*4));
   assert(Data(field+6*4)==Data(R::Parts+w*32) && Data(field+9*4)==unsigned(control || !index || first!=5));
   for(unsigned word=0;word<64;word++)jobs.push_back(Data(R::Scratch+word*4));
   if(sites.vector)GatherFrontier(w,!control && index && first==5);
  }
  return jobs;
 }
};

int main(int argc,char** argv){
 const bool vector=argc==4 && std::string(argv[3])=="--vector92";
 assert(argc==2 || argc==3 || vector);const bool pair=argc>2;
 unsigned layers=0;
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3){
  auto m=std::make_unique<Linked>(argv[1],false,vector);m->Prepare(layer);
  auto control=pair?std::make_unique<Linked>(argv[2],true,vector):nullptr;
  if(control)control->Prepare(layer);
  for(unsigned role=0;role<2;role++){
   assert(m->Dispatch(role));
   if(control){
    assert(control->Dispatch(role));
    for(unsigned word=0;word<64;word++)assert(m->Data(Linked::Frame+4*word)==control->Data(Linked::Frame+4*word));
   }
   if(!layers){
    const auto jobs=m->Jobs(role);
    if(control){
     auto expected=control->Jobs(role);assert(jobs.size()==expected.size());
     if(role)for(unsigned worker=0;worker<119;worker++)expected[worker*64+25]=0;
     assert(jobs==expected);
    }
   }
  }
  assert(m->projections==2 && m->seals==4 && m->phases==2);layers++;
  if(vector){
   // These phases use slot 1 too, but they are not gate/up. Run the
   // genuine earlier descriptors through both measured-controller builds.
   m->Prepare(layer,0,4);control->Prepare(layer,0,4);
   for(unsigned slot=0;slot<4;slot++){
    assert(m->Dispatch(slot,64,false,0,4) && control->Dispatch(slot,64,false,0,4));
    if(layers==1)assert(m->Jobs(slot,0)==control->Jobs(slot,0));
   }
   assert(m->projections==6 && m->seals==12 && m->phases==6);
  }
 }
 assert(layers==48);
 unsigned refused=0;
 for(int variant=1;variant<=(pair?2:1);variant++)for(bool gate:{false,true})for(unsigned field:{3U,4U,6U,8U,9U,14U,15U,16U,21U,32U,39U,40U,41U,42U,43U,53U}){
  auto m=std::make_unique<Linked>(argv[variant],variant==2,vector);m->Prepare(0);
  assert(m->Dispatch(0));const unsigned projections=m->projections;
  assert(!m->Dispatch(1,field,gate) && m->projections==projections);refused++;
 }
 printf("PASS linked FFN Matrix gate/up on %u layers, %u actual job frames, %u late identity faults before projection; synthetic endpoints, no card or timing\n",layers,238*(pair?2U:1U),refused);
 if(pair)puts("PASS exact full frames across both builds; only the up fetch bit differs among the 238 paired jobs");
 if(vector)puts("PASS earlier recurrent slots 0..3 on all 48 layers; both linked images still refetch");
 if(vector)puts("PASS linked gather counter frontiers for gate/up and earlier slots: only reuse subtracts 2 reads and 162 flits");
}
