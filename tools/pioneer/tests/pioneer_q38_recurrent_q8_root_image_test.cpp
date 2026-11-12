/* SPDX-License-Identifier: MIT */
// Actual compiled RV32 controller against synthetic memory; not card execution.
#define PIONEER_Q38_ROOT_IMAGE_FIXTURE_ONLY 1
#include "pioneer_q38_dense_q8_root_image_test.cpp"
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_INSPECTION 1
#include "../PioneerQ38RecurrentQ8MatrixRootJobs.h"
int main(int argc,char** argv){
 assert(argc==3);char* end=nullptr;unsigned entry=unsigned(std::strtoul(argv[2],&end,16));assert(end && !*end);
 for(unsigned role:{5U,6U,13U})for(unsigned shift=0;shift<7;shift++){
  HostIO host;unsigned first=0,units=role==5?192:role==6?320:160;
  for(unsigned stripe=0;stripe<7;stripe++){
   const unsigned b=(stripe+shift)%7,count=units/7+unsigned(stripe<units%7);
   host.table[b*3]=0x04000000+b*0x100000;host.table[b*3+1]=first;host.table[b*3+2]=count*32;first+=count*32;
  }
  assert(PioneerQ38RecurrentQ8MatrixRootJobs::Build(host,0,role));Probe machine(argv[1]);
  assert(machine.Call(host,entry,0,role) && machine.writes==host.writes && machine.minStack>=0x70000);
  for(unsigned i=0;i<host.writes;i++)assert(machine.memory.at(Probe::OutputBase+i*4)==host.output[i]);
  HostIO bad=host;bad.table[0]++;Probe malformed(argv[1]);assert(!malformed.Call(bad,entry,0,role) && !malformed.writes);
  Probe changed(argv[1]);changed.alterAt=22;assert(!changed.Call(host,entry,0,role) && !changed.writes);
 }
 puts("PASS compiled RV32 recurrent matrix scheduler, all roles and mutation refusal; SYNTHETIC MEMORY NO DEVICE");
}
