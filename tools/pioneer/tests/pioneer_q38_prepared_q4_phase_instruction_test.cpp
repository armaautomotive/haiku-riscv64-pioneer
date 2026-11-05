/* SPDX-License-Identifier: MIT */
// Complete emitted Projection::Run phases, actual119 retained workers.
// Real authenticated model/cache BYTES, deterministic raw input, simulated
// NoC/ACK/CSRs. No native owner, physical card, whole-model or TPS claim.
#define PIONEER_Q38_PREPARED_COORDINATOR_MACHINE_ONLY
#include "pioneer_q38_prepared_q4_coordinator_instruction_test.cpp"
#define PIONEER_GRAPH_MATH_BASE_ALREADY_INCLUDED
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "../PioneerQ38PreparedQ4RootTables.h"
#include "../PioneerQ38MixedRecurrentProjectionPlan.h"
#include <fcntl.h>
using FullUpload=PioneerQ38PreparedQ4UploadPlan<>;
struct PhaseSource {
 using Cache=PioneerModelWeightCache;
 FILE* file=nullptr;int fd=-1;struct stat identity{};
 std::unique_ptr<PioneerQwen38Gguf> model=std::make_unique<PioneerQwen38Gguf>();
 std::unique_ptr<PioneerQwen38PackedPlan> original=std::make_unique<PioneerQwen38PackedPlan>();
 std::unique_ptr<PioneerQ38NativeQ4WeightCache> weights=std::make_unique<PioneerQ38NativeQ4WeightCache>();
 PioneerQ38NativeQ4CachedPages pages;
 std::unique_ptr<FullUpload> upload=std::make_unique<FullUpload>();
 std::unique_ptr<PioneerQ38PreparedQ4RootTables<>> tables=std::make_unique<PioneerQ38PreparedQ4RootTables<>>();
 struct Blob {unsigned bank,address;std::vector<uint8_t> bytes;};
 std::map<uint64_t,Blob> blobs;std::map<unsigned,std::vector<uint8_t>> originals;
 std::string error;
 static uint64_t Key(unsigned bank,unsigned address){return uint64_t(bank)<<32|address;}
 bool Ready()const{
  struct stat now{};if(fd<0 || !model->Stable() || fstat(fd,&now))return false;
  return identity.st_dev==now.st_dev && identity.st_ino==now.st_ino && identity.st_size==now.st_size
   && identity.st_mtimespec.tv_sec==now.st_mtimespec.tv_sec && identity.st_mtimespec.tv_nsec==now.st_mtimespec.tv_nsec
   && identity.st_ctimespec.tv_sec==now.st_ctimespec.tv_sec && identity.st_ctimespec.tv_nsec==now.st_ctimespec.tv_nsec;
 }
 PhaseSource(const char* path,const char* directory){
  assert(path[0]=='/' && directory[0]=='/');fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);
  file=fdopen(fd,"rb");assert(file && !fstat(fd,&identity) && S_ISREG(identity.st_mode));
  Cache::Digest sha;puts("PREPARED_PHASE_AUTH_SAME_READ_ONLY_FD_NO_DEVICE");
  assert(Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error)
   && Cache::Hex(sha)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
  assert(model->Open(file) && original->Build(*model,8192,true,true) && Ready());
  const auto recipe=PioneerQ38NativeQ4WeightCache::Recipe(PIONEER_NATIVE_Q4_PREPARATION_SHA256);
  assert(weights->Bind(directory,*model,*original,sha,Cache::Hash(recipe.data(),recipe.size()),[this]{return Ready();},error));
  PioneerQ38NativeQ4WeightCache::Statistics stats;
  while(!weights->AllPrepared()){
   assert(weights->PrepareNext(stats,error,[](uint64_t,uint64_t,bool verifying){return verifying;}));
   assert(!stats.convertedTiles && stats.manifestHits==stats.fragments && stats.bundleHits==stats.fragments);
  }
  assert(pages.Bind(directory,*weights,[this]{return Ready();},error) && upload->Build(*weights,pages,error)
   && tables->Build(*upload,*model,error) && Ready());
  printf("PREPARED_PHASE_EXISTING_CACHE fragments%llu converted0 source851\n",(unsigned long long)stats.fragments);
 }
 ~PhaseSource(){tables.reset();upload.reset();weights.reset();model.reset();if(file)assert(!fclose(file));}
 void Load(unsigned role){
  auto raw=[this](unsigned f,unsigned p,uint8_t* out,unsigned n){return Ready() && weights->OriginalPlan().ReadWeightPage(*model,f,p,out,n) && Ready();};
  for(unsigned s=0;s<upload->Spans().size();s++){
   const auto& span=upload->Spans()[s];const auto& part=weights->OriginalPlan().Parts()[span.fragment];
   if(part.layer || part.role!=role || blobs.count(Key(span.bank,span.address)))continue;
   Blob b{span.bank,span.address,std::vector<uint8_t>(span.allocated)};
   for(unsigned p=0;p<span.allocated/4096;p++)assert(upload->ReadPage(s,p,b.bytes.data()+p*4096,4096,raw,error));
   assert(blobs.emplace(Key(b.bank,b.address),std::move(b)).second);
   if(span.kind==FullUpload::PreparedPrimary){
    auto& old=originals[span.fragment];old.resize(span.allocated);
    for(unsigned p=0;p<span.allocated/4096;p++)assert(raw(span.fragment,p,old.data()+p*4096,4096));
   }else if(span.kind==FullUpload::Original)originals[span.fragment]=blobs.at(Key(span.bank,span.address)).bytes;
  }
  assert(Ready());printf("PREPARED_PHASE_LOADED_REAL_LAYER0_ROLE%u immutableSourceBlobs%zu\n",role,blobs.size());
 }
 const Blob& Find(unsigned route,unsigned address,unsigned bytes)const{
  unsigned bank=7;for(unsigned b=0;b<7;b++)if(PioneerQ38ProjectionRoute(b)==route)bank=b;assert(bank<7);
  auto p=blobs.upper_bound(Key(bank,address));assert(p!=blobs.begin());--p;
  const auto& blob=p->second;assert(blob.bank==bank && address>=blob.address && uint64_t(address)-blob.address+bytes<=blob.bytes.size());return blob;
 }
 unsigned WordAt(unsigned route,unsigned address)const{const auto& b=Find(route,address,4);return PreparedQ4Peer::Get(b.bytes.data()+address-b.address);}
 unsigned Fragment(unsigned role,unsigned bank,unsigned address)const{
  const auto& parts=weights->OriginalPlan().Parts();for(unsigned f=0;f<parts.size();f++){
   const auto& p=parts[f];if(!p.kind && !p.layer && p.role==role && p.bank==bank && address>=p.proposedAddress && address-p.proposedAddress<p.bytes)return f;
  }assert(false);return 0;
 }
};
struct PhaseRoot;
struct PhasePeer:PreparedQ4Peer {
 PhaseRoot* fabric=nullptr;bool active=false,preciseActive=false;unsigned precisePackets=0;
 mutable unsigned checkedSequence=UINT32_MAX,referenceEvaluations=0,referenceReuses=0;
 mutable std::array<unsigned,4096> checkedOutput{};
 PhasePeer(const char* old,const char* image,unsigned worker):PreparedQ4Peer(old,image,8,5120,1,0,worker){}
 void Receive();void FinishPhase();
 unsigned PreparedSourceWord(const Packet& packet,unsigned at)const override;
 unsigned Load(unsigned address)override;
 void SubmitActivation()override;
 void Store(unsigned address,unsigned value)override;
 bool CorrectRows()const override{
  // The strict publisher calls this for every command word. Reuse ONLY
  // this job's already checked immutable-input reference, comparing EVERY
  // output/canary word again. Never skip an emitted firmware instruction.
  if(checkedSequence==sequence){
   assert(matrixInstructions==ExpectedMatrixInstructions());
   for(unsigned i=0;i<4096;i++)assert(checkedOutput[i]==buffers[4096+i]);referenceReuses++;return true;
  }
  if(!PreparedQ4Peer::CorrectRows())return false;
  assert(matrixInstructions==ExpectedMatrixInstructions());checkedSequence=sequence;referenceEvaluations++;
  for(unsigned i=0;i<4096;i++)checkedOutput[i]=buffers[4096+i];return true;
 }
};
struct PhaseRoot:ScalarMachine {
 enum:unsigned{Owner=0x6e000,Frame=Owner+768,Return=0x60004};
 const PreparedRootImage& image;PhaseSource& source;
 std::array<std::unique_ptr<PhasePeer>,119> peers;
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::unique_ptr<PioneerQ38MixedRecurrentProjectionPlan> phase;
 std::array<unsigned,1920> sources{};std::array<unsigned,10> command{};std::array<bool,10> readback{};
 std::array<unsigned,8> counts{};std::array<unsigned,4> slave{};
 std::map<uint64_t,unsigned> dram;
 unsigned entry=0,packets=0,scatter=0,minimumStack=0x70000,faultWrites=0,role=0,completed=0;
 unsigned jobWrites=0,doorbells=0,receipts=0,preciseCompleted=0;
 PhaseRoot(const PreparedRootImage& rom,PhaseSource& data,const char* fixture,const char* worker):ScalarMachine(0,0),image(rom),source(data){
  assert(canonical->Build());std::ifstream sym;
  for(unsigned i=0;i<16;i++)sources[i]=i?0x76540000+i:0x16005;
  for(unsigned w=0;w<119;w++){
   peers[w]=std::make_unique<PhasePeer>(fixture,worker,w+1);auto& p=*peers[w];p.fabric=this;p.delay=3;
   for(unsigned i=0;i<16;i++){p.source[i]+=w*0x100;sources[(w+1)*16+i]=p.boot[16+i]=p.source[i];}
   p.boot[5]=p.source[0];for(unsigned i=0;i<64;i++)p.memory[M::Boot+i*4]=p.boot[i];p.BootPushed();
  }
  for(unsigned a=0x6e000;a<0x70000;a+=4)memory[a]=0x7fc55555;
  const unsigned h[]={RR::Magic,1,64,129,sources[0],119,8192,64,1,0,2,0,3,0,4,0};
  for(unsigned i=0;i<64;i++)memory[RR::Boot+i*4]=memory[Owner+i*4]=i<16?h[i]:i<32?sources[i-16]:0;
  for(unsigned i=0;i<128;i++)memory[RR::Outer+i*4]=memory[Owner+256+i*4]=0;
  memory[RR::Outer+48*4]=memory[Owner+256+48*4]=memory[RR::Doorbell]=1;
  for(unsigned i=0;i<16;i++)memory[0xffb20000U+BlackholeDramSourceOffset(i)]=sources[i];
  for(unsigned i=0;i<4;i++)memory[0xffb20040U+i*0x800]=0;
  memory[0xffb20064U]=0x10101010;memory[0xffb20068U]=0;
  for(unsigned i=0;i<32;i++)memory[0xffb20240U+i*4]=0;
  for(unsigned w=0;w<119;w++){
   memory[RP::Nonces+w*4]=0;
   for(unsigned i=0;i<64;i++)memory[RP::PeerJobs+w*256+i*4]=0;
  }
  counts.fill(0xfffffff8);pc=0;regs[2]=0x70000;regs[27]=Owner;
 }
 static uint64_t Key(unsigned route,unsigned address){return uint64_t(route)<<32|address;}
 unsigned RootWord(unsigned address)const{return memory.at(address);}
 unsigned BankWord(unsigned route,unsigned address)const{
  const auto found=dram.find(Key(route,address));return found==dram.end()?source.WordAt(route,address):found->second;
 }
 void SetPhase(unsigned selected,unsigned function){
  assert(!pc || pc==Return);role=selected;entry=function;source.Load(role);
  phase=std::make_unique<PioneerQ38MixedRecurrentProjectionPlan>();
  assert(phase->Build(*canonical,0,role,0,0,0,1000+role,1,{1,2,3,4},sources.data()));
  const auto* d=phase->Frame();assert(d[7]==119);
  for(unsigned i=0;i<64;i++)memory[RR::Header+i*4]=memory[Frame+i*4]=d[i];
  memory[Owner+1024]=role==5?4:role==2?10:role==3?11:13;
  for(unsigned w=0;w<119;w++){
   for(unsigned i=0;i<8;i++)memory[RP::Parts+w*32+i*4]=phase->Parts()[w][i];
   memory[RP::Rows+w*4]=phase->Rows()[w];
  }
  for(unsigned i=0;i<16;i++)memory[RP::MatrixHeader+i*4]=phase->Header()[i];
  for(unsigned i=0;i<8;i++)memory[RP::Banks+i*4]=phase->Banks()[i];
  if(role>=2 && role<=4){
   std::array<uint8_t,PR::Bytes> bytes{};assert(source.tables->Read(0,role,0,bytes.data(),bytes.size()));
   const unsigned remote=d[4]+0x1000000+PR::PhaseOffset(role);
   for(unsigned i=0;i<bytes.size()/4;i++)dram[Key(d[8],remote+i*4)]=PreparedQ4Peer::Get(bytes.data()+i*4);
  }
  if(role!=4)for(unsigned i=0;i<d[14];i++)memory[d[53]+i*4]=Bits(float(std::sin(i*.013+role*.17))*.7f+float(std::cos(i*.037))*.2f);
  completed=0;printf("PREPARED_COMPLETE_RUN_BEGIN role%u K%u rows%u workers%u\n",role,d[14],d[11],d[7]);
 }
 bool CodeAddress(unsigned a)const override{return image.Find(a);}
 unsigned Index(unsigned a)override{const auto* s=image.Find(a);assert(s && !(a&3));l1[8180]=Word(s->bytes,a-s->base);return 8180;}
 unsigned StepLimit()const override{return 1000000000;}
 unsigned Load(unsigned a)override{
  assert(!(a&3));if(CodeAddress(a))return l1[Index(a)];
  if(a>=0xffb20000U && a<0xffb20028U){const unsigned i=(a-0xffb20000U)/4;readback[i]=true;return command[i];}
  for(unsigned i=0;i<8;i++)if(a==0xffb20000U+PreparedRootMachine::Counter(i))return counts[i];
  const unsigned offsets[]={0x2f0,0x2e0,0x2e8,0x2c4};for(unsigned i=0;i<4;i++)if(a==0xffb20000U+offsets[i])return slave[i];
  if(a>=Push::Mailbox && a<Push::Mailbox+119*256){
   const unsigned w=(a-Push::Mailbox)/256;if(peers[w]->active)peers[w]->FinishPhase();
  }
  if(!memory.count(a))fprintf(stderr,"PHASE_ROOT_UNMAPPED %08x pc%08x role%u\n",a,pc,role);
  return memory.at(a);
 }
 unsigned PeerWord(PhasePeer& p,unsigned at){
  if(at==0xffb121b0)return 0x46800;
  if(at==0xffb13140)return p.pc;
  for(unsigned i=0;i<16;i++)if(at==0xffb20000U+BlackholeDramSourceOffset(i))return p.source[i];
  for(unsigned i=0;i<8;i++)if(at==0xffb20000U+PreparedRootMachine::Counter(i))return p.Count(i);
  for(unsigned i=0;i<4;i++)if(at==0xffb20040U+i*0x800)return 0;
  if(at>=0xffb20240U && at<0xffb202c0U)return 0;
  if(at>=0x84000 && at<0x88000)return p.buffers[4096+(at-0x84000)/4];
  if(!p.memory.count(at))fprintf(stderr,"PHASE_PEER_UNMAPPED w%u %08x pc%08x role%u\n",p.logicalWorker,at,pc,role);
  return p.memory.at(at);
 }
 void Submit(){
  const bool write=command[7]==0x212;assert(write || !command[7]);
  const unsigned remote=command[write?3:0],route=command[write?5:2],local=command[write?0:3],bytes=command[8];
  assert(command[write?2:5]==129 && bytes && bytes<=16384 && !((remote|local|bytes)&3));
  unsigned w=119;for(unsigned i=0;i<119;i++)if(route==M::Peer(i+1))w=i;
  if(w<119){
   auto& p=*peers[w];if(p.active && p.preciseActive && !write)p.FinishPhase();assert(!p.active);
   for(unsigned i=0;i<bytes/4;i++)if(write)p.memory[remote+i*4]=memory.at(local+i*4);else memory[local+i*4]=PeerWord(p,remote+i*4);
   if(write && remote==M::Job){assert(bytes==256);jobWrites++;}
   if(write && remote==M::Doorbell){assert(bytes==4);p.Receive();doorbells++;}
  }else{
   assert(route==phase->Frame()[8]);
   for(unsigned i=0;i<bytes/4;i++)if(write)dram[Key(route,remote+i*4)]=memory.at(local+i*4);else memory[local+i*4]=BankWord(route,remote+i*4);
   if(write)scatter++;
  }
  for(unsigned i=0;i<8;i++)counts[i]+=i==(write?5U:1U)?(bytes+63)/64:
   i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U;
  packets++;
 }
 void Store(unsigned a,unsigned v)override{
  assert(!(a&3));
  if(a>=0xffb20000U && a<0xffb20028U){const unsigned i=(a-0xffb20000U)/4;command[i]=v;readback[i]=false;return;}
  if(a==0xffb20040U){assert(v==1);for(bool b:readback)assert(b);assert(!command[1] && !command[4] && !command[6] && !command[9]);Submit();return;}
  if(a==0x75460 || a==0x75448){
   assert((a==0x75460 && pc==image.faultLoop-24 && !faultWrites) || (a==0x75448 && pc==image.faultLoop-8 && faultWrites==1 && v==0x51465431));
   memory[a]=v;faultWrites++;return;
  }
  assert((a>=0x6e410 && a<0x70000) || (a>=RP::Summary && a<0x180000));
  // The raw private owner and immutable ABI mirrors may never be changed.
  assert(!(a>=RR::Boot && a<RR::Boot+256) && !(a>=RR::Header && a<RR::Header+256) && !(a>=RR::Outer && a<RR::Outer+512));
  if(a<0x70000)minimumStack=std::min(minimumStack,a);memory[a]=v;
 }
 void Incoming(const PhasePeer& p,unsigned bytes){
  assert((bytes==256 && !p.payload[0]) || (bytes==4 && p.memory.at(Push::PushBuffer)==Push::ReceiptMagic));
  for(unsigned i=0;i<bytes/4;i++)memory[Push::Slot(p.logicalWorker)+i*4]=p.memory.at(Push::PushBuffer+i*4);
  slave[0]++;slave[1]+=(bytes+63)/64;slave[2]++;slave[3]++;if(bytes==4)receipts++;
 }
 void RunPhase(){
  assert(!pc || pc==Return);pc=entry;regs[1]=Return;regs[10]=Frame;
  while(pc!=Return && pc!=image.faultLoop)StepScalar();
  if(pc==image.faultLoop)fprintf(stderr,"COMPLETE_PHASE_FAULT role%u afterPackets%u jobs%u doorbells%u complete%u returnPC%08x\n",role,packets,jobWrites,doorbells,completed,memory.at(0x75460));
  assert(pc==Return && !faultWrites && regs[2]==0x70000 && regs[27]==Owner && regs[10]==phase->Frame()[11] && completed==119);
  unsigned outputs=0;for(unsigned w=0;w<119;w++){
   const auto& p=*peers[w];assert(!p.active && p.pc==p.waitPC);
   for(unsigned row=0;row<p.FfnPersistentPeer::groups*16;row++){
    assert(dram.at(Key(phase->Frame()[8],phase->Frame()[4]+phase->Frame()[10]+(phase->Rows()[w]+row)*4))==p.buffers[4096+row]);outputs++;
   }
   for(unsigned i=0;i<8;i++)assert(memory.at(RP::Counters+w*32+i*4)==p.Count(i));
  }
  assert(outputs==phase->Frame()[11] && source.Ready());
  printf("PASS_PREPARED_COMPLETE_RUN role%u actualWorkers119 actualROOTQuantizer%u actualAdmitGatherScatter1 outputRows%u receipts%u actualRootInstructions%u;NOT_NATIVE_MODEL_OR_TPS\n",role,unsigned(role!=4),outputs,receipts,steps);
 }
};
unsigned PhasePeer::PreparedSourceWord(const Packet& p,unsigned at)const{
 assert(fabric);if(p.route==129)return fabric->RootWord(p.address+at);return fabric->source.WordAt(p.route,p.address+at);
}
void PhasePeer::Receive(){
 assert(fabric && !active && pc==waitPC && !pending && !matrixScope && MathIdle() && BanksFree());
 std::array<unsigned,8> physical;for(unsigned i=0;i<8;i++)physical[i]=Count(i);
 const unsigned next=memory.at(M::Job+4),op=memory.at(M::Job+8);assert(next==sequence+1 && (op==1 || op==3));
 for(unsigned i=0;i<8;i++)countBase[i]=physical[i];NextPackets();
 if(op==3){
  sequence=next;denseActive=gdnActive=convActive=outputActive=false;activation=preciseActive=gdnLifetime=true;precisePackets=0;
  requests=responses=0;
  for(unsigned i=0;i<64;i++)FfnPersistentPeer::descriptor[i]=memory.at(PioneerQ38PreciseWorkerLayout::Descriptor+i*4);
  assert(PioneerQ38PreciseWorkerLayout::DescriptorShape(FfnPersistentPeer::descriptor.data()));active=true;return;
 }
 preciseActive=gdnLifetime=denseActive=gdnActive=convActive=outputActive=activation=false;
 sequence=next;type=memory.at(M::Job+17*4);inputs=memory.at(M::Job+21*4);base=memory.at(M::Job+22*4);
 FfnPersistentPeer::groups=memory.at(M::Job+23*4);sourceRoute=memory.at(M::Job+24*4);count=type==8?32:256;
 tile.assign(type==8?576:2304,0);tiles.assign(FfnPersistentPeer::groups*Blocks(),tile);vector.resize(inputs);scales.resize(inputs/count);
 const auto& source=fabric->source;const auto& layout=source.weights->OriginalPlan();
 unsigned bank=7;for(unsigned b=0;b<7;b++)if(sourceRoute==PioneerQ38ProjectionRoute(b))bank=b;assert(bank<7);
 const unsigned fragment=source.Fragment(fabric->role,bank,base);const auto& part=layout.Parts()[fragment];
 const unsigned tileFirst=(base-part.proposedAddress)/unsigned(tile.size());
 for(unsigned t=0;t<tiles.size();t++)memcpy(tiles[t].data(),source.originals.at(fragment).data()+(tileFirst+t)*tile.size(),tile.size());
 root.clear();packets.clear();inputPackets.clear();const unsigned scaleBytes=(unsigned(scales.size()*4)+63)&~63U;
 for(unsigned at=0;at<inputs*2;at+=16384)packets.push_back({0xa0000+at,129,0xa0000+at,std::min(16384U,inputs*2-at),-1});
 packets.push_back({0xab000,129,0x94000,scaleBytes,-1});inputReads=unsigned(packets.size());inputPackets=packets;
 for(unsigned i=0;i<inputs/2;i++)root[0xa0000+i*4]=fabric->RootWord(RP::Native+i*4);
 for(unsigned i=0;i<scaleBytes/4;i++)root[0xab000+i*4]=fabric->RootWord(RP::Scales+i*4);
 for(unsigned i=0;i<inputs;i++)vector[i]=uint16_t(root.at(0xa0000+i/2*4)>>((i%2)*16));
 for(unsigned i=0;i<scales.size();i++)scales[i]=root.at(0xab000+i*4);
 if(type==12){
  FullUpload::Job job;std::string error;assert(source.upload->Describe(fragment,fabric->phase->Rows()[logicalWorker-1],FfnPersistentPeer::groups,logicalWorker,sequence,job,error));
  for(unsigned i=0;i<16;i++)descriptor[i]=job.descriptor[i];prefixTiles=descriptor[10];
  const auto& side=source.Find(sourceRoute,descriptor[8],descriptor[9]);correction=side.bytes;native.clear();
  const auto& primary=source.Find(sourceRoute,base,unsigned(tiles.size()*2304));
  for(unsigned t=0;t<tiles.size();t++)native.emplace_back(primary.bytes.begin()+base-primary.address+t*2304,primary.bytes.begin()+base-primary.address+(t+1)*2304);
  packets.push_back({PreparedLayout::RootSlot(logicalWorker),129,PreparedLayout::Descriptor,64,-2});
  for(unsigned group=0;group<FfnPersistentPeer::groups;group++){
   const unsigned offset=(prefixTiles+group*Blocks())*4,first=offset/64*64,n=(offset%64+Blocks()*4+63)/64*64;
   packets.push_back({descriptor[8]+first,sourceRoute,PreparedLayout::Index,n,-3});const auto window=ExpectedGroup(group);
   for(unsigned copied=0;copied<window.bytes;){const unsigned bytes=std::min(16384U,window.bytes-copied);packets.push_back({descriptor[8]+window.start+copied,sourceRoute,PreparedLayout::Payload+copied,bytes,-4});copied+=bytes;}
   for(unsigned b=0;b<Blocks();b+=Batch()){const unsigned t=group*Blocks()+b;packets.push_back({base+t*2304,sourceRoute,Buffer(t),Batch()*2304,int(t)});}
  }
 }else for(unsigned t=0;t<tiles.size();t+=Batch())packets.push_back({base+t*576,sourceRoute,Buffer(t),Batch()*576,int(t)});
 completedTiles.assign(tiles.size(),false);extraMemory.clear();inputDefined.clear();maskDefined.clear();maskTile=UINT32_MAX;
 descriptorReady=indexReady=payloadReady=false;extraReads=extraFlits=payloadWritten=expectedPayload=0;
 mailbox.fill(0);payload.fill(0);pushes=programmed=0;pushCommand=false;pushReadback.fill(false);queueAtCommit=false;
 stageCalls=waitCalls=restoreCommands=rawWrites=0;validatedTile=UINT32_MAX;assert(!stageReturn);
 for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;
 Prepare();active=true;for(unsigned i=0;i<8;i++)assert(Count(i)==physical[i]);
}
void PhasePeer::Store(unsigned a,unsigned v){
 if(preciseActive && Range(a,0x90800,1664)){assert(!pending && !(a&3));memory[a]=v;return;}
 const bool push=!preciseActive && a==0xffb20040 && command[7]==0x212;const unsigned bytes=push?command[8]:0;
 PreparedQ4Peer::Store(a,v);if(push)fabric->Incoming(*this,bytes);
}
unsigned PhasePeer::Load(unsigned a){
 if(preciseActive && Range(a,0x90000,3712))return memory.at(a);
 return PreparedQ4Peer::Load(a);
}
void PhasePeer::SubmitActivation(){
 if(!preciseActive){FfnPersistentPeer::SubmitActivation();return;}
 assert(fabric && precisePackets<4 && !pending && MathIdle() && BanksFree());ClearTensorState();
 const unsigned p=precisePackets++,write=p>=2;const auto& d=FfnPersistentPeer::descriptor;
 const unsigned local[]={0x90000,0x90400,0x90c00,0x90e00},bytes[]={1024,1024,512,128};
 const unsigned remote=d[4]+d[10+p];
 const unsigned expected[]={write?local[p]:remote,0,write?selfRoute:d[8],write?remote:local[p],0,
  write?d[8]:selfRoute,0,write?0x212U:0,bytes[p],0};
 for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
 for(unsigned i=0;i<bytes[p]/4;i++)if(write)fabric->dram[PhaseRoot::Key(d[8],remote+i*4)]=memory.at(local[p]+i*4);
 else memory[local[p]+i*4]=fabric->BankWord(d[8],remote+i*4);
 const unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes[p]/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
}
void PhasePeer::FinishPhase(){
 assert(active && fabric);
 if(preciseActive){
  while(!GdnFinished(sequence) && !GdnFaulted())StepBlock();
  assert(pc==waitPC && !pending && !GdnFaulted() && precisePackets==4 && !matrixScope && MathIdle() && BanksFree());
  active=false;fabric->preciseCompleted++;return;
 }
 while(!heldPayload && pc!=faultPC)StepBlock();assert(heldPayload && pushes==1 && !GdnFaulted());
 AckPayload();while(!GdnFinished(sequence) && !GdnFaulted())StepBlock();
 assert(pc==waitPC && pushes==2 && !pending && !heldPayload && !GdnFaulted() && requests==ExpectedReadPackets() && responses==requests);
 CheckEnd();active=false;fabric->completed++;
 if(!(fabric->completed%16) || fabric->completed==119)printf("PREPARED_COMPLETE_RUN_WORKERS role%u finished%u/119\n",fabric->role,fabric->completed);
}
int main(int argc,char** argv){
 assert(argc==6);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 PreparedRootImage image(argv[1]);PhaseSource source(argv[4],argv[5]);
 std::ifstream symbols(std::string(argv[1])+"/program.symbols");std::string at,kind,name;std::vector<unsigned> runs;
 while(symbols>>at>>kind>>name)if(name=="_ZN10ProjectionL3RunEPKj")runs.push_back(unsigned(std::stoul(at,nullptr,16)));assert(runs.size()==2);
 auto root=std::make_unique<PhaseRoot>(image,source,argv[2],argv[3]);
 for(unsigned role:{5U,2U,3U,4U}){root->SetPhase(role,runs[0]);root->RunPhase();}
 assert(root->preciseCompleted==68);
 for(auto& p:root->peers){assert(p->referenceEvaluations==4 && p->referenceReuses);p->StopLocal();}
 printf("PASS_PREPARED_COMPLETE_PHASE_HISTORY workers119 wholeRuns4 actualMatrixJobs476 actualPreciseProducers68 actualSTOP119 realSourceAndCache1;NO_COMPLETE_LAYER_NATIVE_CARD_MODEL_OR_TPS\n");
}
