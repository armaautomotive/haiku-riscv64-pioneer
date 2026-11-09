/* SPDX-License-Identifier: MIT */
// Actual linked RV32 value writer, synthetic delayed NoC and intercepted seals.
#define main pioneer_unused_dense_stage_test_main
#include "pioneer_q38_qgw8_dense_stage_image_test.cpp"
#undef main

struct DenseValueMachine: DenseTraceMachine {
 using DenseTraceMachine::DenseTraceMachine;
 unsigned failTransfer=0,corruptTransfer=0;
 uint32_t Load(uint32_t at) override {
  assert(!(at&3));if(CodeAddress(at))return l1[Index(at)];
  if(at==0xffb20040 && pending && transfers.size()!=failTransfer && polls && !--polls){
   for(unsigned i=0;i<command[8];i+=4){assert(memory.count(command[0]+i));memory[command[3]+i]=memory.at(command[0]+i);}
   if(command[7]==0){
    memory[0xffb20208]++;memory[0xffb2020c]+=command[8]/64;memory[0xffb20210]++;memory[0xffb20214]++;
   }else{
    memory[0xffb20210]++;memory[0xffb20204]++;memory[0xffb20228]++;memory[0xffb20220]+=command[8]/64;
   }
   memory[at]=0;pending=false;
   if(transfers.size()==corruptTransfer)memory[0xffb20000]^=64;
  }
  assert(memory.count(at));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value) override {
  assert(!(at&3));
  if(at==0xffb20040){
   assert(value==1 && !pending);for(unsigned i=0;i<10;i++)command[i]=memory.at(0xffb20000+i*4);
   if(command[7]==0){assert(command[2]==0x1234 && command[3]==0x78000 && command[8]==4096 && command[5]==129);}
   else{assert(command[0]==0x78000 && command[2]==129 && command[5]==0x1234 && command[7]==0x212 && command[8]==6144);}
   transfers.push_back({command[0],command[3],command[8]});pending=true;polls=4;memory[at]=1;return;
  }
  assert(Range(at,0x6e000,8192) || Range(at,0x78000,16384) || Range(at,0x80000,4096) || Range(at,0xffb20000,40));memory[at]=value;
 }
 bool Capture(unsigned workspace,unsigned layer,unsigned position,unsigned phase){
  const char* writer="_ZN5BlockL15TraceDenseValueEPN30PioneerQ38MixedBlockRootLayout11PrivateSealE";
  const char* seal="q38_layer_return_dense_seal";const char* quiet="_ZN18DensePartitionRootL5QuietEv";
  assert(symbols.count(writer) && symbols.count(seal) && symbols.count(quiet));
  memory[Owner+268]=layer;memory[Owner+272]=position;memory[Owner+288]=0x1234;memory[Owner+292]=workspace;memory[Owner+1024]=phase;
  for(unsigned offset:{0x204U,0x208U,0x20cU,0x210U,0x214U,0x220U,0x228U})memory[0xffb20000+offset]=0;
  for(unsigned i=0;i<4096;i+=4)memory[workspace+0x18d000+i]=0x51a00000U+i;
  for(unsigned i=0;i<2048;i+=4)memory[0x146800+i]=0x52b00000U+i;
  pc=symbols.at(writer);regs[1]=Return;regs[2]=0x70000;regs[10]=Owner;
  while(pc!=Return && pc!=Fault){
   if(pc==symbols.at(seal))pc=regs[1];
   else if(pc==symbols.at(quiet)){regs[10]=1;pc=regs[1];}
   else Step();
  }
  return pc==Return;
 }
};
int main(int argc,char** argv){
 assert(argc==2);
 for(unsigned base:{0x04000000U,0xc0000000U}){
  DenseValueMachine m(argv[1]);assert(m.Capture(base,3,0,5));const unsigned dest=base+0x3500000;
  const std::vector<std::array<uint32_t,3>> expected={{base+0x18d000,0x78000,4096},{0x78000,dest,6144}};
  assert(m.transfers==expected && !m.pending && m.regs[2]==0x70000);
  for(unsigned i=0;i<4096;i+=4)assert(m.memory.at(dest+i)==0x51a00000U+i);
  for(unsigned i=0;i<2048;i+=4)assert(m.memory.at(dest+4096+i)==0x52b00000U+i);
 }
 for(unsigned layer:{0U,2U,4U,63U}){DenseValueMachine m(argv[1]);assert(m.Capture(0x04000000,layer,0,5) && m.transfers.empty());}
 {DenseValueMachine m(argv[1]);assert(m.Capture(0x04000000,3,1,5) && m.transfers.empty());}
 for(unsigned phase:{0U,4U,6U,7U}){DenseValueMachine m(argv[1]);assert(!m.Capture(0x04000000,3,0,phase) && m.transfers.empty());}
 for(unsigned base:{0x03fff000U,0x04000001U,0xc0001000U}){DenseValueMachine m(argv[1]);assert(!m.Capture(base,3,0,5) && m.transfers.empty());}
 for(unsigned which:{1U,2U}){
  DenseValueMachine corrupt(argv[1]);corrupt.corruptTransfer=which;assert(!corrupt.Capture(0x04000000,3,0,5) && corrupt.transfers.size()==which);
  DenseValueMachine timeout(argv[1]);timeout.failTransfer=which;assert(!timeout.Capture(0x04000000,3,0,5) && timeout.transfers.size()==which && timeout.pending);
 }
 puts("PASS linked value writer, raw+packed data, bounds and delayed NoC; SEAL/QUIET INTERCEPTED, NO DEVICE");
}
