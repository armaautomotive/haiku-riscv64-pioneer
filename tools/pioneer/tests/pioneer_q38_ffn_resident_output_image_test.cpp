/* SPDX-License-Identifier: MIT */
// ACTUAL emitted RV32 image;NoC/GDDR endpoints ONLY are simulated. The
// matrix producer is NOT executed here:that integration remains unqualified.
#define PIONEER_Q38_FFN_RESIDENT_ACTIVATION_MACHINE_ONLY
#include "pioneer_q38_ffn_resident_activation_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnOutputShape.h"
struct OutputMachine:ResidentActivationMachine {
 uint32_t rowMap[119][4]={};std::vector<uint32_t> remoteOutput;
 unsigned mapFault=0,payloadLoads=0;
 static uint32_t Value(unsigned row) {
  // Copy is bit-exact,including signed zeros,subnormals and finite extremes;
  // no BabyRISCV floating arithmetic/FTZ is permitted in this transport.
  const uint32_t special[]={0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff};
  return row<6?special[row]:((row&1)<<31)|0x3f000000|((row*2654435761U)&0x7fffff);
 }
 OutputMachine(const char* path,unsigned bank=0,bool edge=false,unsigned mode=0,unsigned workers=119,unsigned order=0)
  :ResidentActivationMachine(path,bank,edge) {
  auto& d=descriptor;d[0]=TT_Q38_FFN_OUTPUT_MARKER;d[5]=mode;d[7]=workers;d[9]=129;
  d[10]=mode==0?30848:mode==1?100480:274880;d[11]=mode==2?5120:17408;d[12]=d[11]*4;
  d[13]=0x74400;d[14]=4;d[42]=unsigned((bank*9+mode)%64);
  remoteOutput.assign(d[11],0xdeadbeef);unsigned groups=d[11]/16,first=0,packed=0;
  std::vector<std::pair<unsigned,unsigned>> spans;
  for(unsigned w=0;w<workers;w++){unsigned rows=(groups/workers+unsigned(w<groups%workers))*16;spans.push_back({first,rows});first+=rows;}
  for(unsigned w=0;w<workers;w++) {
   unsigned index=order==0?w:order==1?workers-1-w:(w+workers/3)%workers;
   const auto& span=spans[index];rowMap[w][0]=span.first;rowMap[w][1]=span.second;rowMap[w][2]=packed;rowMap[w][3]=1+w%17;
   for(unsigned row=0;row<span.second;row++)memory[0x80000+(packed+row)*4]=Value(span.first+row);
   packed+=span.second;
  }
  SetDescriptor();SetMap();assert(PioneerQ38FfnOutputMap(d.data(),rowMap));
  for(uint32_t guard:{0x741fcU,0x743fcU,0x74b70U,0x7fffcU,0x91000U})memory[guard]=0xdecafbad;
 }
 void SetMap(){for(unsigned w=0;w<119;w++)for(unsigned i=0;i<4;i++)memory[0x74400+(w*4+i)*4]=rowMap[w][i];}
 uint32_t Load(uint32_t a) override {
  if(Range(a,0x74400,119*16))return memory.at(a);
  if(Range(a,0x80000,descriptor[11]*4)){payloadLoads++;return memory.at(a);}
  return ResidentActivationMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t value) override {
  // Stronger than the parent's arithmetic fixture:no payload/result/math,
  // descriptor/map,code,host/PCI/reset or control-CSR store is permitted.
  assert(Range(a,0xffb20000,40) || a==0xffb20040 || Range(a,TT_Q38_FFN_RESIDENT_SUMMARY,128)
   || Range(a,0x6e000,8192) || Range(a,0x17ffc0,12));
  if(a==0xffb20040){assert(value==1 && !pending);SubmitOutput();return;}
  ResidentActivationMachine::Store(a,value);
 }
 void SubmitOutput() {
  unsigned worker=packets++;assert(worker<descriptor[7]);const auto& p=rowMap[worker];
  unsigned size=p[1]*4,local=0x80000+p[2]*4,remote=descriptor[4]+descriptor[10]+p[0]*4;
  const unsigned routes[]={786,978,1170,785,977,1169,1361};
  const uint32_t expected[]={local,0,129,remote,0,routes[descriptor[3]],0,0x212,size,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  assert(size && size<=1024 && !(size&63) && local>=0x80000 && local+size<=0x80000+descriptor[11]*4);
  if(packets==failPacket){pending=true;return;}
  for(unsigned i=0;i<size/4;i++)remoteOutput[p[0]+i]=memory.at(local+i*4);
  counts[4]++;counts[5]+=size/64;counts[2]++;counts[6]=counts[2];counts[7]++;
  if(packets==badCounter)counts[7]++;if(packets==foreignCounter)counts[0]++;
  if(packets==sourceFault)seals[11]^=1;
  if(packets==descriptorFault)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
  if(packets==mapFault)memory[TT_Q38_FFN_OUTPUT_MAP]^=16;
 }
 bool RunOutput() {
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(uint32_t guard:{0x741fcU,0x743fcU,0x74b70U,0x7fffcU,0x91000U})assert(memory.at(guard)==0xdecafbad);
  assert(!activatedWrites && !operandWrites && !nativeScaleWrites && !outputWrites && !scaleWrites);
  if(pc==0x64058) {
   assert(memory.at(0x17ffc8)==TT_Q38_FFN_OUTPUT_FAULT && !memory.at(0x17ffc4));
   unsigned oldPackets=packets,oldReads=nocReads,oldWrites=commandWrites,oldLoads=payloadLoads;
   for(unsigned i=0;i<16;i++)StepScalar();
   assert(packets==oldPackets && nocReads==oldReads && commandWrites==oldWrites && payloadLoads==oldLoads);return false;
  }
  assert(memory.at(0x17ffc8)==TT_Q38_FFN_OUTPUT_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==descriptor[11];
 }
 void VerifyOutput() {
  assert(packets==descriptor[7] && !pending && payloadLoads==descriptor[11]);
  for(unsigned row=0;row<descriptor[11];row++)assert(remoteOutput[row]==Value(row));
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<4;i++)assert(memory.at(0x74400+(w*4+i)*4)==rowMap[w][i]);
  uint32_t s[32],before[8],after[8];for(unsigned i=0;i<32;i++)s[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  for(unsigned i=0;i<8;i++){before[i]=s[8+i];after[i]=s[16+i];}
  assert(PioneerQ38FfnOutputSummary(s,descriptor.data(),before,after));
  for(unsigned i=0;i<32;i++){s[i]^=1;assert(!PioneerQ38FfnOutputSummary(s,descriptor.data(),before,after));s[i]^=1;}
 }
 void Refused(){auto before=remoteOutput;assert(!RunOutput() && !packets && !nocReads && before==remoteOutput);}
};
#ifndef PIONEER_Q38_FFN_OUTPUT_MACHINE_ONLY
int main(int argc,char** argv) {
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positive=0,negative=0,maxSteps=0,minStack=0x70000;
 // Compare descriptor destinations with the canonical model layout,not
 // only constants shared with the device implementation.
 Fixture fixture;PioneerQwen38Gguf model;assert(model.Open(fixture.file));PioneerQwen38PackedPlan plan;assert(plan.Build(model,8192,true,true));
 for(unsigned bank=0;bank<7;bank++) {
  PioneerQ38FfnScratchLayout layout;assert(PioneerQ38FfnScratchLayout::Describe(model,plan,bank,layout));
  assert(layout.slice[layout.Gate].offset==30848 && layout.slice[layout.Up].offset==100480 && layout.slice[layout.Output].offset==274880);
  for(bool edge:{false,true})for(unsigned mode=0;mode<3;mode++)for(unsigned order=0;order<3;order++) {
   unsigned workers=order==0?(mode==2?20:68):order==1?96:119;
   OutputMachine m(argv[1],bank,edge,mode,workers,order);auto payload=m.memory;
   assert(m.RunOutput());m.VerifyOutput();positive++;
   for(unsigned i=0;i<m.descriptor[11];i++)assert(m.memory.at(0x80000+i*4)==payload.at(0x80000+i*4));
   maxSteps=std::max(maxSteps,m.steps);minStack=std::min(minStack,m.minStack);
  }
 }
 for(unsigned w=0;w<119;w++)for(unsigned field=0;field<4;field++) {
  OutputMachine m(argv[1]);m.rowMap[w][field]^=field==1?16:field==3?0x80000000:1;m.SetMap();m.Refused();negative++;
 }
 for(unsigned field=0;field<4;field++){OutputMachine m(argv[1],0,false,0,68);m.rowMap[118][field]=1;m.SetMap();m.Refused();negative++;}
 for(unsigned field:{0U,1U,2U,5U,8U,9U,10U,11U,12U,13U,14U,15U}) {
  OutputMachine m(argv[1]);m.descriptor[field]^=1;m.SetDescriptor();m.Refused();negative++;
 }
 for(unsigned field=43;field<64;field++){OutputMachine m(argv[1]);m.descriptor[field]=1;m.SetDescriptor();m.Refused();negative++;}
 for(unsigned fault=0;fault<15;fault++) {
  OutputMachine m(argv[1]);auto& d=m.descriptor;
  if(!fault)d[3]=7;if(fault==1)d[4]=0x03fff000;if(fault==2)d[4]=0xc0001000;if(fault==3)d[4]++;
  if(fault==4)d[6]=0;if(fault==5)d[6]=0x7fffffff;if(fault==6)d[7]=0;if(fault==7)d[7]=120;
  if(fault==8)d[16]=0;if(fault==9)d[42]=64;if(fault>=10 && fault<=13)d[32+(fault-10)*2]=d[33+(fault-10)*2]=0;
  if(fault==14)d[5]=3;m.SetDescriptor();m.Refused();negative++;
 }
 for(unsigned row:{0U,8704U,17407U})for(uint32_t bits:{0x7f800000U,0xff800000U,0x7fc00001U}) {
  OutputMachine m(argv[1]);m.memory[0x80000+row*4]=bits;m.Refused();negative++;
 }
 for(unsigned packet:{1U,60U,119U})for(unsigned fault=0;fault<6;fault++) {
  OutputMachine m(argv[1]);if(!fault)m.failPacket=packet;if(fault==1)m.badCounter=packet;if(fault==2)m.foreignCounter=packet;
  if(fault==3)m.sourceFault=packet;if(fault==4)m.descriptorFault=packet;if(fault==5)m.mapFault=packet;
  assert(!m.RunOutput() && m.packets==packet);negative++;
 }
 for(unsigned packet:{1U,60U,119U})for(unsigned word=0;word<10;word++) {
  OutputMachine m(argv[1]);m.badCommandRead=(packet-1)*10+word+1;assert(!m.RunOutput() && m.packets==packet-1);negative++;
 }
 for(unsigned source=0;source<16;source++){OutputMachine m(argv[1]);m.seals[source]^=1;assert(!m.RunOutput() && !m.packets);negative++;}
 for(unsigned offset:{0x40U,0x840U,0x1040U,0x1840U,0x240U,0x2bcU}) {
  OutputMachine m(argv[1]);m.busyOffset=offset;assert(!m.RunOutput() && !m.packets);negative++;
 }
 printf("PASS OFFLINE ACTUAL RV32 resident projection output:cases%u/refusals%u/maxsteps%u/minstack%08x;ALL7banks/2edges/3FFNroles/3roworders/20-119workers;allrowsONCE/bit-exact69632or20480B/noCPUproducts/nohostPCI/immutablemapandpayload/8wrappedcounters/terminalZEROlaterIO;MATRIX_PRODUCER_AND_NATIVE_CONTROLLER_NOT_INTEGRATED\n",positive,negative,maxSteps,minStack);
}
#endif
