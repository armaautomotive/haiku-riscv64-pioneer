/* SPDX-License-Identifier: MIT */
// Run only the exact linked q38_compute_once call under a synthetic owner.
// The real front is used to capture the original scope up to entry. A
// converted call then sees an explicitly synthetic PRIVATE mode, never
// a changed wire marker, bypassed seal, resumed publisher or real receipt.
#define PIONEER_Q38_VECTOR_METADATA_WORKER_MACHINE_ONLY
#include "pioneer_q38_vector_q4_worker_test.cpp"
#include "../PioneerQ38NativeQ8Data.h"
#ifdef PIONEER_Q38_OFFBOARD_NATIVE_Q8_WIRE
#include "../PioneerQ38TwinNativeQ8Wire.h"
#endif
#ifdef PIONEER_Q38_OFFBOARD_REAL_Q8
#include "../PioneerModelWeightCache.h"
#include "../PioneerQwen38TiledReader.h"
#ifdef PIONEER_Q38_OFFBOARD_NATIVE_Q8_WIRE
#include "../PioneerQ38TwinQ8ProjectionFixture.h"
#endif
#include <fcntl.h>
#endif

namespace {
constexpr unsigned FormatWord=0x74038, UnitReturn=0x6004;
constexpr unsigned Raw=1, Sm7=2;
using Codec=PioneerQ38NativeQ8Data;
using Tiles=std::vector<std::vector<uint8_t>>;
#ifdef PIONEER_Q38_OFFBOARD_REAL_Q8
struct RealSource {
 std::unique_ptr<FILE,decltype(&fclose)> file{nullptr,&fclose};
 PioneerQwen38Gguf model;
 PioneerQwen38TiledReader reader;
 struct stat initial={};
 static constexpr const char* Digest="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747";
 explicit RealSource(const char* path){
  assert(path && path[0]=='/');
  const int fd=open(path,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);
  assert(fd>=0 && !fstat(fd,&initial) && S_ISREG(initial.st_mode));
  file.reset(fdopen(fd,"rb"));assert(file && model.Open(file.get()));
  PioneerModelWeightCache::Digest digest={};std::string error;
  assert(PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
   && PioneerModelWeightCache::Hex(digest)==Digest && Stable() && reader.Bind(model));
 }
 bool Stable()const{
  struct stat now={};
  if(!model.Stable() || fstat(fileno(file.get()),&now) || now.st_dev!=initial.st_dev || now.st_ino!=initial.st_ino
   || now.st_mtime!=initial.st_mtime || now.st_ctime!=initial.st_ctime || now.st_size!=initial.st_size)return false;
#if defined(__APPLE__)
  return now.st_mtimespec.tv_nsec==initial.st_mtimespec.tv_nsec && now.st_ctimespec.tv_nsec==initial.st_ctimespec.tv_nsec;
#else
  return now.st_mtim.tv_nsec==initial.st_mtim.tv_nsec && now.st_ctim.tv_nsec==initial.st_ctim.tv_nsec;
#endif
 }
 Tiles ReadTileRows(unsigned first,unsigned groups){
  const auto* tensor=model.Find("blk.0.attn_qkv.weight");
  assert(tensor && tensor->rank==2 && tensor->type==8 && tensor->inputs==5120 && tensor->outputs==10240
   && !(first%16) && groups && first+16*groups<=tensor->outputs);
  const unsigned index=unsigned(tensor-model.Tensors().data()),total=groups*160*Codec::TileBytes;
  std::vector<uint8_t> packed(total);
  for(unsigned at=0;at<total;at+=Codec::PageBytes){
   std::array<uint8_t,Codec::PageBytes> page={};
   assert(Stable() && reader.ReadPage(model,index,first,groups*16,at/Codec::PageBytes,page.data(),page.size()));
   std::copy_n(page.begin(),std::min(unsigned(Codec::PageBytes),total-at),packed.begin()+at);
  }
  assert(Stable());Tiles tiles;
  for(unsigned at=0;at<total;at+=Codec::TileBytes)
   tiles.emplace_back(packed.begin()+at,packed.begin()+at+Codec::TileBytes);
  return tiles;
 }
};
#endif
struct MathUnit:VectorMetadataPeer {
 const bool converted;
 const unsigned entry, groupsIn;
 Tiles original;
 std::array<uint32_t,64> outer={};
 bool inUnit=false,unpackPending=false;
 unsigned unpackAddress=0,panels=0,retiredPanels=0,setup=0,restores=0,bufferMask=0;
 unsigned fmt73Writes=0,beginSteps=0;
 bool dropUnpack=false;
#ifdef PIONEER_Q38_OFFBOARD_NATIVE_Q8_WIRE
 bool connected=false,returned=false;
 unsigned actualReturn=0;
#endif
 struct BadOperand {};
 MathUnit(const char* legacy,const char* image,unsigned method,bool sm,unsigned k,unsigned groups,unsigned trial,
  const Tiles* learned=nullptr,const Tiles* prepared=nullptr)
  :VectorMetadataPeer(legacy,image,8,k,groups),converted(sm),entry(method),groupsIn(groups){
  assert(!steps && type==8);
  if(learned){assert(learned->size()==tiles.size());tiles=*learned;}
  else for(unsigned t=0;t<tiles.size();t++)for(unsigned i=Codec::Header;i<Codec::QuantEnd;i++)
   tiles[t][i]=uint8_t(int((37*i+13*t+trial*53+(i%16)*59)%255)-127);
  tile=tiles[0];
  if(trial)FreshInputs(*this);
  if(converted){
   original=tiles;
   if(prepared){assert(learned && prepared->size()==tiles.size());tiles=*prepared;}
   else for(auto& bytes:tiles){
    std::array<uint8_t,Codec::PageBytes> source={},wire={};
    std::copy(bytes.begin(),bytes.end(),source.begin());
    assert(Codec::PreparePage(source.data(),0,Codec::TileBytes,wire.data(),wire.size()));
    std::copy_n(wire.begin(),Codec::TileBytes,bytes.begin());
   }
  }else assert(!prepared);
 }
 bool Active()const override{
  const unsigned mode=memory.at(FormatWord);
  return mode==Raw || (inUnit && converted && mode==Sm7);
 }
 bool NativeInt8Unpack()const override{return inUnit && converted && config[0]==0x423e;}
 bool DirectNativeQ8Sfpu()const override{return NativeInt8Unpack();}
 bool DirectQ4Panel()const override{return NativeInt8Unpack();} // this observer name just means zero MOVs
#ifdef PIONEER_Q38_OFFBOARD_NATIVE_Q8_WIRE
#ifdef PIONEER_Q38_OFFBOARD_REAL_Q8
 void FixtureActivation(const PioneerQ38TwinQ8ProjectionFixture::Activation& a){
  assert(!steps && a.words.size()*2==inputs && a.scales.size()==scales.size());
  for(unsigned w=0;w<a.words.size();w++){
   vector[w*2]=uint16_t(a.words[w]);vector[w*2+1]=uint16_t(a.words[w]>>16);
   root.at(0xa0000+w*4)=a.words[w];
  }
  for(unsigned b=0;b<a.scales.size();b++)scales[b]=root.at(0xab000+b*4)=a.scales[b];
 }
#endif
 void StepBlock()override{
  if(connected && pc==entry){
   assert(!returned && !inUnit && !actualReturn && memory.at(FormatWord)==Sm7);
   actualReturn=regs[1];inUnit=true;
  }
  VectorMetadataPeer::StepBlock();
  if(connected && inUnit && pc==actualReturn){
   assert(!returned && !unpackPending && !pending && config[0]==0x4231 && config[1]==0xa03c8000 && config[73]==0x13);
   assert(regs[10]==groupsIn*inputs/16 && panels==groupsIn*inputs/16 && panels==retiredPanels && setup==1 && restores==1
    && bufferMask==3 && fmt73Writes==2*groupsIn*Blocks()+1);
   tiles.swap(original); // ONLY the independent host oracle uses original bytes from now on
   returned=true;inUnit=false;
  }
 }
 void StageConnected(unsigned mode,bool delayed){
  namespace W=PioneerQ38TwinNativeQ8Wire;
  assert(converted && !steps);connected=true;delay=3;
  boot[W::BootIndex]=W::BootTag;memory[M::Boot+4*W::BootIndex]=W::BootTag;
  BootPushed();Prepare();asynchronous=delayed;
  const unsigned header[]={Push::Magic,1,1,logicalWorker,selfRoute,0,0,2025};
  for(unsigned i=0;i<8;i++){outer[i]=header[i];outer[8+i]=boot[8+i];}
  for(unsigned i=0;i<10;i++)outer[16+i]=matrixJob[i];
  outer[26]=mode;
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=outer[i];
  memory[M::Doorbell]=1;sequence=1;
 }
 void FinishConnected(){
  namespace W=PioneerQ38TwinNativeQ8Wire;
  AwaitPayload();assert(returned && !inUnit && !mailbox[0]);
  assert(memory.at(0x83f00+4*W::ProofIndex)==W::ProofTag);
  for(unsigned i=W::ProofIndex+1;i<64;i++)assert(!memory.at(0x83f00+i*4));
  for(unsigned i=0;i<64;i++)assert(memory.at(M::Job+i*4)==outer[i]);
  AckPayload();while(!GdnFinished(1) && pc!=faultPC)StepBlock();
  assert(pc==waitPC && !memory.at(M::Status+8) && pushes==2 && requests==ExpectedReadPackets());
  const std::array<uint32_t,8> header{{Push::ReceiptMagic,64,logicalWorker,1,0,0,selfRoute,Push::Root}};
  const std::array<uint32_t,4> slave{{2,5,2,2}};
  assert(rootMaster==rootOriginal && rootSlave==slave && minStack>=0x6e000 && !heldPayload);
  for(unsigned i=0;i<8;i++)assert(mailbox[i]==header[i] && mailbox[56+i]==beforePush[i]);
  for(unsigned i=0;i<32;i++)assert(mailbox[8+i]==outer[i]);
  for(unsigned i=0;i<16;i++)assert(mailbox[40+i]==boot[16+i]);
  CheckEnd();if(asynchronous)assert(retiredDuringMath>0);
 }
 void StopConnected(){
  assert(pc==waitPC && sequence==1 && !pending);
  std::array<uint32_t,64> stop={};
  const unsigned header[]={Push::Magic,2,unsigned(Push::Stop),logicalWorker,selfRoute,0,0,9999};
  for(unsigned i=0;i<8;i++){stop[i]=header[i];stop[8+i]=boot[8+i];}
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=stop[i];
  memory[M::Doorbell]=2;sequence=2;
  while(pc!=stopPC && pc!=faultPC)StepBlock();
  assert(pc==stopPC && memory.at(M::Status+4)==2 && memory.at(M::Status+16)==2 && !pending);
  auto snap=memory;unsigned reads=requests;
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(pc==stopPC && memory==snap && requests==reads && pushes==2);
 }
#endif
 void Store(uint32_t at,uint32_t value)override{
  if(inUnit && converted && at==0xffef0000 && setupConfiguration==25 && config[0] && value!=config[0]
   && (value==0x423e || value==0x4231)){
   // Same model of a private drained configuration transition as the old
   // independently tested single-buffer SM7 path.
   assert(drained && !ownerA && !ownerB && !rawPending && !unpackPending && phaseQueue.empty() && pendingAccumulator.empty());
   if(value==0x423e){assert(config[0]==0x4231 && !setup);setup++;}
   else{assert(config[0]==0x423e && setup==1 && !restores);restores++;}
   config[0]=value;configWrites++;return;
  }
  if(inUnit && converted && at==0xffef0000+73*4 && config[0]==0x423e){
   assert(value==3 || value==0x13);fmt73Writes++;
  }
  assert(!unpackPending || at==0xffe000fc || at==0xffe40000 || Range(at,0x6e000,8192));
  VectorMetadataPeer::Store(at,value);
 }
 void Instruction(uint32_t op)override{
  if(NativeInt8Unpack() && op==0x420000c1 && config[73]==3){
   assert(!unpackPending && !rawPending && phaseQueue.empty() && pendingAccumulator.empty()
    && !sources[unpackBank] && !ownerA && thread[5]==4 && adc0==0 && adc1==255
    && config[1]==0xa03c0000 && !config[49] && config[64]==0x10001e && config[65]==16
    && config[72]==14 && config[84]==0x400040);
   if(pending){assert(DisjointPrefetchCompute());overlapOps++;}
   const unsigned raw=RawInputBase(),tile=CurrentTile(),panel=matrixInstructions/MatrixPhasesPerPanel()%2;
   assert(panels==tile*2+panel);
   unpackAddress=(config[76]+1)*16;
   assert(unpackAddress==raw+32+256*panel && unpackAddress+256<=raw+Codec::QuantEnd);
   bufferMask|=raw>=0x9a000?2:1;
   unpackPending=true;panels++;drained=false;return;
  }
  if(unpackPending){
   assert(op==0xa2101816 || op==0x4500017e);
   if(op==0x4500017e){
    assert(backendWait && NativeInt8Unpack() && (!pending || DisjointPrefetchCompute()));
    if(dropUnpack)throw BadOperand{};
    for(unsigned i=0;i<256;i++){
     const unsigned at=unpackAddress+i;
     const unsigned byte=(memory.at(at&~3U)>>(8*(at&3)))&255;
     if(byte==128)throw BadOperand{}; // noncanonical encoded zero cannot come from the producer
     a[i/16][i%16]=float(int(byte&127)*((byte&128)?-1:1));
    }
    sources[unpackBank]=true;unpackBank^=1;ownerA=sources[mathBank];
    unpackPending=false;panelMoves=0;retiredPanels++;
   }
  }
  VectorMetadataPeer::Instruction(op);
 }
 void Begin(bool delayed){
  delay=3;BootPushed();Prepare();StagePushed();
  while(pc!=entry && pc!=faultPC)StepBlock();
  assert(pc==entry && !matrixInstructions && !requests && !pushes && !pending && !unpackPending);
  for(unsigned i=0;i<outer.size();i++)outer[i]=memory.at(M::Job+i*4);
  assert(outer[26]==Raw && memory.at(FormatWord)==Raw);
  // Host-only standalone call, keeping the observed scope/boot/NoC state.
  // Multiplication and all real checks still execute; no parent or physical
  // receipt can be observed for this purpose-built input.
  if(converted)memory[FormatWord]=Sm7;
  // Timed replies, including the first batch of EVERY group, progress
  // independently. Require actual math/read overlap separately at return.
  regs[1]=UnitReturn;inUnit=true;asynchronous=delayed;beginSteps=steps;
 }
 bool Finish(){
  while(pc!=UnitReturn && pc!=faultPC)StepBlock();
  assert(!pushes && !mailbox[0] && !memory.at(M::Status+4));
  for(unsigned i=0;i<outer.size();i++)assert(memory.at(M::Job+i*4)==outer[i]);
  if(pc==faultPC)return false;
  assert(regs[10]==groupsIn*inputs/16 && requests==ExpectedReadPackets() && responses==requests);
  assert(!unpackPending && !pending && config[0]==0x4231 && config[1]==0xa03c8000 && config[73]==0x13);
  if(converted){
   assert(panels==groupsIn*inputs/16 && panels==retiredPanels && restoreCommands==groupsIn
    && setup==1 && restores==1 && bufferMask==3 && fmt73Writes==2*groupsIn*Blocks()+1);
   tiles.swap(original); // original bytes ONLY for the offboard numerical oracle
  }else assert(!panels && !setup && !restores);
  CheckEnd();
  if(asynchronous)assert(retiredDuringMath>0);
  return true;
 }
};
}

