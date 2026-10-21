/* SPDX-License-Identifier: MIT */
// ACTUAL emitted RV32/SFPU;ONLY NoC+GDDR endpoints are a simulated fabric.
// No native allocation,initialized owner,coordinator or throughput claim.
#define PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
#define PIONEER_Q38_DELTA_VECTOR_POST_SCALE
#define PIONEER_Q38_DELTA_VECTOR_IMAGE_MACHINE_ONLY
#include "pioneer_q38_delta_vector_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnShape.h"
struct ResidentGdnImage:DeltaVectorImage {
 std::array<uint32_t,64> descriptor={};
 std::array<uint32_t,8192> prior={},next={};
 uint32_t command[10]={},counts[8]={},seals[16]={};
 unsigned packets=0,commandWrites=0,nocReads=0,failPacket=0,badCounter=0,foreignCounter=0,sourceFault=0,descriptorFault=0,dropCommand=0;
 bool pending=false;
 static unsigned SourceOffset(unsigned i) {
  const unsigned offsets[]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c}; return offsets[i];
 }
 ResidentGdnImage(const char* image,unsigned trial=1,unsigned readBank=0,unsigned writeBank=6,unsigned worker=0,bool edge=false)
  :DeltaVectorImage(image,trial) {
  for (unsigned i=0;i<8192;i++) { prior[i]=memory.at(0x90000+i*4); memory[0x90000+i*4]=0x7fc00000; next[i]=0x7fc00000; }
  for (auto& word:command) word=0xdeadbeef; // never confuse last command word5 with physical identity
  for (auto& word:counts) word=0xfffffff8;
  for (unsigned i=0;i<16;i++) seals[i]=0xa5000000+i*0x13579; seals[0]=0x16005;
  descriptor[0]=TT_Q38_RGDN_MARKER; descriptor[1]=1; descriptor[2]=128; descriptor[3]=64;
  descriptor[4]=readBank; descriptor[5]=edge?0xc4000000-32768:0x04000000;
  descriptor[6]=writeBank; descriptor[7]=edge?0x04000000:0xc4000000-32768;
  descriptor[8]=17; descriptor[9]=worker; descriptor[10]=worker%3; descriptor[11]=worker%2;
  descriptor[12]=PioneerQ38SpanRoute(readBank); descriptor[13]=PioneerQ38SpanRoute(writeBank);
  descriptor[14]=32768; descriptor[15]=PioneerQ38ResidentGdnPeer(worker);
  for (unsigned i=0;i<16;i++) descriptor[16+i]=seals[i];
  for (unsigned i=32;i<40;i++) descriptor[i]=0x13570000+i;
  descriptor[40]=123; descriptor[41]=7;
  for (unsigned i=0;i<64;i++) memory[TT_Q38_RGDN_DESCRIPTOR+i*4]=descriptor[i];
  for (unsigned i=0;i<32;i++) memory[TT_Q38_RGDN_SUMMARY+i*4]=0xdeadbeef;
  assert(PioneerQ38ResidentGdnShape(descriptor.data()));
 }
 uint32_t Load(uint32_t at) override {
  if (Range(at,0xffb20000,0x2000)) {
   nocReads++; unsigned offset=at-0xffb20000;
   if (offset<40) return command[offset/4];
   for (unsigned i=0;i<4;i++) if (offset==0x40+i*0x800) return i?0:unsigned(pending);
   if (offset>=0x240 && offset<0x2c0) return 0;
   for (unsigned i=0;i<16;i++) if (offset==SourceOffset(i)) return seals[i];
   const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for (unsigned i=0;i<8;i++) if (offset==offsets[i]) return counts[i];
   assert(false && "unqualified resident GDN NoC read");
  }
  if (Range(at,TT_Q38_RGDN_DESCRIPTOR,256) || Range(at,TT_Q38_RGDN_SUMMARY,128)) return memory.at(at);
  return DeltaVectorImage::Load(at);
 }
 void Submit() {
  packets++; if (packets==failPacket) { pending=true; return; }
  bool write=packets>2; unsigned half=(packets-1)%2,remote=descriptor[write?7:5]+half*16384,local=(write?0xa0000:0x90000)+half*16384;
  const uint32_t expected[]={write?local:remote,0,write?descriptor[15]:descriptor[12],write?remote:local,0,write?descriptor[13]:descriptor[15],0,write?0x212U:0,16384,0};
  assert(packets<=4); for (unsigned i=0;i<10;i++) assert(command[i]==expected[i]);
  if (write) assert(nextWrites==8192 && resultWrites==64 && unit.drained);
  else assert(!nextWrites && !resultWrites && !unit.ops);
  for (unsigned i=0;i<4096;i++) {
   if (write) next[half*4096+i]=memory.at(local+i*4);
   else memory[local+i*4]=prior[half*4096+i];
  }
  unsigned offset=write?4:0; counts[offset]++; counts[offset+1]+=256; counts[2]++; counts[6]=counts[2]; counts[offset+3]++;
  if (packets==badCounter) counts[offset+3]++;
  if (packets==foreignCounter) counts[write?0:4]++;
  if (packets==sourceFault) seals[11]^=1;
  if (packets==descriptorFault) memory[TT_Q38_RGDN_DESCRIPTOR+40*4]^=1;
 }
 void Store(uint32_t at,uint32_t value) override {
  if (Range(at,0xffb20000,40)) { assert(!pending); if (++commandWrites!=dropCommand) command[(at-0xffb20000)/4]=value; return; }
  if (at==0xffb20040) { assert(value==1 && !pending); Submit(); return; }
  if (Range(at,TT_Q38_RGDN_SUMMARY,128)) { memory[at]=value; return; }
  DeltaVectorImage::Store(at,value);
 }
 bool RunResident() {
  while (pc!=uint32_t(L::Park) && pc!=uint32_t(L::FaultPark)) StepScalar();
  assert(minStack>=0x6e000);
  if (pc==uint32_t(L::FaultPark)) {
   assert(memory.at(0x17ffc8)==TT_Q38_RGDN_FAULT); unsigned oldPackets=packets,oldReads=nocReads,ops=unit.ops;
   for (unsigned i=0;i<16;i++) StepScalar(); assert(oldPackets==packets && oldReads==nocReads && ops==unit.ops); return false;
  }
  assert(memory.at(0x17ffc8)==TT_Q38_RGDN_MARKER); return memory.at(0x17ffc4)==64;
 }
 void VerifyResident() {
  assert(packets==4 && !pending); DeltaVectorImage::Verify();
  for (unsigned i=0;i<8192;i++) assert(memory.at(0x90000+i*4)==prior[i] && next[i]==memory.at(0xa0000+i*4));
  uint32_t summary[32],before[8],after[8]; for (unsigned i=0;i<32;i++) summary[i]=memory.at(TT_Q38_RGDN_SUMMARY+i*4);
  for (unsigned i=0;i<8;i++) { before[i]=summary[8+i]; after[i]=summary[16+i]; }
  assert(PioneerQ38ResidentGdnSummary(summary,descriptor.data(),before,after));
  for (unsigned i=0;i<32;i++) { summary[i]^=1; assert(!PioneerQ38ResidentGdnSummary(summary,descriptor.data(),before,after)); summary[i]^=1; }
 }
};
int main(int argc,char** argv) {
 assert(argc==2); setvbuf(stdout,nullptr,_IOLBF,0);
 for (unsigned at:{0x72038U,0x7203cU}) {
  ResidentGdnImage m(argv[1]); m.pc=at; auto before=m.memory; m.StepScalar(); assert(m.pc==0x72038 && m.memory==before);
 }
 for (unsigned bank=0;bank<7;bank++) for (bool edge:{false,true}) for (unsigned trial=0;trial<4;trial++) {
  ResidentGdnImage m(argv[1],trial,bank,(bank+3)%7,(bank*13+trial)%96,edge); assert(m.RunResident()); m.VerifyResident();
 }
 { ResidentGdnImage m(argv[1],1,3,3,95); assert(m.RunResident()); m.VerifyResident(); }
 for (unsigned readBank=0;readBank<7;readBank++) for (unsigned writeBank=0;writeBank<7;writeBank++) {
  ResidentGdnImage m(argv[1],4,readBank,writeBank,(readBank*7+writeBank)%96); assert(m.RunResident()); m.VerifyResident();
 }
 for (unsigned worker=0;worker<96;worker++) {
  ResidentGdnImage m(argv[1],1,0,6,worker); unsigned physical=worker+1;
  const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
  assert(m.descriptor[15]==(columns[physical/10]|((physical%10+2)<<6)) && m.descriptor[15]!=129);
 }
 for (unsigned head=0;head<4;head++) {
  std::array<uint32_t,8192> carry={};
  for (unsigned token=0;token<16;token++) {
   ResidentGdnImage m(argv[1],4+head*16+token,head,(head+1)%7,head*2+(token%2));
   m.prior=carry; assert(m.RunResident()); m.VerifyResident(); carry=m.next;
  }
 }
 puts("PASS ACTUAL OFFLINE resident GDN:ALL7 bank pairs/both3GiB edges/four numeric domains,32KiB read->post-dot vector->DIFFERENT32KiB next slot,ALL8256 words exact,ALL8 wrapped counters+32 summary fields;NOT native/coordinator/model speed");
 puts("PASS ACTUAL OFFLINE resident GDN:ALL49 read/write bank pairs,ALL96 non-root peer route metadata,four half-heads x16 successive NONRESETTING decayed-state updates with varied Q/K/V;NO native/coordinator/GGML-FP64/long-context quality claim");
 for (unsigned packet=1;packet<=4;packet++) for (unsigned fault=0;fault<5;fault++) {
  ResidentGdnImage m(argv[1]);
  if (!fault) m.failPacket=packet; if (fault==1) m.badCounter=packet; if (fault==2) m.foreignCounter=packet;
  if (fault==3) m.sourceFault=packet; if (fault==4) m.descriptorFault=packet;
  assert(!m.RunResident() && m.packets==packet);
 }
 for (unsigned word=1;word<=10;word++) { ResidentGdnImage m(argv[1]); m.dropCommand=word; assert(!m.RunResident() && !m.packets); }
 for (unsigned source=0;source<16;source++) { ResidentGdnImage m(argv[1]); m.seals[source]^=1; assert(!m.RunResident() && !m.packets && !m.unit.ops); }
 for (unsigned word:{0U,1U,2U,3U,12U,13U,14U,15U,42U,63U}) {
  ResidentGdnImage m(argv[1]); m.memory[TT_Q38_RGDN_DESCRIPTOR+word*4]^=1;
  assert(!m.RunResident() && !m.packets && !m.nocReads && !m.unit.ops);
 }
 for (unsigned fault=0;fault<15;fault++) {
  ResidentGdnImage m(argv[1]); auto* d=m.descriptor.data();
  if (!fault) d[4]=7; if (fault==1) d[6]=7; if (fault==2) d[5]=0x04000001; if (fault==3) d[7]=0xc4000000-32764;
  if (fault==4) d[8]=0; if (fault==5) d[8]=0x7fffffff; if (fault==6) d[9]=96; if (fault==7) d[10]=3;
  if (fault==8) d[11]=2; if (fault==9) { d[6]=d[4]; d[7]=d[5]+4096; d[13]=d[12]; }
  if (fault==10) d[16]=0; if (fault>=11) d[32+(fault-11)*2]=d[33+(fault-11)*2]=0;
  for (unsigned i=0;i<64;i++) m.memory[TT_Q38_RGDN_DESCRIPTOR+i*4]=d[i];
  assert(!m.RunResident() && !m.packets && !m.nocReads && !m.unit.ops);
 }
 for (unsigned at:{uint32_t(L::Query),uint32_t(L::Key),uint32_t(L::Value),uint32_t(L::Decay),uint32_t(L::Beta)}) {
  ResidentGdnImage m(argv[1]); m.memory[at]=0x7fc00000; assert(!m.RunResident() && !m.packets && !m.nocReads && !m.unit.ops);
 }
 { ResidentGdnImage m(argv[1]); m.prior[8191]=0x7fc00000; assert(!m.RunResident() && m.packets==2 && !m.unit.ops); }
 puts("PASS ACTUAL OFFLINE resident GDN fault controls:all4 packet timeouts/counter/source/descriptor faults,10 command readback failures,16 source seals,shape/overlap/epoch/route/input/state refusals;inert fault park with ZERO later packets/NoC MMIO/math;NO host/PCI/native selection");
}
