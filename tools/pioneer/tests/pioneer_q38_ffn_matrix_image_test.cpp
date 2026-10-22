/* SPDX-License-Identifier: MIT */
// ACTUAL new ROOT instructions;synthetic peer proof/numeric fabric ONLY.
// Real learned arithmetic/native lifetime/model/timing NOT qualified here.
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_TEST_PRIVATE_FFN_ROOT
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#ifndef PIONEER_Q38_FFN_MATRIX_DISPATCH_INCLUDED
#include "pioneer_qwen38_dispatch_image_test.cpp"
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnMatrixShape.h"
struct FfnMatrixMachine:DispatchMachine {
 std::vector<uint8_t> prefix;
 std::array<uint32_t,64> frame={};std::array<uint32_t,119> first={};
 std::vector<uint32_t> remoteInputs,remoteScales,remoteOutput;
 unsigned role,workspaceBank,order,residentReads=0,residentWrites=0,nocReads=0;
 unsigned failResidentRead=0,failResidentWrite=0,sourceFaultRead=0,sourceFaultWrite=0,mapFaultWrite=0;
 unsigned frameFaultRead=0,foreignCounterRead=0,headerFaultGather=0,rowFaultGather=0;
 unsigned dramCommandReads=0,badDramCommandRead=0;
 bool sourceChanged=false;unsigned sourceWord=0;
 FfnMatrixMachine(const char* path,unsigned mode=0,unsigned bank=0,bool edge=false,unsigned ordering=0,unsigned type=12,unsigned workers=119)
  :DispatchMachine(path,workers,type,mode==2?17408:5120),prefix(Read((std::string(path)+"/prefix.bin").c_str())),
   role(mode),workspaceBank(bank),order(ordering) {
  assert(!prefix.empty() && prefix.size()<=0x4000 && !(prefix.size()&3));
  for(unsigned i=0;i<prefix.size()/4;i++)l1[Index(0x64000+i*4)]=Word(prefix,i*4);
  ConfigureFfn(1,edge);
 }
 unsigned Index(uint32_t a) override {
  if(Range(a,0x64000,0x4000)){assert(!(a&3));return 4096+(a-0x64000)/4;}
  return DispatchMachine::Index(a);
 }
 bool CodeAddress(uint32_t a)const override{return Range(a,0x64000,unsigned(prefix.size())) || DispatchMachine::CodeAddress(a);}
 void ConfigureFfn(unsigned nonce,bool edge=false) {
  Configure(nonce);memory[0x74000]=TT_Q38_FFN_MATRIX_MARKER;memory[0x7401c]=3;memory[0x74020]=1;
  unsigned rows=role==2?5120:17408,groups=rows/16;std::vector<std::pair<unsigned,unsigned>> spans;
  unsigned start=0;for(unsigned w=0;w<active;w++){unsigned n=groups/active+unsigned(w<groups%active);spans.push_back({start,n});start+=n*16;}
  for(unsigned w=0;w<119;w++) {
   if(w>=active){first[w]=0;continue;}
   unsigned index=order==0?w:order==1?active-1-w:(w+active/3)%active;
   first[w]=spans[index].first;memory[0x74080+w*32]=0x10000040+(w/7)*0x00800000;
   memory[0x74088+w*32]=spans[index].second;
  }
  auto& d=frame;d={};d[0]=TT_Q38_FFN_MATRIX_MARKER;d[1]=1;d[2]=67108864;d[3]=workspaceBank;d[4]=edge?0xc0000000:0x04000000;
  d[5]=role;d[6]=nonce;d[7]=active;d[8]=memory.at(0x74040+workspaceBank*4);d[9]=129;
  d[10]=role==0?30848:role==1?100480:274880;d[11]=rows;d[12]=rows*4;d[13]=0x7fa00;d[14]=k;d[15]=kind;
  for(unsigned i=0;i<16;i++)d[16+i]=i?0x12340000+i:0x16005;
  for(unsigned i=32;i<40;i++)d[i]=0x13570000+i;d[40]=123;d[41]=7;d[42]=workspaceBank*9+unsigned(edge);
  SetFrame();SetRows();remoteInputs=originalInputs;remoteScales.resize(((k/256+15)&~15U));
  for(unsigned i=0;i<remoteScales.size();i++)remoteScales[i]=memory.at(0xab000+i*4);
  for(unsigned i=0;i<k/2;i++)memory[0xa0000+i*4]=0xdeadbeef;
  for(unsigned i=0;i<remoteScales.size();i++)memory[0xab000+i*4]=0xdeadbeef;
  remoteOutput.assign(rows,0xdeadbeef);
  memory[0x7fbfc]=0xdecafbad; // after ALL119row words,not cycle data or row0
 }
 void SetFrame(){for(unsigned i=0;i<64;i++)memory[0x7f900+i*4]=frame[i];}
 void SetRows(){for(unsigned i=0;i<119;i++)memory[0x7fa00+i*4]=first[i];}
 uint32_t ReadCycleCsr(unsigned csr)override {assert(csr==0xb00 || csr==0xb80);uint64_t now=(UINT64_C(1)<<40)+uint64_t(steps)*19;return csr==0xb00?uint32_t(now):uint32_t(now>>32);}
 uint32_t Load(uint32_t a) override {
  if(Range(a,0x64000,unsigned(prefix.size())))return l1[Index(a)];
  if(Range(a,0x7f900,256) || Range(a,0x7fa00,119*4))return memory.at(a);
  if(Range(a,0xffb20000,0x2000)) {
   nocReads++;
   if(Range(a,0xffb20000,40) && (command[7]==0x212?command[5]:command[2])==frame[8]) {
    unsigned word=(a-0xffb20000)/4;uint32_t value=command[word];
    if(++dramCommandReads==badDramCommandRead)value^=1;seen[word]=true;return value;
   }
   if(sourceChanged && a==0xffb20000+BlackholeDramSourceOffset(sourceWord))return (sourceWord?0x12340000+sourceWord:0x16005)^1;
  }
  return DispatchMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override {
  // Root instructions may NOT stage/expand/requantize the native payload.
  // Only simulated NoC DATA response endpoints update those dictionaries.
  assert(!Range(a,0xa0000,k*2) && !Range(a,0xab000,2176)
   && !Range(a,0x7f900,256) && !Range(a,0x7fa00,119*4));
  if(a==0x17ffc8 && value==TT_Q38_FFN_MATRIX_FAULT){unknown=true;l1[Index(a)]=value;return;}
  DispatchMachine::Store(a,value);
 }
 void Packet()override {
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];
  if(route!=frame[8]) {
   assert(route!=1555); // NO host PCI endpoint,including accidental legacy DMA
   DispatchMachine::Packet();
   if(gathers && gathers==headerFaultGather)memory[0x74018]^=1;
   if(gathers && gathers==rowFaultGather)memory[0x7fa00]^=16;
   return;
  }
  assert(pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9]);
  assert((write?command[2]:command[5])==129 && command[7]==(write?0x212U:0U));
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(bytes && bytes<=16384 && !(bytes&63));
  if(!write) {
   assert(!descriptors && !doorbells && !gathers);unsigned index=residentReads++,operandBytes=k*2;
   unsigned operandPackets=(operandBytes+16383)/16384;
   assert(index<=operandPackets);
   if(residentReads==failResidentRead){faultAt=packets;return;}
   if(index<operandPackets) {
    unsigned at=index*16384,n=std::min(16384U,operandBytes-at);
    assert(local==0xa0000+at && remote==frame[4]+(role==2?239744:20480)+at && bytes==n);
    for(unsigned i=0;i<n/4;i++)memory[local+i*4]=remoteInputs[at/4+i];
   }else {
    assert(local==0xab000 && remote==frame[4]+(role==2?274560:30720) && bytes==remoteScales.size()*4);
    for(unsigned i=0;i<remoteScales.size();i++)memory[local+i*4]=remoteScales[i];
   }
   if(residentReads==sourceFaultRead)sourceChanged=true;
   if(residentReads==frameFaultRead)memory[0x7f900+32*4]^=1;
   if(residentReads==foreignCounterRead)counts[4]++;
  }else {
   assert(AllFinished() && gathers==active);residentWrites++;
   if(residentWrites==failResidentWrite){faultAt=packets;return;}
   unsigned firstByte=remote-frame[4]-frame[10];assert(remote>=frame[4]+frame[10] && firstByte<frame[12] && bytes<=frame[12]-firstByte);
   unsigned packed=0;
   if(!order)assert(local==0x80000+firstByte && !(firstByte&16383));
   else {
    unsigned w=residentWrites-1;assert(w<active && firstByte==first[w]*4 && bytes==memory.at(0x74088+w*32)*64);
    for(unsigned previous=0;previous<w;previous++)packed+=memory.at(0x74088+previous*32)*16;
    assert(local==0x80000+packed*4);
   }
   for(unsigned i=0;i<bytes/4;i++)remoteOutput[firstByte/4+i]=memory.at(local+i*4);
   if(residentWrites==sourceFaultWrite)sourceChanged=true;
   if(residentWrites==mapFaultWrite)memory[0x7fa00]^=16;
  }
  counts[write?4:0]++;counts[write?5:1]+=bytes/64;counts[write?7:3]++;counts[2]++;responses++;pending=false;
 }
 bool RunFfn() {
  while(pc!=TT_Q38_DISPATCH_PARK && pc!=TT_Q38_DISPATCH_FAULT_PARK)StepBlock();
  assert(minStack>=0x6e000 && memory.at(0x7fbfc)==0xdecafbad);
  if(pc==TT_Q38_DISPATCH_FAULT_PARK){assert(unknown);return false;}
  assert(l1[Index(0x17ffc8)]==TT_Q38_FFN_MATRIX_MARKER);
  if(l1[Index(0x17ffc4)]!=active)return false;
  assert(!pending && descriptors==active && doorbells==active && gathers==(operation==1?active:0));
  return true;
 }
 void VerifyFfn() {
  assert(residentReads==(role==2?4:2) && residentWrites==(order?active:(role==2?2:5)));
  assert(memory.at(0x7f8c4)==TT_Q38_FFN_MATRIX_MARKER && memory.at(0x7f8e0)==1);
  for(unsigned w=0;w<active;w++)for(unsigned row=0;row<memory.at(0x74088+w*32)*16;row++)assert(remoteOutput[first[w]+row]==ExpectedOutput(w,row));
  for(unsigned i=0;i<k/2;i++)assert(memory.at(0xa0000+i*4)==remoteInputs[i]);
  for(unsigned i=0;i<remoteScales.size();i++)assert(memory.at(0xab000+i*4)==remoteScales[i]);
  for(unsigned i=0;i<64;i++)assert(memory.at(0x7f900+i*4)==frame[i]);
  for(unsigned i=0;i<119;i++)assert(memory.at(0x7fa00+i*4)==first[i]);
 }
 void Inert(){unsigned oldReads=nocReads,oldPackets=packets;for(unsigned i=0;i<32;i++)StepBlock();assert(nocReads==oldReads && packets==oldPackets);}
};
#ifndef PIONEER_Q38_FFN_MATRIX_MACHINE_ONLY
int main(int argc,char** argv) {
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned good=0,bad=0,minStack=0x70000,maxSteps=0;
 for(unsigned bank=0;bank<7;bank++)for(bool edge:{false,true})for(unsigned role=0;role<3;role++)for(unsigned order=0;order<3;order++)for(unsigned type:{12U,14U}) {
  FfnMatrixMachine m(argv[1],role,bank,edge,order,type,order==0?(role==2?20:68):119);
  assert(m.RunFfn());m.VerifyFfn();good++;minStack=std::min(minStack,m.minStack);maxSteps=std::max(maxSteps,m.steps);
  if(type==14 && order==2)printf("OFFLINE ACTUAL resident matrix ROOT fullgeometry bank%u edge%u role%u/realroot-syntheticpeers PASS\n",bank,unsigned(edge),role);
 }
 for(unsigned mode:{0U,1U,2U}) {FfnMatrixMachine m(argv[1]);m.memory[0x7401c]=mode;assert(!m.RunFfn() && !m.packets && !m.descriptors);bad++;}
 {FfnMatrixMachine m(argv[1]);m.memory[0x74000]=0x314d3851;assert(!m.RunFfn() && !m.packets);bad++;}
 for(unsigned w=0;w<119;w++){FfnMatrixMachine m(argv[1]);m.first[w]^=1;m.SetRows();assert(!m.RunFfn() && !m.packets);bad++;}
 for(unsigned field:{0U,1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,14U,15U,42U}) {
  FfnMatrixMachine m(argv[1]);if(field==42)m.frame[field]=64;else m.frame[field]^=field==3?7:1;m.SetFrame();
  // role0->1 requires its own fixed output offset,so still malformed.
  assert(!m.RunFfn() && !m.packets);bad++;
 }
 for(unsigned field=43;field<64;field++){FfnMatrixMachine m(argv[1]);m.frame[field]=1;m.SetFrame();assert(!m.RunFfn() && !m.packets);bad++;}
 for(unsigned field=0;field<4;field++){FfnMatrixMachine m(argv[1]);m.frame[32+field*2]=m.frame[33+field*2]=0;m.SetFrame();assert(!m.RunFfn() && !m.packets);bad++;}
 for(unsigned field=0;field<64;field++){FfnMatrixMachine m(argv[1],2);m.corruptWorker=119;m.corruptProof=field;
  assert(!m.RunFfn() && m.unknown && !m.residentWrites);m.Inert();bad++;}
 for(unsigned worker:{1U,60U,119U}){FfnMatrixMachine m(argv[1]);m.badPcWorker=worker;
  assert(!m.RunFfn() && m.unknown && !m.residentWrites);m.Inert();bad++;}
 for(unsigned role:{0U,2U})for(unsigned input=1;input<=(role==2?4U:2U);input++)for(unsigned fault=0;fault<4;fault++) {
  FfnMatrixMachine m(argv[1],role);if(!fault)m.failResidentRead=input;if(fault==1)m.sourceFaultRead=input;
  if(fault==2)m.frameFaultRead=input;if(fault==3)m.foreignCounterRead=input;
  assert(!m.RunFfn() && m.unknown && m.residentReads==input && !m.descriptors && !m.residentWrites);m.Inert();bad++;
 }
 for(unsigned role:{0U,2U})for(unsigned output=1;output<=(role==2?2U:5U);output++)for(unsigned fault=0;fault<3;fault++) {
  FfnMatrixMachine m(argv[1],role);if(!fault)m.failResidentWrite=output;if(fault==1)m.sourceFaultWrite=output;if(fault==2)m.mapFaultWrite=output;
  assert(!m.RunFfn() && m.unknown && m.residentWrites==output);m.Inert();bad++;
 }
 for(unsigned fault=0;fault<2;fault++){FfnMatrixMachine m(argv[1]);if(!fault)m.headerFaultGather=119;else m.rowFaultGather=119;
  assert(!m.RunFfn() && m.unknown && !m.residentWrites);m.Inert();bad++;}
 for(unsigned source=0;source<16;source++){FfnMatrixMachine m(argv[1]);m.sourceWord=source;m.sourceChanged=true;
  assert(!m.RunFfn() && m.unknown && !m.packets);m.Inert();bad++;}
 for(unsigned fault=0;fault<3;fault++){FfnMatrixMachine m(argv[1],2);
  if(!fault)m.remoteInputs.back()=0xffff4001;if(fault==1)m.remoteScales[0]=0x7fc00001;if(fault==2)m.remoteScales.back()=1;
  assert(!m.RunFfn() && m.unknown && m.residentReads==4 && !m.descriptors && !m.residentWrites);m.Inert();bad++;}
 for(unsigned role:{0U,2U})for(unsigned packet=0;packet<(role==2?6U:7U);packet++)for(unsigned word=0;word<10;word++) {
  FfnMatrixMachine m(argv[1],role);m.badDramCommandRead=packet*10+word+1;
  assert(!m.RunFfn() && m.unknown && m.residentReads+m.residentWrites==packet);m.Inert();bad++;
 }
 for(unsigned snapshot=0;snapshot<3;snapshot++) {
  FfnMatrixMachine m(argv[1]);m.active=119;m.Configure(1,true);m.memory[0x74000]=TT_Q38_FFN_MATRIX_MARKER;
  m.stopSlowWorker=1;m.stopSnapshotMode=snapshot;assert(m.RunFfn() && !m.residentReads && !m.residentWrites);
  for(const auto& p:m.peers)assert(p.finished==p.requested && p.operation==2);
  for(unsigned i=49;i<64;i++)assert(!m.memory.at(0x7f800+i*4));good++;
 }
 printf("PASS OFFLINE ACTUAL QFM1 resident matrix root:fullprojectioncases%u/refusals%u/maxsteps%u/minstack%08x;GDDRnativeinput->ALLpeerPC/reset/source/counter/commandproofs->GDDRoutput/ordered5or2writes/unorderedscatter/zerohostPCIorCPUproducts/newABIrejectslegacy0-2;SYNTHETIC_PEERS_NOT_NATIVE_ARITHMETIC_OR_SPEED\n",good,bad,maxSteps,minStack);
}
#endif