int main(int argc,char** argv){
 const bool pilot=argc==7 && !strcmp(argv[6],"--pilot");
#ifdef PIONEER_Q38_OFFBOARD_NATIVE_Q8_WIRE
 const bool connected=argc==7 && (!strcmp(argv[6],"--wire")
#ifdef PIONEER_Q38_OFFBOARD_REAL_Q8
  || argv[6][0]=='/'
#endif
 );
#else
 const bool connected=false;
#endif
#ifdef PIONEER_Q38_OFFBOARD_REAL_Q8
 const bool learned=argc==7 && argv[6][0]=='/';
#else
 const bool learned=false;
#endif
 assert(argc==6 || pilot || learned || connected);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 unsigned count=0,words=0;
 const unsigned oldEntry=unsigned(std::stoul(argv[4],nullptr,16)),candidate=unsigned(std::stoul(argv[5],nullptr,16));
#ifdef PIONEER_Q38_OFFBOARD_NATIVE_Q8_WIRE
 if(connected){
#ifdef PIONEER_Q38_OFFBOARD_REAL_Q8
  if(learned){
   namespace F=PioneerQ38TwinQ8ProjectionFixture;
   RealSource source(argv[6]);
   std::unique_ptr<const F::Fixture> fixture;
   unsigned calls=0;
   auto interrupted=[&](){return ++calls<80 && source.Stable();};
   assert(!F::Build(source.model,interrupted,fixture) && !fixture && calls==80);
   const auto ready=[&](){return source.Stable();};
   assert(F::Build(source.model,ready,fixture) && fixture);
   assert(!F::Build(source.model,ready,fixture));
   // Independent original-row reader, not the fixture's tiled producer
   // called a second time. Compare ALL 34 bytes of all 7,680 blocks.
   const auto* tensor=source.model.Find(F::Tensor);assert(tensor);
   const unsigned index=unsigned(tensor-source.model.Tensors().data());
   for(unsigned row=0;row<F::Rows;row++){
    std::array<uint8_t,F::Blocks*34> packed={};
    assert(source.model.ReadPacked(index,row*packed.size(),packed.size(),packed.data(),packed.size()));
    for(unsigned b=0;b<F::Blocks;b++)for(unsigned i=0;i<34;i++){
     const unsigned tile=(row/16)*F::Blocks+b;
     assert(fixture->raw[tile*Codec::TileBytes+i*16+row%16]==packed[b*34+i]);
    }
   }
   Tiles rawTiles,nativeTiles;
   unsigned changed=0;
   for(unsigned at=0;at<F::Bytes;at+=Codec::TileBytes){
    rawTiles.emplace_back(fixture->raw.begin()+at,fixture->raw.begin()+at+Codec::TileBytes);
    nativeTiles.emplace_back(fixture->native.begin()+at,fixture->native.begin()+at+Codec::TileBytes);
    for(unsigned i=0;i<Codec::TileBytes;i++){
     const uint8_t raw=fixture->raw[at+i],wire=fixture->native[at+i];
     if(i<Codec::Header)assert(wire==raw);
     else if(i>=Codec::QuantEnd)assert(!wire && !raw);
     else{uint8_t decoded=0;assert(Codec::Decode(wire,decoded) && decoded==raw);changed+=wire!=raw;}
    }
   }
   for(unsigned at=F::Bytes;at<F::Storage;at++)assert(!fixture->raw[at] && !fixture->native[at]);
   assert(changed);
   F::Activation testInput={};
   testInput.words.fill(0xdeadc0de);testInput.scales.fill(0xfeedbeef);const auto before=testInput;
   assert(!F::MakeActivation(2,testInput) && testInput.words==before.words && testInput.scales==before.scales);
   std::array<uint32_t,F::Rows> previous={};
   constexpr const char* expected[]={
    "0e94ceba949d9266e0383ca43b96c73da4bcaf0785fec9d45b4570f73e021254",
    "42f5a264959edb0b327fc337a227ae2f4bcb55bc74a637cbde09087b057ebfd2"};
   for(unsigned variant=0;variant<2;variant++){
    auto raw=std::make_unique<MathUnit>(argv[1],argv[2],oldEntry,false,F::Inputs,F::Groups,0,&rawTiles);
    auto wire=std::make_unique<MathUnit>(argv[1],argv[3],candidate,true,F::Inputs,F::Groups,0,&rawTiles,&nativeTiles);
    assert(F::MakeActivation(variant,testInput));raw->FixtureActivation(testInput);wire->FixtureActivation(testInput);
    raw->delay=3;raw->BootPushed();raw->Prepare();raw->StagePushed();raw->asynchronous=true;
    raw->AwaitPayload();raw->FinishPushed(true);raw->CheckEnd();raw->StopLocal();
    wire->StageConnected(Sm7,true);wire->FinishConnected();wire->StopConnected();
    assert(raw->pushes==2 && wire->pushes==2 && wire->requests==raw->requests
     && wire->matrixInstructions==raw->matrixInstructions);
    std::array<uint8_t,F::Rows*4> canonical={};
    for(unsigned i=0;i<F::Rows;i++){
     const uint32_t word=raw->buffers[4096+i];
     assert(wire->buffers[4096+i]==word && previous[i]!=word);previous[i]=word;
     for(unsigned b=0;b<4;b++)canonical[i*4+b]=uint8_t(word>>(b*8));
    }
    using Cache=PioneerModelWeightCache;
    const auto resultHash=Cache::Hex(Cache::Hash(canonical.data(),canonical.size()));
    assert(resultHash==expected[variant]);
    printf("NATIVE_Q8_WIRE_REAL K%u rows%u variant%u matching_FP32=%u NoCrequests=%u matrix=%u output=%s\n",
     F::Inputs,F::Rows,variant,F::Rows,raw->requests,raw->matrixInstructions,resultHash.c_str());
   }
   assert(source.Stable());
   using Cache=PioneerModelWeightCache;
   const auto rawHash=Cache::Hex(Cache::Hash(fixture->raw.data(),F::Storage));
   const auto nativeHash=Cache::Hex(Cache::Hash(fixture->native.data(),F::Storage));
   assert(rawHash=="a6a28c1abe3d8475b90c2fb6c79b796a19e3f5ecce0f96066cdd41a59f0aa005"
    && nativeHash=="a3fa3db0700afe50fc55e60b0b525a84a96343a14a2a11fca8713bf34197a68c");
   printf("NATIVE_Q8_WIRE_REAL source=%s raw=%s native=%s\n",RealSource::Digest,rawHash.c_str(),nativeHash.c_str());
   printf("PASS linked raw and native Q8 real fixture: %u pages per arm, %u changed sign bytes, 96 exact outputs, both STOP; modeled hardware\n",
    F::Pages,changed);
   return 0;
  }
#endif
  for(unsigned k:{5120U,6144U,17408U}){
   unsigned groups=k==5120?1U:3U;
   auto raw=std::make_unique<MathUnit>(argv[1],argv[2],oldEntry,false,k,groups,1);
   auto wire=std::make_unique<MathUnit>(argv[1],argv[3],candidate,true,k,groups,1);
   raw->Begin(true);assert(raw->Finish());
   wire->StageConnected(Sm7,true);wire->FinishConnected();wire->StopConnected();
   assert(wire->requests==raw->requests && wire->matrixInstructions==raw->matrixInstructions);
   for(unsigned i=0;i<groups*16;i++)assert(wire->buffers[4096+i]==raw->buffers[4096+i]);
   printf("NATIVE_Q8_WIRE complete worker K%u groups%u exact%u with real push+STOP; modeled NoC\n",k,groups,groups*16);
  }
  for(unsigned mode:{0U,1U,3U}){
   auto bad=std::make_unique<MathUnit>(argv[1],argv[3],candidate,true,5120,1,0);
   bad->StageConnected(mode,false);
   while(bad->pc!=bad->faultPC)bad->StepBlock();
   assert(!bad->requests && !bad->inUnit && !bad->matrixInstructions && !bad->pushes && !bad->mailbox[0]);
  }
  auto oldBoot=std::make_unique<MathUnit>(argv[1],argv[3],candidate,true,5120,1,0);
  while(oldBoot->pc!=oldBoot->waitPC && oldBoot->pc!=oldBoot->faultPC)oldBoot->StepBlock();
  assert(oldBoot->pc==oldBoot->faultPC && !oldBoot->requests && !oldBoot->pushes && !oldBoot->matrixInstructions);
  puts("PASS native Q8 wire worker: 3 actual complete linked projections 112 exact words, native push and STOP; 4 inert mismatches; synthetic driver and NoC");
  return 0;
 }
#endif
 auto match=[&](unsigned k,unsigned groups,unsigned variant,const Tiles* weights){
  auto old=std::make_unique<MathUnit>(argv[1],argv[2],oldEntry,false,k,groups,variant,weights);
  auto next=std::make_unique<MathUnit>(argv[1],argv[3],candidate,true,k,groups,variant,weights);
  old->Begin(variant);next->Begin(variant);
  assert(old->Finish() && next->Finish());
  assert(old->matrixInstructions==next->matrixInstructions && old->requests==next->requests && old->responses==next->responses);
  assert(old->Count(1)==next->Count(1) && old->nativeStoreOps && !next->nativeStoreOps && next->vectorOps<old->vectorOps);
  assert(old->directReads==next->directReads && old->baseWrites==next->baseWrites);
  for(unsigned row=0;row<groups*16;row++)assert(old->buffers[4096+row]==next->buffers[4096+row]);
  words+=groups*16;count++;
  printf("TWIN_NATIVE_Q8_FULL_K K%u groups%u fixture%u exact%u matrix%u reads%u quants%u "
   "RV%u/%u vectorOps%u/%u drains%u/%u weights=%s; SYNTHETIC NoC/ACK\n",
   k,groups,variant,groups*16,next->matrixInstructions,next->requests,next->retiredPanels*256,
   old->steps-old->beginSteps,next->steps-next->beginSteps,old->vectorOps,next->vectorOps,old->drains,next->drains,
   weights?"original-GGUF":"synthetic");
 };
 for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U})for(unsigned variant:{0U,1U})
  if(!pilot || (k==5120 && groups==1 && !variant))match(k,groups,variant,nullptr);
 if(!pilot)match(17408,16,1,nullptr);
