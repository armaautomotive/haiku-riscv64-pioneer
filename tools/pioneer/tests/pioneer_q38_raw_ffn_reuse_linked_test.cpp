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

static Recipe MakeRecipe(unsigned index,unsigned layer){
 Recipe r{};unsigned role=R::Role(5+index),seq=100+R::PhaseDelta(5+index);
 r[0]=TT_Q38_MIXED_RECURRENT_MATRIX;r[1]=1;r[2]=67108864;r[3]=3;r[4]=0x10000000;
 r[6]=seq;r[7]=119;r[8]=PioneerQ38ProjectionRoute(3);r[9]=129;r[10]=PioneerQ38MixedRecurrentOffset(role);
 r[11]=17408;r[12]=17408*4;r[13]=TT_Q38_DENSE_MATRIX_ROWS;r[14]=5120;r[15]=12;
 for(unsigned i=16;i<40;i++)r[i]=0x1000+i;
 r[40]=23;r[41]=248319;r[42]=layer;r[43]=role;r[44]=119;r[45]=1;
 const uint32_t h[]={TT_Q38_MIXED_RECURRENT_MATRIX,1,1,12,5120,119,seq,TT_Q38_DENSE_MATRIX_INPUT,1};
 std::copy(h,h+9,r.begin()+64);
 for(unsigned bank=0;bank<7;bank++)r[1151+bank]=PioneerQ38ProjectionRoute(bank);
 unsigned row=0,address=0x04000000+index*0x04000000;
 for(unsigned w=0;w<119;w++){
  unsigned groups=9+unsigned(w<17),i=80+w*8;
  r[i]=address;r[i+2]=groups;r[i+4]=r[1151];r[i+5]=w+1;
  r[1032+w]=row;row+=groups*16;address+=groups*20*2304;
 }
 assert(!r[53] && PioneerQ38MixedRecurrentRecipe(r.data(),r.data()+64,r.data()+1151,
  reinterpret_cast<const uint32_t(*)[8]>(r.data()+80),r.data()+1032));
 return r;
}

struct Linked:Machine {
 // Exact instruction addresses in the SHA-pinned compact V2 ELF.
 enum {Matrix=0x56750,Phase=0x56724,Seal=0x56658,Run=0x5193c,Job=0x4a5c8,
  Fault=0x60000,Return=0x60004,Private=0x6f800,Sp=0x6f700,Frame=Private+768};
 const std::array<std::vector<uint8_t>,2> rom;
 std::vector<uint32_t> words=std::vector<uint32_t>(0x180000/4);
 unsigned seals=0,projections=0,phases=0;
 Linked(const std::string& dir):rom{Read((dir+"/text.bin").c_str()),Read((dir+"/conv.bin").c_str())}{}
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
   || in(R::Rows,119*4) || in(R::Banks,32) || in(R::Scratch,256));
  Data(a)=v;
 }
 void Prepare(unsigned layer){
  for(unsigned i=0;i<2;i++){
   auto r=MakeRecipe(i,layer);
   for(unsigned j=0;j<r.size();j++)Data(R::RecipeCopies+i*R::RecipeStride+j*4)=r[j];
  }
  Data(Private+(64+48)*4)=25;
  for(unsigned i=0;i<5120;i++)Data(R::PostNormalized+i*4)=0x3f800000+(i%128);
 }
 bool Dispatch(unsigned index,unsigned corrupt=64,bool corruptGate=false){
  regs[1]=Return;regs[2]=Sp;regs[10]=Private;regs[11]=index;
  regs[12]=3;regs[13]=10+index;pc=Matrix;
  while(pc!=Return && pc!=Fault){
   if(pc==Seal){assert(regs[10]==3);seals++;pc=regs[1];}
   else if(pc==Phase){
    assert(regs[10]==Private && regs[11]==10+index);phases++;
    if(corrupt<64)Data((corruptGate?unsigned(R::RecipeCopies):unsigned(Frame))+corrupt*4)^=1;
    pc=regs[1];
   }else if(pc==Run){
    assert(regs[10]==Frame && Data(Frame+43*4)==R::Role(5+index));
    assert(Data(Frame+53*4)==R::Input);
    for(unsigned i=0;i<5120;i++)assert(Data(R::Input+i*4)==Data(R::PostNormalized+i*4));
    projections++;regs[10]=Data(Frame+11*4);pc=regs[1];
   }else Step();
  }
  return pc==Return;
 }
 void Jobs(unsigned index){
  for(unsigned word=0;word<119*8;word++)Data(R::PartsShadow+word*4)=Data(R::Parts+word*4);
  for(unsigned w=0;w<119;w++){
   regs[1]=Return;regs[2]=Sp;regs[10]=Frame;regs[11]=w;regs[12]=110+w;pc=Job;
   while(pc!=Return){assert(pc!=Fault);Step();}
   const unsigned field=R::Scratch+16*4;
   assert(Data(R::Scratch+4)==110+w && Data(R::Scratch+3*4)==w+1);
   assert(Data(field)==0x31483851 && Data(field+4)==12 && Data(field+5*4)==5120);
   assert(Data(field+6*4)==Data(R::Parts+w*32) && Data(field+9*4)==unsigned(!index));
  }
 }
};

int main(int argc,char** argv){
 assert(argc==2);
 unsigned layers=0;
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3){
  auto m=std::make_unique<Linked>(argv[1]);m->Prepare(layer);
  assert(m->Dispatch(0));if(!layers)m->Jobs(0);
  assert(m->Dispatch(1));if(!layers)m->Jobs(1);
  assert(m->projections==2 && m->seals==4 && m->phases==2);layers++;
 }
 assert(layers==48);
 unsigned refused=0;
 for(bool gate:{false,true})for(unsigned field:{3U,4U,6U,8U,9U,14U,15U,16U,21U,32U,39U,40U,41U,42U,43U,53U}){
  auto m=std::make_unique<Linked>(argv[1]);m->Prepare(0);
  assert(m->Dispatch(0));const unsigned projections=m->projections;
  assert(!m->Dispatch(1,field,gate) && m->projections==projections);refused++;
 }
 printf("PASS linked FFN Matrix gate/up on %u layers, 238 actual job frames, %u late identity faults before projection; synthetic endpoints, no card or timing\n",layers,refused);
}
