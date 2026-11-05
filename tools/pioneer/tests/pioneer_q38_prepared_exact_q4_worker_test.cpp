/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_EXACT_Q4_WORK_MACHINE_ONLY
#include "pioneer_q38_exact_q4_work_worker_test.cpp"
#include "../PioneerQ38PreparedQ4Layout.h"
#include "../PioneerQ38Q4EscapeTiles.h"
using PreparedLayout=PioneerQ38PreparedQ4Layout;
struct PreparedQ4Peer:ExactQ4WorkPeer {
 using L=PreparedLayout;using Codec=PioneerQ38Q4EscapeTiles;
 static constexpr unsigned CorrectionBase=0x20000000;
 std::vector<std::vector<uint8_t>> native;
 std::vector<uint8_t> correction;
 std::array<unsigned,16> descriptor{};
 std::vector<Packet> inputPackets;
 std::vector<bool> completedTiles;
 std::map<unsigned,unsigned> extraMemory;
 std::set<unsigned> inputDefined,maskDefined;
 unsigned maskTile=UINT32_MAX,extraReads=0,extraFlits=0,payloadWritten=0,expectedPayload=0,prefixTiles=0;
 unsigned sourceRoute=0x47;
 bool descriptorReady=false,indexReady=false,payloadReady=false;
 PreparedQ4Peer(const char* fixture,const char* image,unsigned type,unsigned k,unsigned groups,unsigned prefix=0,unsigned worker=69,unsigned route=0x47):ExactQ4WorkPeer(fixture,image,type,k,groups,worker),prefixTiles(prefix),sourceRoute(route){
  if(prefix){assert(type==12);base+=prefix*2304;matrixJob[6]=base;memory[0x74018]=base;}
  matrixJob[8]=sourceRoute;memory[0x74020]=sourceRoute;
  inputPackets.assign(packets.begin(),packets.begin()+inputReads);PrepareSource(1);
 }
 static unsigned Get(const uint8_t* p){return p[0]|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
 static void Put(uint8_t* p,unsigned v){for(unsigned i=0;i<4;i++)p[i]=uint8_t(v>>(i*8));}
 L::Window ExpectedGroup(unsigned group)const{
  unsigned first=UINT32_MAX,last=0;
  for(unsigned block=0;block<Blocks();block++){
   const unsigned index=Get(correction.data()+(prefixTiles+group*Blocks()+block)*4);
   if((index>>30)==2)continue;
   first=std::min(first,index&0x3fffffff);last=std::max(last,index&0x3fffffff);
  }
  if(first==UINT32_MAX)return {0,0};
  const unsigned begin=first/64*64,logical=descriptor[9],end=(std::min(logical,last+512)+63)/64*64;
  return {begin,end-begin}; // independent endpoint geometry,NOT the shader's planner
 }
 void PrepareSource(unsigned request){
  if(type!=12)return;
  const unsigned totalTiles=prefixTiles+unsigned(tiles.size()),indexBytes=(totalTiles*4+15)&~15U;
  correction.assign(indexBytes,0);native.clear();
  for(unsigned t=0;t<prefixTiles;t++)Put(correction.data()+t*4,0x80000000);
  for(unsigned t=0;t<tiles.size();t++){
   std::array<uint8_t,2816> encoded{};assert(Codec::Encode(tiles[t].data(),2304,encoded.data(),encoded.size()));
   native.emplace_back(encoded.begin(),encoded.begin()+2304);std::array<uint8_t,512> packed{};Codec::Mask mask;
   assert(Codec::PackMask(encoded.data()+2304,512,packed.data(),packed.size(),mask));
   const unsigned offset=mask.bytes?unsigned(correction.size()):0;
   Put(correction.data()+(prefixTiles+t)*4,(mask.kind<<30)|offset);
   if(mask.bytes){correction.resize(offset+((mask.bytes+15)&~15U),0);memcpy(correction.data()+offset,packed.data(),mask.bytes);}
  }
  const unsigned logical=unsigned(correction.size());correction.resize((logical+4095)&~4095U,0);
  descriptor={L::Magic,logicalWorker,request,12,inputs,FfnPersistentPeer::groups,base,sourceRoute,
   CorrectionBase,logical,prefixTiles,totalTiles,base-prefixTiles*2304,totalTiles*2304,1,0};
  unsigned fakeJob[16]={};fakeJob[1]=12;fakeJob[2]=2304;fakeJob[5]=inputs;fakeJob[6]=base;fakeJob[7]=FfnPersistentPeer::groups;
  fakeJob[8]=sourceRoute;fakeJob[11]=logicalWorker;fakeJob[12]=request;fakeJob[14]=1;assert(L::Shape(descriptor.data(),fakeJob));
  packets=inputPackets;packets.push_back({0xab400+(logicalWorker-1)*64,129,L::Descriptor,64,-2});
  for(unsigned group=0;group<FfnPersistentPeer::groups;group++){
   const unsigned indexOffset=(prefixTiles+group*Blocks())*4,indexStart=indexOffset/64*64,indexBytes=(indexOffset%64+Blocks()*4+63)/64*64;
   packets.push_back({CorrectionBase+indexStart,sourceRoute,L::Index,indexBytes,-3});
   const auto window=ExpectedGroup(group);
   for(unsigned copied=0;copied<window.bytes;){
    const unsigned n=std::min(16384U,window.bytes-copied);
    packets.push_back({CorrectionBase+window.start+copied,sourceRoute,L::Payload+copied,n,-4});copied+=n;
   }
   for(unsigned b=0;b<Blocks();b+=Batch()){
    const unsigned t=group*Blocks()+b;packets.push_back({base+t*2304,sourceRoute,Buffer(t),Batch()*2304,int(t)});
   }
  }
  completedTiles.assign(tiles.size(),false);extraMemory.clear();inputDefined.clear();maskDefined.clear();maskTile=UINT32_MAX;
  descriptorReady=indexReady=payloadReady=false;extraReads=extraFlits=payloadWritten=expectedPayload=0;
 }
 unsigned ExpectedReadPackets()const override{return type==12?unsigned(packets.size()):ExactQ4WorkPeer::ExpectedReadPackets();}
 virtual unsigned PreparedSourceWord(const Packet& p,unsigned at)const{
  if(p.tile>=0)return Word(native.at(unsigned(p.tile)+at/2304),at%2304);
  if(p.tile==-1)return root.at(p.address+at);
  if(p.tile==-2)return descriptor.at(at/4);
  return Get(correction.data()+p.address-CorrectionBase+at);
 }
 void CompletePreparedRead(){
  assert(pending && requests && requests!=faultAt);const auto& p=packets.at(requests-1);
  for(unsigned at=0;at<p.bytes;at+=4){
   const unsigned value=PreparedSourceWord(p,at);
   memory[p.destination+at]=value;if(p.tile<-1)extraMemory[p.destination+at]=value;
  }
  if(p.tile>=0)for(unsigned i=0;i<p.bytes/2304;i++)completedTiles.at(unsigned(p.tile)+i)=true;
  else if(p.tile<-1){
   extraReads++;extraFlits+=p.bytes/64;
   if(p.tile==-2)descriptorReady=true;
   if(p.tile==-3){
    indexReady=true;payloadWritten=0;const auto window=ExpectedGroup(stageCalls/Blocks());
    expectedPayload=window.bytes;payloadReady=!expectedPayload;
   }
   if(p.tile==-4){assert(p.destination==L::Payload+payloadWritten);payloadWritten+=p.bytes;assert(payloadWritten<=expectedPayload);payloadReady=payloadWritten==expectedPayload;}
  }
  pending=false;responses++;
 }
 uint32_t RawInputBase()const override{
  if(type!=12 || !matrixScope)return ExactQ4WorkPeer::RawInputBase();
  const unsigned t=CurrentTile(),raw=CurrentRaw(t);assert(completedTiles.at(t));
  if(pending)assert(packets.at(requests-1).destination!=Buffer(t));
  if(validatedTile!=t){for(unsigned i=0;i<2304;i+=4)assert(memory.at(raw+i)==Word(native.at(t),i));validatedTile=t;}
  return raw;
 }
 bool DisjointPrefetchCompute()const override{
  if(type!=12)return ExactQ4WorkPeer::DisjointPrefetchCompute();
  if(!matrixScope || !pending || !stageCalls || stageReturn || !requests)return false;
  const unsigned t=CurrentTile(),next=t-t%Batch()+Batch();const auto& p=packets.at(requests-1);
  return next<tiles.size() && next%Blocks()!=0 && p.tile==int(next) && p.address==base+next*2304
   && p.destination!=Buffer(t) && p.destination==Buffer(next) && p.bytes==Batch()*2304
   && responses==requests-1 && matrixInstructions>=t*TilePhases() && matrixInstructions<=(t+1)*TilePhases()
   && RawInputBase()==CurrentRaw(t);
 }
 void NativeOperand(const Task& t,unsigned w)override{
  if(type!=12){ExactQ4WorkPeer::NativeOperand(t,w);return;}
  const unsigned panel=(matrixInstructions/3)%16,block=(matrixInstructions/3/16)%Blocks();
  if(w){
   assert(t.address==L::Input(block*128+panel*8));
   for(unsigned at=t.address-16;at<t.address+256;at+=4)assert(inputDefined.count(at));
  }else if(t.row==0){assert(!t.flip && !partial[unpack[0]] && t.address==RawInputBase()+256+panel*128);}
  else{
   assert((t.row==16 && t.address==0x9c140 && !t.flip && partial[unpack[0]]==1)
    || (t.row==32 && t.address==0x9c240 && t.flip && partial[unpack[0]]==3));
   for(unsigned at=t.address-16;at<t.address;at+=4)assert(!memory.at(at));
  }
 }
 bool SourceRange(unsigned a)const{return Range(a,L::Payload,L::PayloadCapacity) || Range(a,L::Index,L::IndexCapacity) || Range(a,L::Descriptor,64);}
 bool InputRange(unsigned a)const{return Range(a,L::InputFirst-16,4608+224+16) || Range(a,L::InputSecond-16,inputs*2-4608+224+16);}
 unsigned Load(unsigned a)override{
  if(type!=12 || !matrixScope)return ExactQ4WorkPeer::Load(a);
  if(a==0xffb20040){
   doorbellRead=true;if(pending && ++waits>delay && requests!=faultAt)CompletePreparedRead();
   return unsigned(pending);
  }
  if(SourceRange(a)){
   if(Range(a,L::Descriptor,64))assert(descriptorReady);
   else if(Range(a,L::Index,L::IndexCapacity))assert(indexReady && extraMemory.count(a));
   else assert(payloadReady && a<L::Payload+expectedPayload && extraMemory.count(a));
   return memory.at(a);
  }
  if(InputRange(a)){assert(inputDefined.count(a));return memory.at(a);}
  if(Range(a,L::DenseMask,512)){assert(maskTile==CurrentTile() && maskDefined.size()==128);return memory.at(a);}
  if(Range(a,L::Summary,64))return memory.at(a);
  return ExactQ4WorkPeer::Load(a);
 }
 void Store(unsigned a,unsigned value)override{
  if(type!=12 || !matrixScope){ExactQ4WorkPeer::Store(a,value);return;}
  if(SourceRange(a)){fprintf(stderr,"FORBIDDEN_PREPARED_SOURCE_WRITE %08x\n",a);assert(false);}
  if(InputRange(a) || Range(a,L::DenseMask,512) || Range(a,L::Summary,64)){
   assert(exact && MathIdle() && !(a&3));memory[a]=value;dirty=true;
   if(InputRange(a))inputDefined.insert(a);
   if(Range(a,L::DenseMask,512)){
    const unsigned t=CurrentTile();if(maskTile!=t){maskDefined.clear();maskTile=t;}maskDefined.insert(a);
   }
   return;
  }
  // The unchanged caller primes the OLD last-tile command before entering
  // the new helper. It issues NO read there;the helper's descriptor fetch
  // retargets it later. Observe all four exact original fields/readbacks.
  if(!exact && requests==inputReads && !stageCalls && !pushes && Range(a,0xffb20000,40)){
   assert(MathIdle() && drained && !pending && !(a&3));const unsigned w=(a-0xffb20000)/4;
   assert((w==0 && value==base) || (w==2 && value==sourceRoute) || (w==3 && value==0x90000) || (w==8 && value==2304));
   command[w]=value;readback[w]=false;return;
  }
  // FetchStage restores the legacy last-tile command without issuing a
  // packet,even when the next actual packet is a correction-group read.
  // Keep that event under the original exact restore observer;only the
  // new helper (outside FetchStage) may configure the additional reads.
  const bool extra=!stageReturn && Range(a,0xffb20000,40) && requests<packets.size() && packets.at(requests).tile<-1 && !pushCommand;
  if(extra || (a==0xffb20040 && !command[7] && requests<packets.size() && packets.at(requests).tile<-1)){
   assert(MathIdle() && !rawPending && phaseQueue.empty() && pendingAccumulator.empty());
   if(Range(a,0xffb20000,40)){
    const unsigned w=(a-0xffb20000)/4;const auto& p=packets.at(requests);
    const unsigned expected=w==0?p.address:w==2?p.route:w==3?p.destination:p.bytes;
    if(value!=expected)fprintf(stderr,"PREPARED_COMMAND_MISMATCH pc%08x stage%u read%u kind%d word%u actual%08x expected%08x\n",pc,stageCalls,requests,p.tile,w,value,expected);
   }
   if(a==0xffb20040){
    const int kind=packets.at(requests).tile;
    if(kind==-2)descriptorReady=false;
    if(kind==-3){indexReady=false;payloadReady=false;payloadWritten=expectedPayload=0;}
    if(kind==-4)payloadReady=false;
   }
   SharedRowsMachine::Store(a,value);return;
  }
  ExactQ4WorkPeer::Store(a,value);
 }
 void CheckScratchCanaries()const override{
  if(type!=12){ExactQ4WorkPeer::CheckScratchCanaries();return;}
  for(unsigned at=0x90000;at<0x90900;at+=4){
   const auto prior=extraMemory.find(at);assert(memory.at(at)==(prior==extraMemory.end()?0x7fc17fc1:prior->second));
  }
  for(unsigned at:{0x97ffcU,0x99a80U,0x99ffcU,0x9ba80U,0x9bffcU})assert(memory.at(at)==0xdec0adfe);
 }
 void CheckEnd()override{
  ExactQ4WorkPeer::CheckEnd();if(type!=12)return;
  assert(memory.at(L::Summary)==L::SummaryMagic && memory.at(L::Summary+20)==extraReads && memory.at(L::Summary+24)==extraFlits);
 }
 void ReceivePreparedQueued(){ExactQ4WorkPeer::ReceiveQueued();PrepareSource(2);}
};
#ifndef PIONEER_Q38_PREPARED_WORKER_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==4 || (argc==5 && (!strcmp(argv[4],"--pilot") || !strcmp(argv[4],"--group-pilot") || !strcmp(argv[4],"--controls-only") || !strcmp(argv[4],"--faults-only"))));
 setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 const bool pilot=argc==5 && !strcmp(argv[4],"--pilot"),groupPilot=argc==5 && !strcmp(argv[4],"--group-pilot"),faultsOnly=argc==5 && !strcmp(argv[4],"--faults-only"),controlsOnly=argc==5 && !strcmp(argv[4],"--controls-only");
 unsigned cases=0,faults=0,semantic=0,controls=0,stack=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}){
  if(controlsOnly || faultsOnly || (pilot && (type!=12 || k!=5120 || groups!=1)))continue;
  if(groupPilot && (type!=12 || k!=5120 || groups!=3))continue;
  printf("PREPARED_Q4_COMPLETE_CASE type%u K%u groups%u\n",type,k,groups);
  auto old=std::make_unique<ExactQ4WorkPeer>(argv[1],argv[2],type,k,groups);
  auto next=std::make_unique<PreparedQ4Peer>(argv[1],argv[3],type,k,groups);
  for(unsigned side=0;side<2;side++){
   auto& p=side?static_cast<ExactQ4WorkPeer&>(*next):*old;p.delay=3;
   p.BootPushed();p.Prepare();p.StagePushed();p.AwaitPayload();p.FinishPushed(true);p.CheckEnd();
  }
  for(unsigned job=1;job<=2;job++){
   assert(next->requests==old->requests+next->extraReads && next->responses==next->requests && !next->exact && !next->dotProbeReads);
   for(unsigned r=0;r<groups*16;r++)assert(next->buffers[4096+r]==old->buffers[4096+r]);
   for(unsigned i=0;i<56;i++)assert(old->mailbox[i]==next->mailbox[i]); // actual raw pre-push counters legitimately differ
   if(job==2)break;
   for(unsigned side=0;side<2;side++){
    auto& p=side?static_cast<ExactQ4WorkPeer&>(*next):*old;std::array<uint32_t,16> first;
    for(unsigned i=0;i<16;i++)first[i]=p.buffers[4096+i];
    if(side)next->ReceivePreparedQueued();else p.ReceiveQueued();
    p.AwaitSecond();p.FinishSecond(first);stack=std::max(stack,0x70000-p.minStack);
   }
  }
  printf("PASS_PREPARED_Q4_COMPLETE_CASE type%u K%u groups%u changedJobs2 sourceReads%u extraReads%u extraFlits%u RV%u/%u;NOT_SILICON_TPS\n",
   type,k,groups,next->requests,next->extraReads,next->extraFlits,old->steps,next->steps);cases++;
 }
 if(!pilot && !groupPilot && !faultsOnly){
  for(unsigned nonzero:{0U,1U,239U,240U,256U}){
   auto old=std::make_unique<ExactQ4WorkPeer>(argv[1],argv[2],12,5120,3);
   auto next=std::make_unique<PreparedQ4Peer>(argv[1],argv[3],12,5120,3,13);
   old->base+=13*2304;old->matrixJob[6]=old->base;old->memory[0x74018]=old->base;
   for(auto& p:old->packets)if(p.tile>=0)p.address+=13*2304;
   for(unsigned t=0;t<next->tiles.size();t++)for(unsigned panel=0;panel<16;panel++)for(unsigned i=0;i<256;i++){
    const unsigned row=panel*16+i/16,q=(row+t*17)%256<nonzero?15:7;
    auto& byte=next->tiles[t][PreparedQ4Peer::Codec::OriginalAt(panel,i)];
    const unsigned shift=panel&2?4:0;byte=uint8_t((byte&~(15U<<shift))|(q<<shift));
   }
   old->tiles=next->tiles;next->PrepareSource(1);
   for(unsigned side=0;side<2;side++){
    auto& p=side?static_cast<ExactQ4WorkPeer&>(*next):*old;p.delay=3;
    p.BootPushed();p.Prepare();p.StagePushed();p.AwaitPayload();p.FinishPushed();p.CheckEnd();p.StopLocal();
    stack=std::max(stack,0x70000-p.minStack);
   }
   assert(next->requests==old->requests+next->extraReads);
   for(unsigned r=0;r<48;r++)assert(old->buffers[4096+r]==next->buffers[4096+r]);
   for(unsigned i=0;i<56;i++)assert(old->mailbox[i]==next->mailbox[i]);
   printf("PASS_PREPARED_Q4_FRAGMENT_MASK_CONTROL prefixTiles13 groups3 nonzeroRows%u sourceReads%u extraReads%u realSTOP1\n",nonzero,next->requests,next->extraReads);controls++;
  }
 }
 if(!pilot && !groupPilot && !controlsOnly){
  for(unsigned type:{8U,12U,14U})for(unsigned which=0;which<(type==12?8U:4U);which++){
   auto p=std::make_unique<PreparedQ4Peer>(argv[1],argv[3],type,17408,3);
   p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
   p->faultAt=which==0?1:which==1?p->inputReads+1:which==2?p->inputReads+2:which==3?unsigned(p->packets.size())
    :which==4?p->inputReads+3:which==5?p->inputReads+4:which==6?p->inputReads+5:p->inputReads+6;
   p->ReadFault();faults++;
  }
  for(unsigned field=0;field<16;field++){
   auto p=std::make_unique<PreparedQ4Peer>(argv[1],argv[3],12,5120,1);p->descriptor[field]^=0x80000001U;
   p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
   while(p->pc!=p->faultPC && !p->heldPayload)p->StepBlock();
   assert(p->pc==p->faultPC && !p->heldPayload && !p->matrixInstructions && !p->pushes && !p->mailbox[0]);
   const auto memory=p->memory;const unsigned requests=p->requests,responses=p->responses;
   for(unsigned i=0;i<2048;i++)p->StepBlock();
   assert(p->pc==p->faultPC && p->memory==memory && p->requests==requests && p->responses==responses);semantic++;
  }
 }
 printf("%s_PREPARED_Q4_COMPLETE_WORKER shapes%u changedInputJobs%u fragmentMaskControls%u NoCLossInert%u descriptorRefusals%u stack%u/8192;"
  "originalFP32_sourceNoC_ACK_commit_STOP_NO_NATIVE_ROOT_MODEL_HARDWARE_OR_TPS\n",argc==5?"PARTIAL":"PASS",cases,cases*2,controls,faults,semantic,stack);
}
#endif
