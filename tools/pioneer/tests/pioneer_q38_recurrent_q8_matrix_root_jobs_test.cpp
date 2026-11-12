/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#include "../PioneerQ38TokenLayout.h"
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "../PioneerQ38HybridRecurrentQ8MatrixPlan.h"
#include "../PioneerQ38RecurrentQ8MatrixManifest.h"
#include "../PioneerQ38RecurrentQ8MatrixWorkspace.h"
#include "../PioneerQ38DenseQ8MatrixWorkerLayout.h"
#include "../PioneerQ38DenseQ8MatrixRootFrame.h"
#include "../PioneerQ38RecurrentQ8MatrixRootJobs.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

struct IO {
 unsigned table[7][3]={},out[119][4]={},writes=0,reads=0,alterAt=0;
 unsigned Table(unsigned b,unsigned f){assert(b<7 && f<3);return table[b][f]^(++reads==alterAt?4096U:0U);}
 void Output(unsigned w,unsigned f,unsigned v){assert(w<119 && f<4);out[w][f]=v;writes++;}
};
namespace Worker=PioneerQ38DenseQ8MatrixWorkerLayout;
static std::array<uint32_t,64> Boot(unsigned worker){
 std::array<uint32_t,64>b={};b[0]=Worker::Magic;b[1]=1;b[2]=64;b[3]=worker;b[4]=Worker::Peer(worker);
 b[5]=b[16]=0x14000;b[6]=8192;b[7]=64;
 for(unsigned i=0;i<4;i++)b[8+i*2]=i+1;for(unsigned i=40;i<=50;i++)b[i]=Worker::BootWord(i);
 assert(Worker::BootShape(b.data()));return b;
}
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
 using C=PioneerQ38CanonicalModelPlan;using P=PioneerQ38HybridRecurrentQ8MatrixPlan;using M=PioneerQ38RecurrentQ8MatrixManifest;
 using Package=PioneerQ38RecurrentQ8MatrixWorkspace;
 auto source=std::make_unique<C>();P::Proposal proposal;assert(source->Build() && P::Build(*source,proposal));
 std::array<uint32_t,M::Words> manifest={};assert(M::Build(*source,proposal,manifest.data(),manifest.size()));
 std::array<uint32_t,Package::Words> records={};assert(Package::Build(*source,proposal,records.data(),records.size()));
 for(unsigned record=0;record<Package::Records;record++)for(unsigned i=0;i<Package::RecordWords;i++)
  assert(records[record*Package::RecordWords+i]==(i<21?manifest[record*21+i]:0));
 auto original=manifest;auto changed=proposal;changed.parts[0].offset+=4096;
 auto originalRecords=records;
 assert(!Package::Build(*source,changed,records.data(),records.size()) && records==originalRecords);
 assert(!Package::Build(*source,proposal,records.data(),records.size()-1) && records==originalRecords);
 assert(!Package::Build(*source,proposal,reinterpret_cast<uint32_t*>(proposal.parts.data()),Package::Words));
 assert(!M::Build(*source,changed,manifest.data(),manifest.size()) && manifest==original);
 assert(!M::Build(*source,proposal,manifest.data(),manifest.size()-1) && manifest==original);
 assert(!M::Build(*source,proposal,reinterpret_cast<uint32_t*>(proposal.parts.data()),M::Words));
 for(unsigned l=0;l<64;l++)if(l%4!=3)for(unsigned role: {5U,6U,13U}){
  IO actual;unsigned parts=0;
  for(unsigned i=0;i<source->Count();i++){
   const auto* p=source->Part(i);const auto& q=proposal.parts[i];
   if(p->kind!=C::WeightKind || p->layer!=l || p->role!=role)continue;
   assert(q.encoding==P::RecurrentQ8Bfp8 && p->bank<7 && !actual.table[p->bank][0]);
   actual.table[p->bank][0]=C::Base+q.offset;actual.table[p->bank][1]=p->firstRow;
   actual.table[p->bank][2]=p->rows;parts++;
  }
  for(unsigned b=0;b<7;b++)for(unsigned f=0;f<3;f++)
   assert(actual.table[b][f]==records[((l-l/4)*3+(role==5?0:role==6?1:2))*Package::RecordWords+b*3+f]);
  assert(parts==7 && S::Build(actual,l,role) && actual.writes==119*4);
  Verify(actual,role==5?6144:role==6?10240:5120,role==13?208896:174080);
  for(unsigned w=0;w<119;w++){
   auto boot=Boot(w+1);uint32_t job[64]={};
   assert(PioneerQ38DenseQ8MatrixRootFrame::Build(actual.out[w],boot.data(),l,role,0,3,0,job));
   assert(Worker::FpuJob(job) && Worker::DenseJob(job) && Worker::JobShape(job,boot.data(),0,0));
   if(w==0){
    std::array<uint32_t,64> saved={};for(unsigned i=0;i<64;i++)saved[i]=job[i];
    auto bad=std::array<uint32_t,4>{actual.out[w][0],7,actual.out[w][2],actual.out[w][3]};
    assert(!PioneerQ38DenseQ8MatrixRootFrame::Build(bad.data(),boot.data(),l,role,0,3,0,job));
    assert(!PioneerQ38DenseQ8MatrixRootFrame::Build(actual.out[w],boot.data(),l,role,8192,3,0,job));
    for(unsigned i=0;i<64;i++)assert(job[i]==saved[i]);
   }
   job[18]=role==13?5120:6144;assert(!Worker::JobShape(job,boot.data(),0,0));
  }
  for(unsigned w=1;w<=(role==13?24U:20U);w++){
   auto boot=Boot(w);uint32_t job[64]={};job[0]=Worker::Magic;job[1]=1;job[2]=5;job[3]=w;
   job[4]=boot[4];job[5]=l;job[7]=3;for(unsigned i=8;i<16;i++)job[i]=boot[i];
   job[16]=Worker::Producer::Magic;job[17]=256;job[18]=512;job[19]=3;job[20]=12;
   job[28]=role;job[29]=role==13?6144:5120;job[30]=Worker::Producer::Format;job[31]=Worker::Producer::Witness;
   assert(Worker::DenseProducerJob(job) && Worker::JobShape(job,boot.data(),0,0));
   job[29]++;assert(!Worker::JobShape(job,boot.data(),0,0));
  }
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
