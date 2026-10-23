/* SPDX-License-Identifier: MIT */
// ACTUAL QPR1 parent+QPG1 gather+QPM1 core instructions. Peer computation/
// status/physical endpoints are SYNTHETIC in this suite,NOT native/math.
#define PIONEER_Q38_PROJECTION_GRAPH_PLAN_FIXTURE_ONLY
#include "pioneer_q38_projection_graph_plan_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphProof.h"
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_TEST_PRIVATE_FFN_ROOT
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#include "pioneer_qwen38_dispatch_image_test.cpp"
struct ProjectionParentMachine:DispatchMachine {
 struct Node {
  std::array<uint32_t,16> job={},src={};std::array<uint32_t,64> frame={},proof={};
  std::array<uint32_t,8> count={},before={};
  unsigned sequence=0,requested=0,op=0,polls=0,wave=0;bool stopped=false,verified=false;
 };
 ParentPlanFixture fixture;
 std::vector<uint8_t> prefix,graph;
 std::array<Node,119> node={};
 std::vector<uint32_t> gddr=std::vector<uint32_t>(2*1024*1024/4,0x7fc00001);
 std::vector<unsigned> outputCover;
 unsigned producerJobs=0,matrixJobs=0,stopJobs=0,lineReads=0,operandReads=0,scaleWrites=0,scaleReads=0,outputWrites=0;
 unsigned nocReads=0,badNode=0,badProofWord=~0U,badPcNode=0,badSourceNode=0,badCounterNode=0;
 unsigned mutateAtPacket=0,mutateWord=0,dropPacket=0,badCommandRead=0,commandReadCount=0;
 unsigned rootAliasReads=0,badRootAliasRead=0;
 bool observed=false;
 ProjectionParentMachine(const char* path,unsigned layer=0,unsigned role=10,unsigned bank=0,bool stop=false)
  :DispatchMachine(path,119),fixture(layer,role,bank,stop),prefix(Read((std::string(path)+"/prefix.bin").c_str())),graph(Read((std::string(path)+"/graph.bin").c_str())){
  assert(prefix.size()<=16384 && graph.size()<=24576 && !(prefix.size()&3) && !(graph.size()&3));
  for(unsigned i=0;i<prefix.size()/4;i++)l1[Index(0x64000+i*4)]=Word(prefix,i*4);
  assert(fixture.Build());kind=fixture.jobs->Type();k=fixture.jobs->Inputs();Stage();outputCover.assign(fixture.jobs->Outputs(),0);
  for(unsigned w=0;w<119;w++){
   node[w].sequence=node[w].requested=fixture.sequence[w];node[w].count.fill(0xfffffff8);
   for(unsigned i=0;i<16;i++)node[w].src[i]=fixture.sources[w][i];
  }
  for(unsigned i=0;i<fixture.jobs->Inputs();i++)gddr[i]=Bits(float(int(i%255)-127)*.125f);
 }
 unsigned Index(uint32_t a)override{
  if(Range(a,0x64000,16384))return 4096+(a-0x64000)/4;
  if(Range(a,0x68000,24576))return 2047; // immutable ROM fetch slot,NOT a native mapping grant
  return DispatchMachine::Index(a);
 }
 bool CodeAddress(uint32_t a)const override{return Range(a,0x64000,unsigned(prefix.size())) || Range(a,0x68000,unsigned(graph.size())) || DispatchMachine::CodeAddress(a);}
 void StepBlock()override{if(Range(pc,0x68000,unsigned(graph.size())))l1[2047]=Word(graph,pc-0x68000);DispatchMachine::StepBlock();}
 uint32_t ReadCycleCsr(unsigned c)override{assert(c==0xb00 || c==0xb80);uint64_t now=(UINT64_C(1)<<40)+uint64_t(steps)*19;return c==0xb00?uint32_t(now):uint32_t(now>>32);}
 void Stage(){
  const auto* m=fixture.graph->Manifest();assert(m);
  for(unsigned i=0;i<TT_Q38_PROJECTION_GRAPH_WORDS;i++)memory[TT_Q38_PROJECTION_GRAPH_MANIFEST+i*4]=m[i];
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)memory[TT_Q38_PROJECTION_GRAPH_SOURCES+w*64+i*4]=fixture.sources[w][i];
 }
 uint32_t Load(uint32_t a)override{
  if(Range(a,0x64000,unsigned(prefix.size())))return l1[Index(a)];
  if(Range(a,0x68000,unsigned(graph.size())))return Word(graph,a-0x68000);
  if(Range(a,TT_Q38_PROJECTION_GRAPH_MANIFEST,TT_Q38_PROJECTION_GRAPH_WORDS*4)
   || Range(a,TT_Q38_PROJECTION_GRAPH_SHADOW,TT_Q38_PROJECTION_GRAPH_WORDS*4)
   || Range(a,TT_Q38_PROJECTION_MATRIX_FRAME,256) || Range(a,TT_Q38_PROJECTION_MATRIX_ROWS,119*4)
   || Range(a,TT_Q38_PROJECTION_MATRIX_SHADOW,256+119*4) || Range(a,0x74200,384) || Range(a,0x90e00,128))return memory.at(a);
  if(Range(a,0xffb20000,0x2000)){
   assert(!observed);nocReads++;
   if(a==0xffb20210 && ++rootAliasReads==badRootAliasRead){observed=true;return counts[2]^1;}
   if(Range(a,0xffb20000,40)){
    unsigned word=(a-0xffb20000)/4;seen[word]=true;return command[word]^(++commandReadCount==badCommandRead?1U:0U);
   }
  }
  return DispatchMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(Range(a,0xffb20000,0x2000))assert(!observed);
  if(Range(a,TT_Q38_PROJECTION_GRAPH_SHADOW,TT_Q38_PROJECTION_GRAPH_WORDS*4)
   || Range(a,0x74000,0x1000) || Range(a,TT_Q38_PROJECTION_MATRIX_FRAME,256)
   || Range(a,TT_Q38_PROJECTION_MATRIX_ROWS,119*4) || Range(a,TT_Q38_PROJECTION_MATRIX_SHADOW,256+119*4)
   || Range(a,TT_Q38_PROJECTION_GRAPH_COUNTERS,119*32) || Range(a,TT_Q38_PROJECTION_GRAPH_SEQUENCE,119*4)
   || Range(a,TT_Q38_PROJECTION_GRAPH_STATS,256) || Range(a,0xab000,768)){
   assert(!pending && !unknown && !(a&3));memory[a]=value;return;
  }
  if(a==0x17ffc8 && value==TT_Q38_PROJECTION_GRAPH_FAULT){unknown=true;l1[Index(a)]=value;return;}
  if(Range(a,0x80000,119*1024)){
   assert(!pending && !unknown && !(a&3));unsigned active=memory.at(0x74014);
   for(unsigned w=0;w<active;w++)assert(node[w].op==1 && node[w].sequence==node[w].requested && node[w].verified && !node[w].stopped);
   memory[a]=value;return;
  }
  assert(!Range(a,TT_Q38_PROJECTION_GRAPH_MANIFEST,TT_Q38_PROJECTION_GRAPH_WORDS*4));
  DispatchMachine::Store(a,value);
 }
 void Retire(unsigned bytes,bool write){counts[write?4:0]++;counts[write?5:1]+=(bytes+63)/64;counts[write?7:3]++;counts[2]++;responses++;pending=false;}
 virtual void Advance(unsigned w){
  auto& n=node[w];if(n.sequence==n.requested || ++n.polls<=1+w%3)return;
  if(n.op==5){
   const auto* d=n.frame.data();assert(PioneerQ38ProjectionInputJob(n.job.data(),d,n.src.data(),w+1,Route(w+1)));
   const unsigned delta[]={1,16,3,1,2,10,3,2};for(unsigned i=0;i<8;i++)n.count[i]+=delta[i];
   for(unsigned i=0;i<128;i++)gddr.at(d[12]/4+i)=0x40014001;
   uint32_t line[32]={};for(unsigned i=0;i<(d[14]==8?8U:1U);i++)line[i]=0x3c800000;
   const uint32_t fields[]={TT_Q38_PROJECTION_INPUT_MARKER,d[6],d[5],d[14],d[11],d[42],d[43],d[40],d[41]};
   for(unsigned i=0;i<9;i++)line[8+i]=fields[i];for(unsigned i=0;i<8;i++)line[17+i]=d[32+i];
   line[25]=d[15];line[26]=w+1;line[27]=Route(w+1);line[28]=n.src[0];line[31]=1;
   assert(PioneerQ38ProjectionInputLine(line,d));for(unsigned i=0;i<32;i++)gddr.at(d[13]/4+i)=line[i];
   auto& p=n.proof;p={};for(unsigned i=0;i<8;i++){p[i]=n.before[i];p[8+i]=n.count[i];}
   p[16]=256;p[17]=n.requested;p[18]=w+1;p[19]=TT_Q38_FFN_PERSISTENT_MARKER;
   const uint32_t c[]={0x90e00,0,Route(w+1),d[4]+d[13],0,d[8],0,0x212,128,0};for(unsigned i=0;i<10;i++)p[20+i]=c[i];
   p[30]=n.src[0];p[31]=Route(w+1);for(unsigned i=0;i<16;i++)p[32+i]=n.src[i];
   p[48]=5;p[49]=d[6];p[50]=d[5];p[51]=d[42]|(d[43]<<8)|(d[14]<<16);p[52]=d[40];p[53]=d[41];
   for(unsigned i=0;i<8;i++)p[54+i]=d[32+i];p[62]=TT_Q38_PROJECTION_INPUT_MARKER;p[63]=d[11];producerJobs++;
  }else if(n.op==1){
   const auto* d=n.job.data();unsigned blocks=d[5]>>(d[1]==8?5:8),reads=d[7]*blocks+(d[5]*2+16383)/16384+1;
   unsigned flits=d[7]*blocks*Bytes(d[1])/64+(d[5]*2+((blocks*4+63)&~63U))/64;
   for(unsigned i=0;i<8;i++)n.count[i]+=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0U;
   auto& p=n.proof;p={};for(unsigned i=0;i<8;i++){p[i]=n.before[i];p[8+i]=n.count[i];}
   p[16]=d[7]*d[5]/16;p[17]=n.requested;p[18]=w+1;p[19]=TT_Q38_FFN_PERSISTENT_MARKER;
   const uint32_t c[]={d[6]+(d[7]*blocks-1)*Bytes(d[1]),0,d[8],0x90000,0,Route(w+1),0,0,Bytes(d[1]),0};
   for(unsigned i=0;i<10;i++)p[20+i]=c[i];p[30]=n.src[0];p[31]=Route(w+1);for(unsigned i=0;i<16;i++)p[32+i]=n.src[i];matrixJobs++;
  }else{assert(n.op==2);n.stopped=true;stopJobs++;}
  n.sequence=n.requested;n.verified=true;
 }
 virtual uint32_t PhysicalRegister(unsigned w,unsigned a){
  auto& n=node[w];
  if(a==0xffb13140)return w+1==badPcNode?0xdeadc0dc:n.stopped?TT_Q38_PEER_STOP:0x7204c;
  if(a==0xffb121b0)return 0x46800;
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+BlackholeDramSourceOffset(i))return n.src[i]^((w+1==badSourceNode && n.verified && i==10)?1U:0U);
  const unsigned off[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)if(a==0xffb20000+off[i])return n.count[i]^((w+1==badCounterNode && n.verified && i==0)?1U:0U);
  assert(false && "unclassified/reserved peer register");return 0;
 }
 virtual uint32_t RowOutput(unsigned w,unsigned row)const{return Bits(float(memory.at(TT_Q38_PROJECTION_MATRIX_ROWS+w*4)+row+1));}
 virtual void BeforeDoorbell(unsigned w){
  auto& n=node[w];assert(n.job[10]==TT_Q38_FFN_PERSISTENT_MARKER && n.job[11]==w+1 && n.job[12]==n.sequence+1);
  n.requested=n.job[12];n.op=n.job[13];n.polls=0;n.verified=false;n.before=n.count;
  if(n.op==1)n.wave=memory.at(TT_Q38_PROJECTION_MATRIX_FRAME+5*4);
 }
 virtual void PeerPacket(){
  bool write=command[7]==0x212;unsigned w=Worker(write?command[5]:command[2])-1;auto& n=node[w];
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(w<119 && (write?command[2]:command[5])==129 && bytes && local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));
  if(write){
   if(remote==0x74200){assert(bytes==256);for(unsigned i=0;i<64;i++)n.frame[i]=memory.at(local+i*4);}
   else if(remote==0x74000){assert(bytes==64);for(unsigned i=0;i<16;i++)n.job[i]=memory.at(local+i*4);}
   else{assert(remote==0x74100 && bytes==4 && memory.at(local)==n.sequence+1);BeforeDoorbell(w);}
  }else if(remote==0x74140){
   assert(bytes==64);Advance(w);for(unsigned i=0;i<16;i++)memory[local+i*4]=0;
   memory[local]=n.requested;memory[local+4]=n.sequence;memory[local+12]=TT_Q38_FFN_PERSISTENT_MARKER;
   memory[local+16]=n.sequence==n.requested?(n.stopped?2U:0U):n.op;
  }else if(remote==0x83f00){
   assert(n.sequence==n.requested && n.verified && (bytes==256 || (n.op==1 && bytes==256+n.job[7]*64)));
   for(unsigned i=0;i<64;i++)memory[local+i*4]=n.proof[i]^((w+1==badNode && i==badProofWord)?1U:0U);
   if(bytes>256)for(unsigned row=0;row<n.job[7]*16;row++)memory[local+256+row*4]=RowOutput(w,row);
  }else{assert(bytes==4);memory[local]=PhysicalRegister(w,remote);}
  Retire(bytes,write);
 }
 void DramPacket(){
  bool write=command[7]==0x212;unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  const auto* m=fixture.graph->Manifest();assert((write?command[2]:command[5])==129 && remote>=m[4] && bytes && bytes<=16384 && !(bytes&63));
  unsigned offset=remote-m[4];assert(offset<gddr.size()*4 && bytes<=gddr.size()*4-offset);
  if(!write && local==0x90e00){assert(bytes==128 && offset>=TT_Q38_PROJECTION_LINES_OFFSET && offset<TT_Q38_PROJECTION_LINES_OFFSET+(m[44]/256)*128);lineReads++;}
  else if(!write && Range(local,0xa0000,m[44]*2)){assert(offset==TT_Q38_PROJECTION_NATIVE_OFFSET+local-0xa0000);operandReads++;}
  else if(local==0xab000){assert(offset==TT_Q38_PROJECTION_COMPACT_OFFSET);if(write)scaleWrites++;else scaleReads++;}
  else{
   unsigned destination=PioneerQ38ProjectionOutputOffset(m[42],m[43]);
   assert(write && local>=0x80000 && local+bytes<=0x80000+119*1024 && offset>=destination);
   unsigned first=(offset-destination)/4;assert(first<outputCover.size() && bytes/4<=outputCover.size()-first);
   for(unsigned i=0;i<bytes/4;i++)assert(!outputCover[first+i]++);outputWrites++;
  }
  for(unsigned i=0;i<bytes/4;i++)if(write)gddr.at(offset/4+i)=memory.at(local+i*4);else memory[local+i*4]=gddr.at(offset/4+i);
  Retire(bytes,write);
 }
 void Packet()override{
  assert(pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9]);
  if(packets==dropPacket)return;
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];assert(route!=1555);
  if(route==fixture.graph->Manifest()[8])DramPacket();else PeerPacket();
  if(packets==mutateAtPacket)memory[TT_Q38_PROJECTION_GRAPH_MANIFEST+mutateWord*4]^=1;
 }
 bool Run(){
  while(pc!=TT_Q38_DISPATCH_PARK && pc!=TT_Q38_DISPATCH_FAULT_PARK)StepBlock();assert(minStack>=0x6e000);
  if(pc==TT_Q38_DISPATCH_FAULT_PARK){assert(unknown);return false;}
  assert(l1[Index(0x17ffc8)]==TT_Q38_PROJECTION_GRAPH_MARKER);return l1[Index(0x17ffc4)]==(fixture.stop?119:fixture.jobs->Outputs());
 }
 void Verify(){
  const auto* m=fixture.graph->Manifest();assert(!pending && !unknown);
  uint32_t receipt[64],before[8],after[8];
  for(unsigned i=0;i<64;i++)receipt[i]=memory.at(TT_Q38_PROJECTION_GRAPH_STATS+i*4);
  for(unsigned i=0;i<8;i++){before[i]=receipt[16+i];after[i]=counts[i==6?2:i];}
  assert(PioneerQ38ProjectionGraphReceipt(receipt,m,before,after));
  for(unsigned i=0;i<64;i++){receipt[i]^=0x80000000U;assert(!PioneerQ38ProjectionGraphReceipt(receipt,m,before,after));receipt[i]^=0x80000000U;}
  for(unsigned w=0;w<119;w++)assert(node[w].sequence==PioneerQ38ProjectionGraphNextPeer(m,w)
   && memory.at(TT_Q38_PROJECTION_GRAPH_SEQUENCE+w*4)==node[w].sequence && node[w].stopped==fixture.stop);
  if(fixture.stop){assert(stopJobs==119 && !producerJobs && !matrixJobs && !lineReads && !outputWrites);}
  else{
   assert(producerJobs==m[44]/256 && matrixJobs==m[12] && !stopJobs && lineReads==m[44]/256 && scaleWrites==1 && scaleReads==m[7]);
   for(unsigned row=0;row<m[45];row++)assert(outputCover[row]==1 && gddr.at(PioneerQ38ProjectionOutputOffset(m[42],m[43])/4+row)==Bits(float(row+1)));
  }
  assert(memory.at(TT_Q38_PROJECTION_GRAPH_STATS)==TT_Q38_PROJECTION_GRAPH_MARKER && memory.at(TT_Q38_PROJECTION_GRAPH_STATS+56*4)==1);
  for(unsigned i=0;i<TT_Q38_PROJECTION_GRAPH_WORDS;i++)assert(memory.at(TT_Q38_PROJECTION_GRAPH_MANIFEST+i*4)==m[i]
   && memory.at(TT_Q38_PROJECTION_GRAPH_SHADOW+i*4)==m[i]);
 }
 void Inert(){unsigned p=packets,r=nocReads;for(unsigned i=0;i<32;i++)StepBlock();assert(p==packets && r==nocReads);}
};
#ifndef PIONEER_Q38_PROJECTION_PARENT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned good=0,faults=0,minStack=0x70000;
 const unsigned cases[][2]={{0,10},{3,6},{0,5},{0,13},{0,2},{0,4},{3,10},{64,0},{64,1}};
 for(const auto& selected:cases){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1],selected[0],selected[1],(good+1)%7);assert(m->Run());m->Verify();minStack=std::min(minStack,m->minStack);good++;
  printf("PASS ACTUAL full-projection parent layer%u role%u type%u K%u N%u ALL%u waves/one ROOT call;SYNTHETIC peer replies,NOT_NATIVE_OR_MATH\n",selected[0],selected[1],m->fixture.jobs->Type(),m->fixture.jobs->Inputs(),m->fixture.jobs->Outputs(),m->fixture.jobs->Waves());
 }
 {auto m=std::make_unique<ProjectionParentMachine>(argv[1],0,10,0,true);assert(m->Run());m->Verify();good++;}
 for(unsigned word=0;word<64;word++){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1]);m->badNode=1;m->badProofWord=word;assert(!m->Run() && !m->lineReads && !m->outputWrites);m->Inert();faults++;
 }
 for(unsigned kind=0;kind<3;kind++){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1]);if(!kind)m->badPcNode=1;if(kind==1)m->badSourceNode=1;if(kind==2)m->badCounterNode=1;
  assert(!m->Run() && !m->lineReads && !m->outputWrites);m->Inert();faults++;
 }
 for(unsigned read:{2U,4U,8U}){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1]);m->badRootAliasRead=read;assert(!m->Run() && m->observed);m->Inert();faults++;
 }
 printf("PASS OFFLINE actual QPR1 parent examples%u/faults%u/maxRootStack%u;op5 physical proofs->actual gather->ALL actual matrix-core waves/global output/STOP;SYNTHETIC PEERS_NOT_ALL119_ARITHMETIC_NATIVE_OR_SPEED\n",good,faults,0x70000-minStack);
}
#endif
