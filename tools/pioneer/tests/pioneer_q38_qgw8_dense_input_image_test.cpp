/* SPDX-License-Identifier: MIT */
// Actual linked RV32 writer under synthetic NoC; seal/quiet are intercepted.
#define main pioneer_unused_dense_stage_test_main
#include "pioneer_q38_qgw8_dense_stage_image_test.cpp"
#undef main

struct DenseInputMachine: DenseTraceMachine {
 using DenseTraceMachine::DenseTraceMachine;
 void Store(uint32_t at,uint32_t value) override {
  if(at==0xffb20040){
   assert(value==1 && !pending);for(unsigned i=0;i<10;i++)command[i]=memory.at(0xffb20000+i*4);
   assert(command[0]==0x78000 && command[2]==129 && command[7]==0x212 && (command[8]==16384 || command[8]==8192));
   transfers.push_back({command[3],command[8],command[5]});pending=true;polls=4;memory[at]=1;return;
  }
  assert(Range(at,0x6e000,8192) || Range(at,0x78000,16384) || Range(at,0xffb20000,40));memory[at]=value;
 }
 bool Capture(unsigned workspace,unsigned layer,unsigned position,unsigned phase){
  const char* writer="_ZN5BlockL15TraceDenseInputEPN30PioneerQ38MixedBlockRootLayout11PrivateSealE";
  const char* seal="q38_layer_return_dense_seal";const char* quiet="_ZN18DensePartitionRootL5QuietEv";
  assert(symbols.count(writer) && symbols.count(seal) && symbols.count(quiet));
  memory[Owner+268]=layer;memory[Owner+272]=position;memory[Owner+288]=0x1234;memory[Owner+292]=workspace;memory[Owner+1024]=phase;
  for(unsigned offset:{0x204U,0x208U,0x20cU,0x210U,0x214U,0x220U,0x228U})memory[0xffb20000+offset]=0;
  for(unsigned i=0;i<24576;i+=4)memory[0x12a000+i]=0x71a00000U+i;
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
  DenseInputMachine m(argv[1]);assert(m.Capture(base,3,0,5));
  const unsigned dest=base+0x3500000;
  const std::vector<std::array<uint32_t,3>> expected={{dest,16384,0x1234},{dest+16384,8192,0x1234}};
  assert(m.transfers==expected && !m.pending && m.regs[2]==0x70000);
  for(unsigned i=0;i<24576;i+=4)assert(m.memory.at(dest+i)==0x71a00000U+i);
 }
 for(unsigned layer:{0U,2U,4U,63U}){DenseInputMachine m(argv[1]);assert(m.Capture(0x04000000,layer,0,5) && m.transfers.empty());}
 {DenseInputMachine m(argv[1]);assert(m.Capture(0x04000000,3,1,5) && m.transfers.empty());}
 for(unsigned phase:{0U,4U,6U,7U}){DenseInputMachine m(argv[1]);assert(!m.Capture(0x04000000,3,0,phase) && m.transfers.empty());}
 for(unsigned base:{0x03fff000U,0x04000001U,0xc0001000U}){DenseInputMachine m(argv[1]);assert(!m.Capture(base,3,0,5) && m.transfers.empty());}
 {DenseInputMachine m(argv[1]);m.corruptAfterFirst=true;assert(!m.Capture(0x04000000,3,0,5) && m.transfers.size()==1);}
 {DenseInputMachine m(argv[1]);m.neverComplete=true;assert(!m.Capture(0x04000000,3,0,5) && m.transfers.size()==1 && m.pending);}
 puts("PASS linked dense input writer, bounds, delayed synthetic NoC, corruption and timeout; SEAL/QUIET INTERCEPTED, NO DEVICE");
}
