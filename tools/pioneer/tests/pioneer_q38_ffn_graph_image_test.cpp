/* SPDX-License-Identifier: MIT */
// ACTUAL root instructions ONLY. Peer proof/values/physical CSR fabric is
// SYNTHETIC. This is NOT real peer arithmetic/native or inference evidence.
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_TEST_PRIVATE_FFN_ROOT
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#include "pioneer_qwen38_dispatch_image_test.cpp"
#include "pioneer_q38_ffn_graph_fixture.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnMatrixShape.h"
struct GraphRootMachine:DispatchMachine {
 struct Node {std::array<uint32_t,16> job={},src={};std::array<uint32_t,64> frame={},proof={};
  std::array<uint32_t,8> counter={},before={};unsigned sequence=0,requested=0,op=0,polls=0,role=0;bool stopped=false;};
 FfnGraphFixture fixture;
 std::vector<uint8_t> prefix,graph;
 std::array<Node,119> node={};std::vector<uint32_t> gddr=std::vector<uint32_t>(303104/4,0);
 unsigned captures=0,startPackets=0,nocReads=0,stages[5]={},phaseWrites[3]={},nativeLoads=0,compactWrites=0;
 unsigned corruptNode=0,corruptWord=~0U,physicalSourceFault=0,physicalCounterFault=0;
 unsigned badStatusNode=0,badPcNode=0,badResetNode=0,mutateManifestPacket=0,mutateSourcePacket=0;
 std::array<uint32_t,16> rootSource={};
 GraphRootMachine(const char* path,bool wide=true,unsigned bank=0,bool edge=false,bool reverse=false,unsigned types=0)
  :DispatchMachine(path,119),fixture(bank,edge,wide,reverse,types),prefix(Read((std::string(path)+"/prefix.bin").c_str())),graph(Read((std::string(path)+"/graph.bin").c_str())) {
  assert(prefix.size()<=16384 && graph.size()<=24576 && !(prefix.size()&3) && !(graph.size()&3));
  for(unsigned i=0;i<prefix.size()/4;i++)l1[Index(0x64000+i*4)]=Word(prefix,i*4);
  for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)memory[TT_Q38_FFN_GRAPH_MANIFEST+i*4]=fixture.manifest[i];
  for(unsigned w=0;w<119;w++){for(unsigned i=0;i<16;i++)node[w].src[i]=fixture.source[w*16+i];node[w].counter.fill(0xfffffff8);}
  for(unsigned i=0;i<16;i++)rootSource[i]=fixture.manifest[16+i];
 }
 unsigned Index(uint32_t a)override {
  if(Range(a,0x64000,16384))return 4096+(a-0x64000)/4;
  if(Range(a,0x68000,24576))return 2047; // sparse immutable ROM fetch slot ONLY
  return DispatchMachine::Index(a);
 }
 bool CodeAddress(uint32_t a)const override{return Range(a,0x64000,unsigned(prefix.size())) || Range(a,0x68000,unsigned(graph.size())) || DispatchMachine::CodeAddress(a);}
 void StepBlock()override {
  // No larger physical/mapped data authority:the extended immutable ROM
  // is backed by its ACTUAL section bytes,not the old fixture's L1 array.
  if(Range(pc,0x68000,unsigned(graph.size())))l1[2047]=Word(graph,pc-0x68000);
  DispatchMachine::StepBlock();
 }
 uint32_t ReadCycleCsr(unsigned csr)override{assert(csr==0xb00 || csr==0xb80);uint64_t now=UINT64_C(0x10000000000)+uint64_t(steps)*19;return csr==0xb00?uint32_t(now):uint32_t(now>>32);}
 uint32_t Load(uint32_t a)override {
  if(Range(a,0x64000,unsigned(prefix.size())))return Word(prefix,a-0x64000);
  if(Range(a,0x68000,unsigned(graph.size())))return Word(graph,a-0x68000);
  if(Range(a,0xffb20000,0x2000))nocReads++;
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+BlackholeDramSourceOffset(i))return rootSource[i];
  if(Range(a,TT_Q38_FFN_GRAPH_MANIFEST,0x8000) || Range(a,0x7f900,256) || Range(a,0x7fa00,119*4)
   || Range(a,0xa0000,34816) || Range(a,0xab000,320))return memory.at(a);
  return DispatchMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override {
  assert(!Range(a,TT_Q38_FFN_GRAPH_MANIFEST,TT_Q38_FFN_GRAPH_WORDS*4)); // original NEVER writable
  if(Range(a,TT_Q38_FFN_GRAPH_SHADOW,TT_Q38_FFN_GRAPH_WORDS*4)) {
   assert(packets==startPackets && captures<TT_Q38_FFN_GRAPH_WORDS && a==TT_Q38_FFN_GRAPH_SHADOW+captures*4);
   assert(value==fixture.manifest[captures]);captures++;memory[a]=value;return;
  }
  if(Range(a,TT_Q38_FFN_GRAPH_COUNTERS,119*32) || Range(a,TT_Q38_FFN_GRAPH_SEQUENCE,119*4) || Range(a,TT_Q38_FFN_GRAPH_STATS,256)
   || Range(a,0x74000,0x1000) || Range(a,0x7f900,256) || Range(a,0x7fa00,119*4) || Range(a,0xab000,320)) {
   assert(!unknown && !pending);memory[a]=value;return;
  }
  if(a==0x17ffc8 && value==TT_Q38_FFN_GRAPH_FAULT){unknown=true;l1[Index(a)]=value;return;}
  if(Range(a,0x80000,69632)) {
   assert(!pending && !unknown && memory.at(0x74000)==TT_Q38_FFN_MATRIX_MARKER && memory.at(0x74020)==1
    && a-0x80000<memory.at(0x7f930));
   for(unsigned w=0;w<memory.at(0x74014);w++)assert(node[w].requested==node[w].sequence);
   memory[a]=value;return;
  }
  // The OLD checked proof-clear/store path would hit original/shadow data
  // and is forbidden above. Work must never enter it,even during STOP.
  DispatchMachine::Store(a,value);
 }
 uint32_t Output(unsigned role,unsigned w,unsigned row)const {
  const auto* m=fixture.manifest.data()+PioneerQ38FfnGraphRoleOffset(role);unsigned first=m[16+119*8+w];
  return Bits(float(role+1)*.25f+float(first+row)*.000001f);
 }
 void FinishNode(unsigned w) {
  auto& p=node[w];const auto& j=p.job;p.before=p.counter;p.proof={};unsigned reads=0,flits=0,writes=0,writeFlits=0;
  if(p.op==1){unsigned blocks=j[5]/256;reads=j[7]*blocks+(j[5]*2+16383)/16384+1;flits=j[7]*blocks*j[2]/64+(j[5]*2+((blocks*4+63)&~63U))/64;}
  if(p.op==3 || p.op==4) {
   reads=p.op==4?1:2;flits=p.op==4?16:32;writes=2;writeFlits=9;
   assert(p.op==4?PioneerQ38FfnPersistentInputShape(j.data(),p.frame.data(),p.src.data(),w+1,Route(w+1))
    :PioneerQ38FfnPersistentActivationShape(j.data(),p.frame.data(),p.src.data(),w+1,Route(w+1)));
   unsigned at=p.frame[12]/4;for(unsigned i=0;i<128;i++)gddr.at(at+i)=0x40014001;
   auto* line=gddr.data()+p.frame[13]/4;line[0]=0x3f800000;line[1]=p.frame[0];line[2]=p.frame[6];line[3]=w;line[4]=p.frame[42];
   line[5]=p.frame[40];line[6]=p.frame[41];for(unsigned i=0;i<8;i++)line[7+i]=p.frame[32+i];line[15]=0;
  }
  for(unsigned i=0;i<8;i++)p.counter[i]+=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  auto& proof=p.proof;for(unsigned i=0;i<8;i++){proof[i]=p.before[i];proof[8+i]=p.counter[i];}
  proof[16]=p.op==1?j[7]*j[5]/16:256;proof[17]=j[12];proof[18]=w+1;proof[19]=TT_Q38_FFN_PERSISTENT_MARKER;
  if(p.op==1) {
   const uint32_t cmd[]={j[6]+(j[7]*(j[5]/256)-1)*j[2],0,j[8],0x90000,0,Route(w+1),0,0,j[2],0};
   for(unsigned i=0;i<10;i++)proof[20+i]=cmd[i];
  }else if(p.op==3 || p.op==4) {
   const auto& d=p.frame;const uint32_t cmd[]={0x90e00,0,d[9],d[4]+d[13],0,d[8],0,0x212,64,0};for(unsigned i=0;i<10;i++)proof[20+i]=cmd[i];
   proof[48]=p.op;proof[49]=d[6];proof[50]=w;proof[51]=d[42];proof[52]=d[40];proof[53]=d[41];
   for(unsigned i=0;i<8;i++)proof[54+i]=d[32+i];proof[62]=d[0];proof[63]=1;
  }
  proof[30]=p.src[0];proof[31]=Route(w+1);for(unsigned i=0;i<16;i++)proof[32+i]=p.src[i];
  p.sequence=p.requested;p.stopped=p.op==2;
  if(w+1==corruptNode && corruptWord<64)p.proof[corruptWord]^=1;
  if(w+1==physicalSourceFault)p.src[11]^=1;
  if(w+1==physicalCounterFault)p.counter[0]++;
 }
 uint32_t Register(unsigned w,unsigned at)const {
  const auto& p=node[w];for(unsigned i=0;i<16;i++)if(at==0xffb20000+BlackholeDramSourceOffset(i))return p.src[i];
  const unsigned off[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)if(at==0xffb20000+off[i])return p.counter[i];
  assert(false && "unclassified/reserved physical CSR read");return 0;
 }
 void Packet()override {
  assert(pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9]);bool write=command[7]==0x212;
  unsigned route=write?command[5]:command[2],local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(route!=1555 && (write?command[2]:command[5])==129 && (write || !command[7]) && bytes && bytes<=16384);
  if(route==fixture.manifest[8]) {
   assert(remote>=fixture.manifest[4] && bytes<=303104 && remote-fixture.manifest[4]<=303104-bytes && !(bytes&63));unsigned at=(remote-fixture.manifest[4])/4;
   if(write){for(unsigned i=0;i<bytes/4;i++)gddr.at(at+i)=memory.at(local+i*4);if(remote==fixture.manifest[4]+30720 || remote==fixture.manifest[4]+274560)compactWrites++;}
   else{for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=gddr.at(at+i);if(local>=0xa0000 && local<0xa8800)nativeLoads++;}
  }else {
   unsigned w=Worker(route)-1;auto& p=node[w];assert(local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));
   if(write && remote==0x74200){assert(bytes==256);for(unsigned i=0;i<64;i++)p.frame[i]=memory.at(local+i*4);}
   else if(write && remote==0x74000){assert(bytes==64);for(unsigned i=0;i<16;i++)p.job[i]=memory.at(local+i*4);assert(p.job[10]==TT_Q38_FFN_PERSISTENT_MARKER && p.job[11]==w+1);
    if(p.job[13]==1){p.role=memory.at(0x7f914);assert(p.role<3);const auto* m=fixture.manifest.data()+PioneerQ38FfnGraphRoleOffset(p.role);const auto* part=m+16+w*8;
     assert(w<m[2] && p.job[0]==0x31483851 && p.job[1]==m[1] && p.job[2]==Bytes(m[1]) && !p.job[3] && !p.job[4]
      && p.job[5]==m[3] && p.job[6]==part[0] && p.job[7]==part[2] && p.job[8]==part[4] && p.job[9]==1 && p.job[14]==1 && !p.job[15]);}
   }
   else if(write){assert(remote==0x74100 && bytes==4 && p.requested==p.sequence && memory.at(local)==p.sequence+1 && memory.at(local)==p.job[12]);p.requested=memory.at(local);p.op=p.job[13];p.polls=0;stages[p.op]++;}
   else if(remote==0x74140) {
    assert(bytes==64);if(p.requested!=p.sequence && ++p.polls>1)FinishNode(w);
    for(unsigned i=0;i<16;i++)memory[local+i*4]=0;
    memory[local]=p.requested;memory[local+4]=p.sequence;memory[local+12]=TT_Q38_FFN_PERSISTENT_MARKER;
    memory[local+16]=p.requested==p.sequence?(p.stopped?2:0):p.op;
    if(w+1==badStatusNode)memory[local+8]=TT_Q38_FFN_PERSISTENT_FAULT;
   }else if(remote==0xffb13140){assert(bytes==4);memory[local]=w+1==badPcNode?0xdeadc0dc:p.stopped?TT_Q38_PEER_STOP:0x7204c;}
   else if(remote==0xffb121b0){assert(bytes==4);memory[local]=w+1==badResetNode?0:0x46800;}
   else if(remote==0x83f00) {
    assert(p.sequence==p.requested && bytes==(p.op==1?256+p.job[7]*64:256));
    for(unsigned i=0;i<64;i++)memory[local+i*4]=p.proof[i];
    if(p.op==1)for(unsigned i=0;i<p.job[7]*16;i++)memory[local+256+i*4]=Output(p.role,w,i);
   }else{for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=Register(w,remote+i*4);}
  }
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)/64;counts[write?7:3]++;counts[2]++;responses++;pending=false;
  if(packets==mutateManifestPacket)memory[TT_Q38_FFN_GRAPH_MANIFEST+40*4]^=1;
  if(packets==mutateSourcePacket)rootSource[11]^=1;
 }
 bool RunGraph() {
  while(pc!=TT_Q38_DISPATCH_PARK && pc!=TT_Q38_DISPATCH_FAULT_PARK)StepBlock();
  assert(minStack>=0x6e000 && captures==TT_Q38_FFN_GRAPH_WORDS);
  if(unknown)return false;assert(l1[Index(0x17ffc8)]==TT_Q38_FFN_GRAPH_MARKER);
  return l1[Index(0x17ffc4)]==(fixture.manifest[5]==1?5120U:119U);
 }
 void VerifyGraph(){assert(!unknown && !pending);for(unsigned w=0;w<119;w++)assert(node[w].sequence==PioneerQ38FfnGraphNextPeer(fixture.manifest.data(),w));
  if(fixture.manifest[5]==1){assert(stages[4]==20 && stages[3]==68 && compactWrites==2);for(unsigned row=0;row<5120;row++)assert(gddr.at(274880/4+row)==Bits(.75f+float(row)*.000001f));}
 }
 void StopRequest() {
  assert(!unknown && !pending && pc==TT_Q38_DISPATCH_PARK);VerifyGraph();
  auto& d=fixture.manifest;d[5]=2;d[6]=d[13];d[13]=d[6]+1;d[14]=0;
  for(unsigned w=0;w<119;w++)d[64+w]=node[w].sequence;
  assert(PioneerQ38FfnGraphManifest(d.data(),fixture.source.data()));
  for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)memory[TT_Q38_FFN_GRAPH_MANIFEST+i*4]=d[i];
  pc=0x6000;regs[0]=0;captures=0;startPackets=packets;
 }
 void Inert(){unsigned old=packets,reads=nocReads;for(unsigned i=0;i<64;i++)StepBlock();assert(old==packets && reads==nocReads);}
};
int main(int argc,char** argv){assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positives=0,faults=0,minStack=0x70000;
 for(bool wide:{true,false})for(unsigned bank=0;bank<7;bank++) {
  GraphRootMachine root(argv[1],wide,bank,(bank&1)!=0,(bank&2)!=0,bank);
  if(!root.RunGraph()){printf("FAIL ROOT PC%08x stage%08x detail%u packets%u;SYNTHETIC_TEST_NOT_NATIVE\n",root.pc,root.l1[root.Index(0x17ffc0)],root.l1[root.Index(0x17ffc4)],root.packets);return 1;}
  root.VerifyGraph();minStack=std::min(minStack,root.minStack);positives++;
  printf("PASS ACTUAL QFG1 complete7stage bank%u all119%u ROOT VMsteps%u packets%u;SYNTHETIC_PEERS_NOT_NATIVE_ARITHMETIC_OR_SPEED\n",bank,unsigned(wide),root.steps,root.packets);
  root.StopRequest();assert(root.RunGraph());root.VerifyGraph();for(const auto& node:root.node)assert(node.stopped);positives++;
 }
 for(unsigned word=0;word<64;word++){GraphRootMachine root(argv[1]);root.corruptNode=1;root.corruptWord=word;
  assert(!root.RunGraph() && root.unknown);root.Inert();faults++;}
 for(unsigned worker:{1U,60U,119U})for(unsigned fault=0;fault<5;fault++) {
  GraphRootMachine root(argv[1]);if(!fault)root.physicalSourceFault=worker;if(fault==1)root.physicalCounterFault=worker;
  if(fault==2)root.badStatusNode=worker;if(fault==3)root.badPcNode=worker;if(fault==4)root.badResetNode=worker;
  assert(!root.RunGraph() && root.unknown);root.Inert();faults++;
 }
 for(unsigned fault=0;fault<4;fault++){GraphRootMachine root(argv[1]);
  if(!fault)root.badReadback=true;if(fault==1)root.faultAt=1;if(fault==2)root.mutateManifestPacket=1;if(fault==3)root.mutateSourcePacket=1;
  assert(!root.RunGraph() && root.unknown);root.Inert();faults++;
 }
 printf("PASS OFFLINE ACTUAL QFG1 root:positive%u/faults%u/maxDeviceStackBytes%u;complete7stage FFN+ALL119STOP/independent physical counters/ALLsourcewords/exact shadow/UNKNOWN inert;SYNTHETIC_PEER_PROTOCOL_VALUES_NO_NATIVE_ARITHMETIC_MODEL_OR_SPEED_AUTHORITY\n",positives,faults,0x70000-minStack);
}
