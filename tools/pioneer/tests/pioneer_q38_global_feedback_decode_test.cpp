/* SPDX-License-Identifier: MIT */
// ISOLATED arithmetic capsule + geometry tests only. Synthetic function
// arguments are NOT a completed global result, acquired parent or launch.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "../PioneerQ38GlobalFeedbackLayout.h"
#include "../TenstorrentQ4K.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingPlan.h"
#include "pioneer_q38_global_output_original_source.h"
#include <array>
#include <memory>
namespace F=PioneerQ38GlobalFeedbackLayout;
struct FeedbackDecodeMachine:ScalarMachine {
 std::vector<uint8_t> text,terminal;unsigned entry,stores=0,selected=0;
 FeedbackDecodeMachine(const char* path,unsigned function):ScalarMachine(0,0),
  text(Read((std::string(path)+"/text.bin").c_str())),terminal(Read((std::string(path)+"/terminal.bin").c_str())),entry(function){
  for(unsigned i=0;i<5120;i++)memory[F::Local+i*4]=0x7fc00001;
  for(unsigned i=0;i<576;i++)memory[F::Raw+i*4]=0;
  regs[2]=0x70000;regs[27]=0x12345678;
 }
 bool CodeAddress(unsigned a)const override{return Range(a,0x40000,unsigned(text.size())) || Range(a,0x60000,8);}
 unsigned Index(unsigned a)override{
  assert(CodeAddress(a) && !(a&3));l1[8180]=a<0x60000?Word(text,a-0x40000):Word(terminal,a-0x60000);return 8180;
 }
 unsigned StepLimit()const override{return 20000000;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(CodeAddress(a))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,F::Raw,2304));assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3) && (Range(a,0x6e000,8192) || (selected<20 && Range(a,F::Local+selected*1024,1024))));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));else stores++;
  memory[a]=value;
 }
 bool Decode(unsigned block,unsigned lane,const uint8_t* tile){
  selected=block;for(unsigned i=0;i<576;i++){
   uint32_t value=0;for(unsigned b=0;b<4;b++)value|=uint32_t(tile[i*4+b])<<(b*8);memory[F::Raw+i*4]=value;
  }
  unsigned before=stores;regs[1]=0x60004;regs[10]=block;regs[11]=lane;pc=entry;
  while(pc!=0x60004)StepScalar();
  assert(regs[2]==0x70000 && regs[27]==0x12345678 && regs[10]<=1);
  bool okay=regs[10];if(okay)assert(stores-before==256);
  return okay;
 }
};
static unsigned Geometry(const PioneerQ38CanonicalModelPlan& c){
 unsigned cases=0;uint32_t plan[8],source[16];for(unsigned i=0;i<16;i++)source[i]=i+1;
 std::array<const PioneerQ38CanonicalPart*,7> stripes={};
 for(unsigned bank=0;bank<7;bank++){stripes[bank]=c.ProjectionStripe(64,0,bank);assert(stripes[bank]);}
 for(unsigned b=0;b<7;b++){
  unsigned workspace=c.Base+c.Workspace(b)->offset;
  for(unsigned token=0;token<248320;token++){
   assert(F::Describe(token,token&1?8191:0,b,workspace,plan));
   const PioneerQ38CanonicalPart* p=nullptr;
   for(const auto* part:stripes)if(token>=part->firstRow && token-part->firstRow<part->rows)p=part;
   assert(p && plan[0]==token && plan[1]==(token&1?8191U:0U) && plan[2]==b && plan[3]==workspace+0x700000
    && plan[4]==p->bank && plan[5]==c.Base+p->offset+((token-p->firstRow)/16)*46080 && plan[6]==(token-p->firstRow)%16 && plan[7]==20);cases++;
  }
  for(unsigned bank=0;bank<7;bank++)for(unsigned edge:{0U,15U,16U,PioneerQ38EmbeddingRows(bank)-1}){
   unsigned token=PioneerQ38EmbeddingFirst(bank)+edge;PioneerQ38EmbeddingPlan independent;
   assert(independent.Build(c,token,1,b,{1,2,3,4},source,17) && F::Describe(token,1,b,workspace,plan));const auto* d=independent.Descriptor();
   assert(plan[4]==d[10] && plan[5]==d[11] && plan[6]==d[13]);
  }
 }
 for(unsigned bad=0;bad<9;bad++){
  plan[0]=0xdecafbad;unsigned token=0,position=0,bank=0,workspace=c.Base+c.Workspace(0)->offset;uint32_t* out=plan;
  if(bad==0)token=248320;if(bad==1)position=8192;if(bad==2)bank=7;if(bad==3)workspace=0;
  if(bad==4)workspace|=1;if(bad==5)workspace=0xc0001000;if(bad==6)workspace=c.Base;if(bad==7)out=nullptr;
  if(bad==8)out=reinterpret_cast<uint32_t*>(~uintptr_t(0)-3);
  assert(!F::Describe(token,position,bank,workspace,out) && plan[0]==0xdecafbad);
 }
 return cases;
}
static void Tile(std::array<uint8_t,2304>& tile,unsigned block,unsigned trial){
 const uint16_t halves[]={0,0x8000,1,0x8001,0x3c00,0xbc00,0x7bff,0xfbff,0x0400,0x8400,0x3555,0xb555};
 for(unsigned lane=0;lane<16;lane++){
  for(unsigned byte=0;byte<144;byte++)tile[byte*16+lane]=uint8_t(byte*37+lane*71+block*19+trial*107);
  uint16_t d=halves[(block+lane+trial)%12],m=halves[(block*7+lane*3+trial)%12];
  tile[lane]=uint8_t(d);tile[16+lane]=uint8_t(d>>8);tile[32+lane]=uint8_t(m);tile[48+lane]=uint8_t(m>>8);
 }
}
int main(int argc,char** argv){
 assert(argc==3 || argc==4);setvbuf(stdout,nullptr,_IOLBF,0);std::ifstream symbols(argv[2]);unsigned entry=0;symbols>>std::hex>>entry;assert(entry);
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());unsigned geometry=Geometry(*c),cases=0;
 GlobalOutputOriginalSource original;if(argc==4)assert(original.Open(argv[3]) && original.Ready());
 for(unsigned bank=0;bank<7;bank++)for(unsigned end=0;end<2;end++)for(unsigned lane=0;lane<16;lane++){
  unsigned first=PioneerQ38EmbeddingFirst(bank)+(end?PioneerQ38EmbeddingRows(bank)-16:0);
  auto machine=std::make_unique<FeedbackDecodeMachine>(argv[1],entry);
  for(unsigned block=0;block<20;block++){
   std::array<uint8_t,2304> tile;if(argc==4)assert(original.EmbeddingTile(first,block,tile.data(),unsigned(tile.size())));else Tile(tile,block,bank+end);
   assert(machine->Decode(block,lane,tile.data()));uint8_t packed[144];float decoded[256];
   for(unsigned byte=0;byte<144;byte++)packed[byte]=tile[byte*16+lane];assert(PioneerDecodeQ4K(packed,decoded));
   for(unsigned i=0;i<256;i++)assert(machine->memory.at(F::Local+(block*256+i)*4)==Bits(decoded[i]));
  }
  assert(machine->stores==5120 && machine->minStack>=0x6e000);cases++;
 }
 unsigned refusals=0;
 for(unsigned bad=0;bad<6;bad++){
  auto machine=std::make_unique<FeedbackDecodeMachine>(argv[1],entry);std::array<uint8_t,2304> tile={};unsigned block=0,lane=0;
  if(bad==0)block=20;if(bad==1)lane=16;if(bad==2)block=~0U;if(bad==3)lane=~0U;
  if(bad==4)tile[16]=0x7c;if(bad==5)tile[48]=0x7e;
  assert(!machine->Decode(block,lane,tile.data()) && !machine->stores);refusals++;
 }
 if(argc==4)assert(original.Ready());
 printf("PASS feedback geometry%u/all tokens x7 destinations/independent canonical source coordinates;actual RETURNING Q4_K capsule%u vectors/5120 BIT-EXACT each/%u refusals/NO NoC or publication;%s;NOT acquired parent/full graph/token commit/native/physical/Work/TPS\n",
  geometry,cases,refusals,argc==4?"SAME OPEN full-SHA authenticated original GGUF raw bytes ONLY":"synthetic raw bytes");
}
