/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "../PioneerQ38HybridRecurrentQ8MatrixPlan.h"
#include "../PioneerQ38RecurrentQ8MatrixRootJobs.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

struct IO {
 unsigned table[7][3]={},out[119][4]={},writes=0,reads=0,alterAt=0;
 unsigned Table(unsigned b,unsigned f){assert(b<7 && f<3);return table[b][f]^(++reads==alterAt?4096U:0U);}
 void Output(unsigned w,unsigned f,unsigned v){assert(w<119 && f<4);out[w][f]=v;writes++;}
};
static IO Fixture(unsigned rows){
 IO io;unsigned next=0,units=rows/32;
 for(unsigned b=0;b<7;b++){
  unsigned n=units/7+unsigned(b<units%7);
  io.table[b][0]=0x04000000+b*0x08000000;io.table[b][1]=next*32;io.table[b][2]=n*32;next+=n;
 }
 return io;
}
static void Verify(const IO& io,unsigned rows,unsigned bandBytes){
 bool seen[320]={};
 for(unsigned w=0;w<119;w++){
  unsigned a=io.out[w][0],b=io.out[w][1],first=io.out[w][2],bands=io.out[w][3];
  assert(b<7 && bands && bands<=8 && first>=io.table[b][1] && !(first&31));
  unsigned offset=(first-io.table[b][1])/32;
  assert(offset+bands<=io.table[b][2]/32 && a==io.table[b][0]+offset*bandBytes);
  for(unsigned i=0;i<bands;i++){unsigned unit=first/32+i;assert(unit<rows/32 && !seen[unit]);seen[unit]=true;}
 }
 for(unsigned i=0;i<rows/32;i++)assert(seen[i]);
}
int main(){
 using S=PioneerQ38RecurrentQ8MatrixRootJobs;unsigned count=0;
 for(unsigned l=0;l<64;l++)if(l%4!=3)for(unsigned role: {5U,6U,13U}){
  unsigned rows=role==5?6144:role==6?10240:5120,bandBytes=role==13?208896:174080;
  IO io=Fixture(rows);assert(S::Build(io,l,role) && io.writes==119*4);
  Verify(io,rows,bandBytes);count++;
 }
 assert(count==144);
 // Exercise the actual canonical seven-bank partitions and converted offsets,
 // not just the balanced synthetic tables above.
 using C=PioneerQ38CanonicalModelPlan;using P=PioneerQ38HybridRecurrentQ8MatrixPlan;
 auto source=std::make_unique<C>();P::Proposal proposal;assert(source->Build() && P::Build(*source,proposal));
 for(unsigned l=0;l<64;l++)if(l%4!=3)for(unsigned role: {5U,6U,13U}){
  IO actual;unsigned parts=0;
  for(unsigned i=0;i<source->Count();i++){
   const auto* p=source->Part(i);const auto& q=proposal.parts[i];
   if(p->kind!=C::WeightKind || p->layer!=l || p->role!=role)continue;
   assert(q.encoding==P::RecurrentQ8Bfp8 && p->bank<7 && !actual.table[p->bank][0]);
   actual.table[p->bank][0]=C::Base+q.offset;actual.table[p->bank][1]=p->firstRow;
   actual.table[p->bank][2]=p->rows;parts++;
  }
  assert(parts==7 && S::Build(actual,l,role) && actual.writes==119*4);
  Verify(actual,role==5?6144:role==6?10240:5120,role==13?208896:174080);
 }
 for(unsigned bad=0;bad<8;bad++){
  IO io=Fixture(5120);
  if(bad==0)io.table[0][0]++;
  if(bad==1)io.table[0][1]++;
  if(bad==2)io.table[0][2]=0;
  if(bad==3)io.table[0][1]=io.table[1][1];
  if(bad==4)io.table[0][0]=0xc4000000;
  if(bad==5)io.alterAt=22;
  if(bad==6)io.table[0][2]-=32;
  if(bad==7)io.table[0][2]=0xffffffff;
  assert(!S::Build(io,0,13) && !io.writes);
 }
 IO io=Fixture(6144);assert(!S::Build(io,3,5) && !io.writes);
 assert(!S::Build(io,64,5) && !io.writes);assert(!S::Build(io,0,7) && !io.writes);
 std::printf("PASS 144 recurrent projection schedules; corrupt and changing tables rejected; NO DEVICE\n");
}
