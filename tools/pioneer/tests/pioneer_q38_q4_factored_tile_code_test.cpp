/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#define PIONEER_Q38_FACTORED_TILE_FIXTURE_ONLY
#include "pioneer_q38_q4_factored_tile_test.cpp"
#include <sstream>
struct FactoredTileCode:Machine {
 FactoredTileMachine& m;std::vector<uint8_t> code;std::array<unsigned,2048> stack{};
 unsigned sync=0,direct=0,ports=0,sumStores=0,minSP=0x70000,result=0xa7a7a7a7;
 FactoredTileCode(FactoredTileMachine& machine,std::vector<uint8_t> bytes):m(machine),code(std::move(bytes)){
  assert(!code.empty() && code.size()<=8192 && !(code.size()&3));
  for(unsigned i=0;i<code.size()/4;i++)l1[i]=Word(code,i*4);
 }
 bool CodeAddress(unsigned a)const override{return a>=0x70000 && a-0x70000<code.size() && !(a&3);}
 bool InlineTensix()const override{return true;}
 unsigned Index(unsigned a)override{assert(CodeAddress(a));return (a-0x70000)/4;}
 void Instruction(unsigned w)override{
  direct++;
  if(w==0xa2101816){m.Drain();return;}
  if(w==0x4500017e){sync=1;return;}
  m.Op(w);
 }
 unsigned Load(unsigned a)override{
  assert(!(a&3));
  if(a>=0x6e000 && a<0x70000)return stack[(a-0x6e000)/4];
  if(CodeAddress(a))return Word(code,a-0x70000);
  if(a>=0xffef0000 && a<0xffef0380)return m.config[(a-0xffef0000)/4];
  if(a==0xffe000fc)return sync;
  if(a==0xffbd8800)return m.Read(a);
  assert(false && "No scalar learned/input payload reads permitted");return 0;
 }
 void Store(unsigned a,unsigned v)override{
  assert(!(a&3));
  if(a>=0x6e000 && a<0x70000){if(a<minSP)minSP=a;stack[(a-0x6e000)/4]=v;return;}
  if(a>=0xffef0000 && a<0xffef0380){m.Config((a-0xffef0000)/4,v);return;}
  if(a==0xffe40000){ports++;Instruction(v);return;}
  if(a==0xffe000fc){assert(!v);sync=0;return;}
  if(a==0x17ff80){assert(m.Clean() && m.nativeInputs%2==0 && m.sumReads==m.groups+1);sumStores++;result=v;return;}
  m.Write(a,v); // accepts ONLY computational constants/header addresses
 }
 void Call(unsigned entry,unsigned group=0){
  pc=entry;regs[1]=0xdead0000;regs[2]=0x70000;regs[10]=group;regs[11]=0x17ff80;
  for(unsigned r:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})regs[r]=0x5a000000|r;
  unsigned began=steps;while(pc!=0xdead0000){assert(steps-began<100000);Step();}
  assert(regs[2]==0x70000);
  for(unsigned r:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(regs[r]==(0x5a000000|r));
 }
};
int main(int argc,char** argv){
 assert(Round(0x3f808000)==0x3f800000); // old RNE recipe remains distinct
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);const auto code=Read(argv[1]);
 std::ifstream in(argv[2]);assert(in.good());std::string line;
 const char* names[]={"pioneer_q4_factored_tile_constants","pioneer_q4_factored_tile_begin","pioneer_q4_factored_tile_group"};unsigned entry[3]={};
 while(std::getline(in,line)){
  std::istringstream row(line);std::string a,type,name;row>>a>>type>>name;
  for(unsigned i=0;i<3;i++)if(name==names[i]){assert(type=="T" && !entry[i]);entry[i]=unsigned(std::stoul(a,nullptr,16));}
 }
 for(auto a:entry)assert(a);
 unsigned groups=0,words=0,maxStack=0,steps=0;
 for(unsigned trial=0;trial<8;trial++){
  TileFixture data(trial);auto m=std::make_unique<FactoredTileMachine>();data.Stage(*m);
  m->packDelay=100003;m->delay[0]=100019;m->delay[1]=7;FactoredTileCode cpu(*m,code);
  cpu.Call(entry[0]);const auto before=m->l1;cpu.Call(entry[1]);
  for(unsigned group=0;group<8;group++){
   cpu.Call(entry[2],group);assert(cpu.regs[10]==1 && cpu.result==data.Sum(group));const auto dot=m->QDots();
   for(unsigned r=0;r<32;r++){assert(dot[r]==data.Dot(r,group));words++;}
   for(unsigned a=0;a<m->l1.size();a++)if(a<Tile::Native+64 || a>=Tile::Native+1088)assert(m->l1[a]==before[a]);
   groups++;
  }
  const unsigned oldSteps=cpu.steps,oldDirect=cpu.direct,oldPorts=cpu.ports,oldStores=cpu.sumStores;
  cpu.Call(entry[2],8);assert(cpu.regs[10]==0 && cpu.direct==oldDirect && cpu.ports==oldPorts && cpu.sumStores==oldStores);
  assert(cpu.steps>oldSteps && m->Clean() && m->matrixOps==128 && m->packs==128 && m->sumReads==8 && cpu.sumStores==8);
  maxStack=std::max(maxStack,0x70000-cpu.minSP);steps+=cpu.steps;
 }
 assert(groups==64 && words==2048);
 printf("PASS_COMPILED_FACTORED_TILE groups%u qdotWords%u SAME_ORIGINAL_L1_BYTES_to_REAL_issued_UNPACK_SFPU_INT8_PACK_wait_BFP8_FPU_dot_sum strictUnchangedStep noScalarLearnedInputReads ABI_storeBounds touchedStack%u RV32steps%u;SIMULATED_FABRIC_NOT_NATIVE_HARDWARE_QUALITY_OR_TPS\n",groups,words,maxStack,steps);
}
