/* SPDX-License-Identifier: MIT */
// ACTUAL compiled RV32 instructions; only NoC/device memory is simulated.
#define PIONEER_Q38_BLOCK_MACHINE_ONLY
#include "pioneer_qwen38_block_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StateZeroShape.h"
#include <array>
struct StateZeroMachine:BlockMachine {
 std::vector<uint32_t> gddr;
 std::array<uint32_t,64> descriptor={};
 uint32_t command[10]={},counts[8]={},seals[16]={};
 unsigned bytes,bank,base,transfers=0,controlWrites=0,mmioReads=0,zeroStores=0,poisonStores=0;
 unsigned failPacket=0,badCounter=0,unexpectedCounter=0,changeSourceAfter=0,dropControl=0,dropZero=0,dropPoison=0,changeDescriptorAfter=0;
 int dropData=-1,corruptData=-1;
 bool pending=false;
 static unsigned SourceOffset(unsigned i) {
  const unsigned offsets[]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c}; return offsets[i];
 }
 StateZeroMachine(const char* image,unsigned length,unsigned b=0,bool edge=false)
  :BlockMachine(image,8,0,.01f,nullptr,true),gddr(length/4),bytes(length),bank(b),base(edge?0xc4000000-length:0x04000000) {
  for (unsigned i=0;i<gddr.size();i++) gddr[i]=0xa5a50000U+i; // deliberately NOT zero
  for (unsigned a:{0x84000U,0x144000U}) for (unsigned i=0;i<4096;i++) memory[a+i*4]=0xdeadbeef;
  for (unsigned i=0;i<32;i++) memory[0x143000+i*4]=0xdeadbeef;
  for (auto& c:command) c=0xdeadbeef;
  for (auto& c:counts) c=0xfffffff8;
  for (unsigned i=0;i<16;i++) seals[i]=0xa5000000+i*0x13579; seals[0]=0x16005;
  descriptor[0]=TT_Q38_ZERO_MARKER; descriptor[1]=descriptor[2]=1; descriptor[4]=bytes; descriptor[5]=base;
  descriptor[6]=17; descriptor[7]=bank; descriptor[8]=PioneerQ38SpanRoute(bank);
  for (unsigned i=0;i<16;i++) descriptor[16+i]=seals[i];
  for (unsigned i=0;i<64;i++) memory[0x140000+i*4]=descriptor[i];
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
 }
 unsigned StepLimit() const override { return 100000000; }
 uint32_t Load(uint32_t a) override {
  if (Range(a,0xffb20000,0x2000)) {
   mmioReads++; unsigned offset=a-0xffb20000;
   if (offset<40) return command[offset/4];
   for (unsigned i=0;i<4;i++) if (offset==0x40+i*0x800) return i?0:unsigned(pending);
   if (offset>=0x240 && offset<0x2c0) return 0;
   for (unsigned i=0;i<16;i++) if (offset==SourceOffset(i)) return seals[i];
   const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for (unsigned i=0;i<8;i++) if (offset==offsets[i]) return counts[i];
   assert(false && "unqualified state-zero MMIO read");
  }
  if (CodeAddress(a)) return BlockMachine::Load(a);
  assert(Range(a,0x6e000,8192) || Range(a,0x140000,256) || Range(a,0x143000,128)
   || Range(a,0x84000,16384) || Range(a,0x144000,16384) || Range(a,0x17ffc0,12));
  return memory.at(a);
 }
 void Submit() {
  unsigned packet=transfers%2; transfers++;
  if (transfers==failPacket) { pending=true; return; }
  bool write=command[7]==0x212; assert(write || !command[7]);
  assert(write==!packet && !(command[8]&4095) && command[8] && command[8]<=16384);
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],words=command[8]/4;
  assert(local==(write?0x84000U:0x144000U) && (write?command[2]:command[5])==129
   && (write?command[5]:command[2])==PioneerQ38SpanRoute(bank) && !command[1] && !command[4] && !command[6] && !command[9]);
  assert(remote>=base && !((remote-base)&4095)); unsigned at=(remote-base)/4;
  assert(at<gddr.size() && words<=gddr.size()-at);
  if (int(packet)!=dropData) for (unsigned i=0;i<words;i++) {
   uint32_t corruption=int(packet)==corruptData && i==0?1:0;
   if (write) gddr[at+i]=memory.at(local+i*4)^corruption;
   else memory[local+i*4]=gddr[at+i]^corruption;
  }
  unsigned offset=write?4:0; counts[offset]++; counts[offset+1]+=words/16; counts[2]++; counts[6]=counts[2]; counts[offset+3]++;
  if (transfers==badCounter) counts[offset+3]++;
  if (transfers==unexpectedCounter) counts[write?0:4]++; // unrelated counter ALSO must remain unchanged
  if (transfers==changeSourceAfter) seals[11]^=1;
  if (transfers==changeDescriptorAfter) memory[0x140000+6*4]^=1;
 }
 void Store(uint32_t a,uint32_t value) override {
  if (Range(a,0xffb20000,40)) { assert(!pending); if (++controlWrites!=dropControl) command[(a-0xffb20000)/4]=value; return; }
  if (a==0xffb20040) { assert(value==1 && !pending); Submit(); return; }
  assert(Range(a,0x6e000,8192) || Range(a,0x143000,128) || Range(a,0x84000,16384)
   || Range(a,0x144000,16384) || Range(a,0x17ffc0,12));
  if (Range(a,0x6e000,8192)) minStack=std::min(minStack,a);
  if (Range(a,0x84000,16384) && ++zeroStores==dropZero) return;
  if (Range(a,0x144000,16384) && ++poisonStores==dropPoison) return;
  memory[a]=value;
 }
 bool Run() {
  while (pc!=0x72038 && pc!=0x7207c) StepBlock();
  assert(minStack>=0x6e000 && !matrixInstructions && !moveInstructions && !floatOps);
  if (pc==0x7207c) {
   assert(memory.at(0x17ffc8)==TT_Q38_ZERO_FAULT); unsigned packets=transfers,reads=mmioReads;
   for (unsigned i=0;i<16;i++) StepBlock(); assert(packets==transfers && reads==mmioReads); return false;
  }
  assert(memory.at(0x17ffc0)==0x7200c && memory.at(0x17ffc8)==TT_Q38_ZERO_MARKER);
  return memory.at(0x17ffc4)==bytes/4096;
 }
 void Verify() {
  assert(!pending && transfers==2*((bytes+16383)/16384));
  for (auto word:gddr) assert(!word);
  uint32_t summary[32],before[8],after[8];
  for (unsigned i=0;i<32;i++) summary[i]=memory.at(0x143000+i*4);
  for (unsigned i=0;i<8;i++) { before[i]=summary[8+i]; after[i]=summary[16+i]; }
  assert(PioneerQ38StateZeroSummary(summary,descriptor.data(),before,after));
  for (unsigned i=0;i<32;i++) { summary[i]^=1; assert(!PioneerQ38StateZeroSummary(summary,descriptor.data(),before,after)); summary[i]^=1; }
 }
};
int main(int argc,char** argv) {
 assert(argc==2);
 for (unsigned at:{0x72038U,0x7203cU}) {
  StateZeroMachine m(argv[1],4096); m.pc=at; auto memory=m.memory; m.StepBlock(); assert(m.pc==0x72038 && m.memory==memory);
 }
 for (unsigned bank=0;bank<7;bank++) for (bool edge:{false,true}) for (unsigned bytes:{4096U,32768U}) {
  StateZeroMachine m(argv[1],bytes,bank,edge); assert(m.Run()); m.Verify();
 }
 for (unsigned bytes:{12288U,20480U,458752U,1048576U}) {
  StateZeroMachine m(argv[1],bytes,6,true); assert(m.Run()); m.Verify();
 }
 for (unsigned packet=1;packet<=4;packet++) for (unsigned fault=0;fault<4;fault++) {
  StateZeroMachine m(argv[1],32768);
  if (fault==0) m.failPacket=packet; if (fault==1) m.badCounter=packet;
  if (fault==2) m.unexpectedCounter=packet; if (fault==3) m.changeSourceAfter=packet;
  assert(!m.Run() && m.transfers==packet);
 }
 for (unsigned packet=0;packet<2;packet++) for (bool corrupt:{false,true}) {
  StateZeroMachine m(argv[1],32768); if (corrupt) m.corruptData=int(packet); else m.dropData=int(packet); assert(!m.Run());
 }
 for (unsigned word=1;word<=10;word++) { StateZeroMachine m(argv[1],4096); m.dropControl=word; assert(!m.Run() && !m.transfers); }
 for (unsigned word:{1U,2048U,4096U}) { StateZeroMachine m(argv[1],4096); m.dropZero=word; assert(!m.Run() && !m.transfers); }
 for (unsigned word:{1U,2048U,4096U,4097U,6144U,8192U}) {
  StateZeroMachine m(argv[1],32768); m.dropPoison=word; assert(!m.Run() && m.transfers==(word<=4096?0U:2U));
 }
 for (unsigned source=0;source<16;source++) { StateZeroMachine m(argv[1],4096); m.seals[source]^=1; assert(!m.Run() && !m.transfers); }
 for (unsigned word=0;word<64;word++) if ((word<16 && word!=6) || word>=32) {
  StateZeroMachine m(argv[1],4096); m.memory[0x140000+word*4]^=1; assert(!m.Run() && !m.transfers && !m.mmioReads);
 }
 for (unsigned fault=0;fault<8;fault++) {
  StateZeroMachine m(argv[1],4096); const unsigned word[]={4,4,5,5,6,6,7,8},value[]={0,1048576+4096,0x04000001,0xc4000000,0,0x7fffffff,7,1555};
  m.memory[0x140000+word[fault]*4]=value[fault]; assert(!m.Run() && !m.transfers && !m.mmioReads);
 }
 { StateZeroMachine m(argv[1],32768); m.changeDescriptorAfter=1; assert(!m.Run()); }
 puts("PASS ACTUAL OFFLINE RV32 state-zero:ALL7 banks/3GiB edges/4KiB..1MiB,full zero+poisoned readback,ALL8 wrapped counters,32 summary fields,both safe terminals,packet/data/source/descriptor/store faults inert after detection;NO host traffic/PCI/matrix/native driver selection or performance claim");
}