#ifdef PIONEER_Q38_OFFBOARD_REAL_Q8
 if(learned){
  RealSource source(argv[6]);
  const auto low=source.ReadTileRows(0,1),high=source.ReadTileRows(10240-48,3);
  match(5120,1,0,&low);match(5120,3,1,&high);
  assert(source.Stable());
  printf("TWIN_NATIVE_Q8_REAL source_sha=%s tensor=blk.0.attn_qkv.weight rows=0..15,10192..10239 unchanged; synthetic activations\n",RealSource::Digest);
 }
#endif
 unsigned faults=0;
 if(!pilot){
  for(unsigned which=0;which<4;which++){
   auto bad=std::make_unique<MathUnit>(argv[1],argv[3],candidate,true,5120,1,0);
   bad->Begin(false);
   if(which<2){
    if(!which)bad->dropUnpack=true;else bad->tiles[0][Codec::Header]=0x80;
    bool caught=false;try{(void)bad->Finish();}catch(const MathUnit::BadOperand&){caught=true;}
    assert(caught && !bad->matrixInstructions && !bad->outputReads && !bad->pushes);
   }else{
    bad->faultAt=which==2?bad->inputReads+1:bad->inputReads+2;
    bad->ReadFault();assert(!bad->outputReads && !bad->pushes);
   }
   faults++;
  }
 }
 assert(count==(pilot?1U:learned?15U:13U) && words==(pilot?16U:learned?704U:640U));
 printf("%s twin native Q8 full isolated math: %u matched projections / %u exact FP32 words / %u fault cases; NO connected owner, receipt, silicon time or TPS\n",
  pilot?"PILOT":"PASS",count,words,faults);
}
