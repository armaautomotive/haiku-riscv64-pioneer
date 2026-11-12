/* SPDX-License-Identifier: MIT */
// Actual RV32 probe instructions with synthetic memory, not a ROOT lifecycle.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#include "../PioneerQ38DenseQ8MatrixRootJobs.h"
#include <array>
#include <cstdlib>
#include <map>
struct HostIO {
 std::array<uint32_t,21> table={};std::array<uint32_t,119*4> output={};unsigned writes=0;
 unsigned Table(unsigned bank,unsigned field){return table[bank*3+field];}
 void Output(unsigned job,unsigned field,unsigned value){output[job*4+field]=value;writes++;}
};
struct Probe:Machine {
 std::vector<uint8_t> code;std::map<uint32_t,uint32_t> memory;unsigned writes=0,reads=0,alterAt=0,minStack=0x72000;
 static constexpr unsigned TableBase=0x80000,OutputBase=0x81000,Return=0x60000;
 explicit Probe(const char* path):code(Read(path)){}
 unsigned StepLimit()const override{return 100000;}
 bool CodeAddress(uint32_t a)const override{return a>=0x40000 && a-0x40000<code.size();}
 unsigned Index(uint32_t a)override{assert(!(a&3) && CodeAddress(a));l1[8191]=Word(code,a-0x40000);return 8191;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(CodeAddress(a))return l1[Index(a)];
  if(a>=TableBase && a<TableBase+21*4){reads++;return memory.at(a)^(reads==alterAt?4096U:0U);}
  assert(a>=0x70000 && a<0x72000);return memory[a];
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(a>=OutputBase && a<OutputBase+119*16){writes++;memory[a]=value;return;}
  assert(a>=0x70000 && a<0x72000);if(a<minStack)minStack=a;memory[a]=value;
 }
 void Step(){
  assert(CodeAddress(pc));const uint32_t op=Word(code,pc-0x40000);
  const unsigned kind=op&127,high=op>>25,f3=(op>>12)&7,rd=(op>>7)&31,rs1=(op>>15)&31,rs2=(op>>20)&31;
  if(kind==0x33 && high==1 && f3!=0){
   assert(++steps<StepLimit());const uint32_t a=regs[rs1],b=regs[rs2];uint32_t result=0;
   if(f3==3)result=uint32_t((uint64_t(a)*b)>>32);
   else if(f3==5)result=b?a/b:UINT32_MAX;
   else if(f3==7)result=b?a%b:a;
   else assert(false);
   if(rd)regs[rd]=result;regs[0]=0;pc+=4;return;
  }
  Machine::Step();
 }
 bool Call(const HostIO& input,unsigned entry,unsigned layer,unsigned role){
  for(unsigned i=0;i<21;i++)memory[TableBase+i*4]=input.table[i];
  pc=entry;regs[1]=Return;regs[2]=0x72000;regs[10]=TableBase;regs[11]=OutputBase;regs[12]=layer;regs[13]=role;
  while(pc!=Return)Step();assert(regs[2]==0x72000);return regs[10]!=0;
 }
};
#ifndef PIONEER_Q38_ROOT_IMAGE_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==3);char* end=nullptr;unsigned entry=unsigned(std::strtoul(argv[2],&end,16));assert(end && !*end);
 for(unsigned role:{5U,6U,7U})for(unsigned shift=0;shift<7;shift++){
  HostIO host;unsigned first=0,units=role==5?384:32;
  for(unsigned stripe=0;stripe<7;stripe++){
   const unsigned b=(stripe+shift)%7,count=units/7+unsigned(stripe<units%7);
   host.table[b*3]=0x04000000+b*0x100000;host.table[b*3+1]=first;host.table[b*3+2]=count*32;first+=count*32;
  }
  assert(PioneerQ38DenseQ8MatrixRootJobs::Build(host,3,role));Probe machine(argv[1]);
  assert(machine.Call(host,entry,3,role) && machine.writes==host.writes && machine.minStack>=0x70000);
  for(unsigned i=0;i<host.writes;i++)assert(machine.memory.at(Probe::OutputBase+i*4)==host.output[i]);
  HostIO bad=host;bad.table[0]++;Probe malformed(argv[1]);assert(!malformed.Call(bad,entry,3,role) && !malformed.writes);
  Probe changed(argv[1]);changed.alterAt=22;assert(!changed.Call(host,entry,3,role) && !changed.writes);
 }
 puts("PASS emitted RV32 dense Q8 schedule, all role geometries and mutation refusal; SYNTHETIC MEMORY NO DEVICE");
}
#endif
