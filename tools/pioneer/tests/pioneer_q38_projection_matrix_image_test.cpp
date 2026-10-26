/* SPDX-License-Identifier: MIT */
// ACTUAL NEW ROOT wave instructions;peer arithmetic/physical endpoints are
// explicitly SYNTHETIC here. Canonical geometry is real private plan data,
// NOT authenticated source/initialized pages/native/lifetime/speed evidence.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_TEST_PRIVATE_FFN_ROOT
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#include "pioneer_qwen38_dispatch_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionMatrixShape.h"
#include <memory>
struct ProjectionMatrixMachine:DispatchMachine {
 std::vector<uint8_t> prefix;
 std::array<uint32_t,64> frame={};std::array<uint32_t,119> first={};
 std::vector<uint32_t> remoteInput,remoteScale,output;
 std::vector<uint8_t> cover;
 unsigned inputPackets=0,outputPackets=0,nocReads=0,dramReadbacks=0;
 unsigned failRead=0,failWrite=0,lateSourceRead=0,lateSourceWrite=0,lateFrameRead=0,lateRowsWrite=0;
 unsigned foreignRead=0,foreignWrite=0,badDramReadback=0;
 unsigned foreignPeerPacket=0,peerSourcePacket=0,aliasReads=0,badAliasRead=0;
 unsigned transientPc=0,badPc=0,deferredSnapshots=0;
 bool changedSource=false,observedAlias=false,observedSource=false;
 ProjectionMatrixMachine(const char* path,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38ProjectionJobs& jobs,
  unsigned layer,unsigned role,unsigned wave,unsigned bank=0,bool edge=false)
  :DispatchMachine(path,jobs.WaveWorkers(wave),jobs.Type(),jobs.Inputs()),prefix(Read((std::string(path)+"/prefix.bin").c_str())){
  assert(active && jobs.Matches(plan,layer,role,123));assert(!prefix.empty() && prefix.size()<=0x4000 && !(prefix.size()&3));
  for(unsigned i=0;i<prefix.size()/4;i++)l1[Index(0x64000+i*4)]=Word(prefix,i*4);
  auto& d=frame;d[0]=TT_Q38_PROJECTION_MATRIX_MARKER;d[1]=1;d[2]=67108864;d[3]=bank;
  d[4]=edge?0xc0000000:0x04000000+plan.Workspace(bank)->offset;d[5]=wave;d[6]=17+wave;d[7]=active;
  d[8]=PioneerQ38ProjectionRoute(bank);d[9]=129;d[10]=PioneerQ38ProjectionOutputOffset(layer,role);d[11]=jobs.Outputs();
  d[13]=TT_Q38_PROJECTION_MATRIX_ROWS;d[14]=k;d[15]=kind;
  for(unsigned i=0;i<16;i++)d[16+i]=i?0x12340000+i:0x16005;
  for(unsigned i=32;i<40;i++)d[i]=0x13570000+i;d[40]=123;d[41]=7;d[42]=layer;d[43]=role;d[44]=jobs.Count();d[45]=jobs.Waves();
  const uint32_t h[]={TT_Q38_PROJECTION_MATRIX_MARKER,1,1,kind,k,active,d[6],TT_Q38_PROJECTION_MATRIX_INPUT,1,0,0,0,0,0,0,0};
  for(unsigned i=0;i<16;i++)memory[0x74000+i*4]=h[i];
  for(unsigned w=0;w<119;w++){
   for(unsigned i=0;i<8;i++)memory[0x74080+w*32+i*4]=0;
   if(w>=active)continue;
   const auto& p=*jobs.Part(wave*119+w);first[w]=p.firstRow;
   const uint32_t part[]={0x04000000+uint32_t(p.offset),p.bank,p.groups,wave+1,PioneerQ38ProjectionRoute(p.bank),0x16005,0,0};
   for(unsigned i=0;i<8;i++)memory[0x74080+w*32+i*4]=part[i];d[12]+=p.groups*64;
   peers[w].requested=peers[w].finished=wave;
  }
  SetFrame();SetRows();assert(Shape());
  remoteInput=originalInputs;
  unsigned blocks=k>>(kind==8?5:8),padded=(blocks+15)&~15U;remoteScale.resize(padded);
  for(unsigned i=0;i<padded;i++)remoteScale[i]=memory.at(0xab000+i*4);
  for(unsigned i=0;i<k/2;i++)memory[0xa0000+i*4]=0x7fc17fc1;
  for(unsigned i=0;i<padded;i++)memory[0xab000+i*4]=0x7fc00001;
  output.assign(d[11],0x7fc00001);cover.assign(d[11],0);memory[0x7fbfc]=0xdecafbad;
 }
 unsigned Index(uint32_t a)override{if(Range(a,0x64000,0x4000)){assert(!(a&3));return 4096+(a-0x64000)/4;}return DispatchMachine::Index(a);}
 bool CodeAddress(uint32_t a)const override{return Range(a,0x64000,unsigned(prefix.size())) || DispatchMachine::CodeAddress(a);}
 void SetFrame(){for(unsigned i=0;i<64;i++)memory[TT_Q38_PROJECTION_MATRIX_FRAME+i*4]=frame[i];}
 void SetRows(){for(unsigned i=0;i<119;i++)memory[TT_Q38_PROJECTION_MATRIX_ROWS+i*4]=first[i];}
 bool Shape()const{
  uint32_t h[16],b[8],p[119][8];for(unsigned i=0;i<16;i++)h[i]=memory.at(0x74000+i*4);
  for(unsigned i=0;i<8;i++)b[i]=memory.at(0x74040+i*4);
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<8;i++)p[w][i]=memory.at(0x74080+w*32+i*4);
  return PioneerQ38ProjectionMatrixShape(frame.data(),h,b,p,first.data());
 }
 uint32_t ReadCycleCsr(unsigned c)override{assert(c==0xb00 || c==0xb80);uint64_t now=(UINT64_C(1)<<40)+uint64_t(steps)*19;return c==0xb00?uint32_t(now):uint32_t(now>>32);}
 virtual uint32_t ExpectedOutput(unsigned w,unsigned row)const override{return Bits(float(first[w]+row+1));}
 uint32_t Load(uint32_t a)override{
  if(Range(a,0x64000,unsigned(prefix.size())))return l1[Index(a)];
  if(Range(a,TT_Q38_PROJECTION_MATRIX_FRAME,256) || Range(a,TT_Q38_PROJECTION_MATRIX_ROWS,119*4))return memory.at(a);
  if(Range(a,TT_Q38_PROJECTION_MATRIX_SHADOW,256+119*4))return memory.at(a);
  if(Range(a,0xffb20000,0x2000)){
   assert(!observedAlias && !observedSource);
   nocReads++;
   if(a==0xffb20210 && ++aliasReads==badAliasRead){observedAlias=true;return counts[2]^1;}
   if(Range(a,0xffb20000,40) && (command[7]==0x212?command[5]:command[2])==frame[8]){
    unsigned word=(a-0xffb20000)/4;seen[word]=true;return command[word]^(++dramReadbacks==badDramReadback?1U:0U);
   }
   if(changedSource && a==0xffb2013c){observedSource=true;return (0x12340000+10)^1;}
  }
  return DispatchMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  // Only response endpoint copies can populate native input/root scales.
  assert(!Range(a,0xa0000,k*2) && !Range(a,0xab000,2176)
   && !Range(a,TT_Q38_PROJECTION_MATRIX_FRAME,256) && !Range(a,TT_Q38_PROJECTION_MATRIX_ROWS,119*4));
  if(a==0x17ffc8 && value==TT_Q38_PROJECTION_MATRIX_FAULT){unknown=true;l1[Index(a)]=value;return;}
  if(Range(a,TT_Q38_PROJECTION_MATRIX_SHADOW,256+119*4)){assert(!packets && !pending);memory[a]=value;return;}
  DispatchMachine::Store(a,value);
 }
 std::array<uint32_t,64> Proof(unsigned worker)const override{
  auto p=DispatchMachine::Proof(worker);p[6]=p[2];p[14]=p[10];p[19]=TT_Q38_FFN_PERSISTENT_MARKER;return p;
 }
 void Packet()override{
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];assert(route!=1555);
  if(route==frame[8]){DramPacket();return;}PeerPacket();
 }
 void DramPacket(){
  bool write=command[7]==0x212;unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9]
   && (write?command[2]:command[5])==129 && command[7]==(write?0x212U:0U) && bytes && bytes<=16384 && !(bytes&63));
  if(!write){
   assert(!descriptors && !doorbells && !gathers);unsigned index=inputPackets++,chunks=(k*2+16383)/16384;
   if(inputPackets==failRead){faultAt=packets;return;}
   if(index<chunks){unsigned at=index*16384,n=std::min(16384U,k*2-at);
    assert(local==0xa0000+at && remote==frame[4]+TT_Q38_PROJECTION_NATIVE_OFFSET+at && bytes==n);
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=remoteInput[at/4+i];
   }else{assert(index==chunks && local==0xab000 && remote==frame[4]+TT_Q38_PROJECTION_COMPACT_OFFSET && bytes==remoteScale.size()*4);
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=remoteScale[i];}
   if(inputPackets==lateSourceRead)changedSource=true;
   if(inputPackets==lateFrameRead)memory[TT_Q38_PROJECTION_MATRIX_FRAME+40*4]^=1;
   if(inputPackets==foreignRead)counts[4]++;
  }else{
   assert(AllFinished() && gathers==active);outputPackets++;if(outputPackets==failWrite){faultAt=packets;return;}
   assert(remote>=frame[4]+frame[10]);unsigned row=(remote-frame[4]-frame[10])/4;
   assert(row<frame[11] && bytes/4<=frame[11]-row);
   for(unsigned i=0;i<bytes/4;i++){assert(!cover[row+i]++);output[row+i]=memory.at(local+i*4);}
   if(outputPackets==lateSourceWrite)changedSource=true;
   if(outputPackets==lateRowsWrite)memory[TT_Q38_PROJECTION_MATRIX_ROWS]^=16;
   if(outputPackets==foreignWrite)counts[0]++;
  }
  Retire(bytes,write);
 }
 void Retire(unsigned bytes,bool write){
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)/64;counts[write?7:3]++;counts[2]++;responses++;pending=false;
  if(packets==foreignPeerPacket)counts[write?0:4]++;
  if(packets==peerSourcePacket)changedSource=true;
 }
 virtual void PeerPacket(){
  bool write=command[7]==0x212;unsigned worker=Worker(write?command[5]:command[2]),w=worker-1;auto& peer=peers[w];
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(w<active && pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9]
   && (write?command[2]:command[5])==129 && (write || !command[7]) && local>=0x75000 && bytes && local+bytes<=0x75800 && !((local^remote)&31));
  if(write && remote==0x74000){
   assert(bytes==64 && !doorbells);descriptors++;for(unsigned i=0;i<16;i++)peer.descriptor[i]=memory.at(local+i*4);
   const auto& d=peer.descriptor;unsigned base=0x74080+w*32;
   assert(d[0]==0x31483851 && d[1]==kind && d[2]==Bytes(kind) && !d[3] && !d[4] && d[5]==k
    && d[6]==memory.at(base) && d[7]==memory.at(base+8) && d[8]==memory.at(base+16) && d[9]==1
    && d[10]==TT_Q38_FFN_PERSISTENT_MARKER && d[11]==worker && d[12]==peer.finished+1 && d[13]==1 && d[14]==1 && !d[15]);
  }else if(write){
   assert(remote==0x74100 && bytes==4 && descriptors==active && !peer.launched && memory.at(local)==peer.finished+1);
   peer.requested=memory.at(local);peer.launched=true;doorbells++;
  }else if(remote==0x74140){
   assert(bytes==64);if(peer.launched && ++peer.polls>1+worker%3)peer.finished=peer.requested;
   for(unsigned i=0;i<16;i++)memory[local+i*4]=0;memory[local]=peer.requested;memory[local+4]=peer.finished;
   memory[local+12]=TT_Q38_FFN_PERSISTENT_MARKER;memory[local+16]=peer.finished==peer.requested?0:1;
  }else if(remote==0xffb13140){
   assert(bytes==4);memory[local]=badPc?badPc:transientPc && deferredSnapshots++<2?transientPc:0x7204c;
  }else if(remote==0xffb121b0){assert(bytes==4);memory[local]=0x46800;}
  else{
   assert(remote==0x83f00 && AllFinished() && bytes==256+peer.descriptor[7]*64);gathers++;
   auto proof=Proof(worker);if(worker==corruptWorker && corruptProof<64)proof[corruptProof]^=1;
   for(unsigned i=0;i<64;i++)memory[local+i*4]=proof[i];
   for(unsigned row=0;row<peer.descriptor[7]*16;row++)memory[local+256+row*4]=worker==nonfiniteWorker && !row?0x7fc00001:ExpectedOutput(w,row);
  }
  Retire(bytes,write);
 }
 bool Run(){
  while(pc!=TT_Q38_DISPATCH_PARK && pc!=TT_Q38_DISPATCH_FAULT_PARK)StepBlock();assert(minStack>=0x6e000 && memory.at(0x7fbfc)==0xdecafbad);
  if(pc==TT_Q38_DISPATCH_FAULT_PARK){assert(unknown);return false;}
  assert(l1[Index(0x17ffc8)]==TT_Q38_PROJECTION_MATRIX_MARKER);
  return l1[Index(0x17ffc4)]==active;
 }
 void Verify(){
  assert(!pending && descriptors==active && doorbells==active && !readbacks && gathers==active
   && inputPackets==PioneerQ38ProjectionMatrixInputReads(frame.data()));
  unsigned rows=0;for(unsigned w=0;w<active;w++)for(unsigned r=0;r<memory.at(0x74088+w*32)*16;r++){
   assert(cover[first[w]+r]==1 && output[first[w]+r]==ExpectedOutput(w,r));rows++;
  }
  unsigned covered=0;for(unsigned i=0;i<frame[11];i++)covered+=cover[i];assert(covered==rows && rows*4==frame[12]);
  for(unsigned i=0;i<k/2;i++)assert(memory.at(0xa0000+i*4)==remoteInput[i]);
  for(unsigned i=0;i<remoteScale.size();i++)assert(memory.at(0xab000+i*4)==remoteScale[i]);
  for(unsigned i=0;i<64;i++)assert(memory.at(TT_Q38_PROJECTION_MATRIX_FRAME+i*4)==frame[i]);
  for(unsigned i=0;i<119;i++)assert(memory.at(TT_Q38_PROJECTION_MATRIX_ROWS+i*4)==first[i]);
  assert(memory.at(0x7f8c4)==TT_Q38_PROJECTION_MATRIX_MARKER && memory.at(0x7f8dc)==rows && memory.at(0x7f8e0)==1);
 }
 void Inert(){unsigned reads=nocReads,n=packets;for(unsigned i=0;i<32;i++)StepBlock();assert(reads==nocReads && n==packets);}
};
#ifndef PIONEER_Q38_PROJECTION_MATRIX_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned matrices=0,waves=0,rows=0,faults=0,minStack=0x70000;
 for(unsigned layer=0;layer<=64;layer++)for(unsigned role=0;role<14;role++){
  unsigned t=0,k=0,n=0;if(!plan->ProjectionGeometry(layer,role,t,k,n))continue;
  auto jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(jobs->Build(*plan,layer,role,123));
  assert(PioneerQ38ProjectionJobCount(n)==jobs->Count());std::vector<unsigned> covered(n,0);
  for(unsigned wave=0;wave<jobs->Waves();wave++){
   ProjectionMatrixMachine m(argv[1],*plan,*jobs,layer,role,wave,layer%7,(layer+role+wave)&1);assert(m.Run());m.Verify();waves++;
   for(unsigned r=0;r<n;r++)if(m.cover[r]){assert(!covered[r]++);rows++;}
   minStack=std::min(minStack,m.minStack);
  }
  for(auto present:covered)assert(present==1);matrices++;
  if(layer==64 || (layer%8==0 && role==2))printf("PASS ACTUAL projection ROOT layer%u role%u type%u K%u N%u ALL%u waves/exact global row placement;SYNTHETIC peers NOT_NATIVE\n",layer,role,t,k,n,jobs->Waves());
 }
 auto small=std::make_unique<PioneerQ38ProjectionJobs>();assert(small->Build(*plan,0,10,123));
 for(unsigned w=0;w<64;w++){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);m.corruptWorker=3;m.corruptProof=w;
  assert(!m.Run() && !m.outputPackets);m.Inert();faults++;
 }
 for(unsigned at:{0x67fe0U,0x71f80U,0x73fe4U,0x72080U,0x7209cU,0x72060U,0xdeadc0dcU}){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);m.badPc=at;assert(!m.Run() && !m.outputPackets);m.Inert();faults++;
 }
 for(unsigned at:{0x66aacU,0x6733cU,0x71000U}){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);m.transientPc=at;assert(m.Run());m.Verify();faults++;
 }
 for(unsigned stage=1;stage<=2;stage++)for(unsigned fault=0;fault<4;fault++){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);if(!fault)m.failRead=stage;if(fault==1)m.lateSourceRead=stage;
  if(fault==2)m.lateFrameRead=stage;if(fault==3)m.foreignRead=stage;
  assert(!m.Run() && m.inputPackets==stage && !m.descriptors && !m.outputPackets);m.Inert();faults++;
 }
 for(unsigned fault=0;fault<4;fault++){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);if(!fault)m.failWrite=1;if(fault==1)m.lateSourceWrite=1;
  if(fault==2)m.lateRowsWrite=1;if(fault==3)m.foreignWrite=1;
  assert(!m.Run() && m.outputPackets==1);m.Inert();faults++;
 }
 for(unsigned word:{0U,1U,2U,3U,4U,5U,7U,8U,9U,10U,11U,12U,13U,14U,15U,44U,45U,46U,63U}){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);m.frame[word]^=word==3?7U:1U;m.SetFrame();assert(!m.Run() && !m.packets);faults++;
 }
 for(unsigned w:{0U,1U,2U,118U}){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);m.first[w]^=1;m.SetRows();assert(!m.Run() && !m.packets);faults++;
 }
 for(unsigned word=0;word<20;word++){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);m.badDramReadback=word+1;assert(!m.Run() && m.inputPackets==word/10 && !m.descriptors);m.Inert();faults++;
 }
 for(uint32_t bits:{0x80000000U,0xbc800000U,0x7fc00001U,0x00000001U}){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);m.remoteScale[0]=bits;assert(!m.Run() && !m.descriptors && !m.outputPackets);m.Inert();faults++;
 }
 for(unsigned read:{2U,4U,8U,12U,16U,20U}){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);m.badAliasRead=read;
  assert(!m.Run() && !m.outputPackets && m.observedAlias && m.aliasReads==read);m.Inert();faults++;
 }
 for(unsigned packet:{3U,4U,5U,6U,7U,8U})for(unsigned kind=0;kind<2;kind++){
  ProjectionMatrixMachine m(argv[1],*plan,*small,0,10,0);if(!kind)m.foreignPeerPacket=packet;else m.peerSourcePacket=packet;
  assert(!m.Run() && !m.outputPackets && m.packets==packet);m.Inert();faults++;
 }
 printf("PASS OFFLINE ACTUAL QPM1 ROOT matrices%u/waves%u/full-row-cover%u/faults%u/maxDeviceStack%u;ALL498 types+small64/3workers+9wave vocab/strict no-host endpoints;SYNTHETIC PEERS_NOT_NATIVE_OWNER_FULL_MODEL_OR_SPEED\n",matrices,waves,rows,faults,0x70000-minStack);
}
#endif
