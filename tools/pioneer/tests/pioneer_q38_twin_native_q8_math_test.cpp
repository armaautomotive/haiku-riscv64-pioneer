/* SPDX-License-Identifier: MIT */
// Run only the exact linked q38_compute_once call under a synthetic owner.
// The real front is used to capture the original scope up to entry. A
// converted call then sees an explicitly synthetic PRIVATE mode, never
// a changed wire marker, bypassed seal, resumed publisher or real receipt.
#define PIONEER_Q38_VECTOR_METADATA_WORKER_MACHINE_ONLY
#include "pioneer_q38_vector_q4_worker_test.cpp"
#include "../PioneerQ38NativeQ8Data.h"
#ifdef PIONEER_Q38_OFFBOARD_REAL_Q8
#include "../PioneerModelWeightCache.h"
#include "../PioneerQwen38TiledReader.h"
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
 struct BadOperand {};
 MathUnit(const char* legacy,const char* image,unsigned method,bool sm,unsigned k,unsigned groups,unsigned trial,
  const Tiles* learned=nullptr)
  :VectorMetadataPeer(legacy,image,8,k,groups),converted(sm),entry(method),groupsIn(groups){
  assert(!steps && type==8);
  if(learned){assert(learned->size()==tiles.size());tiles=*learned;}
  else for(unsigned t=0;t<tiles.size();t++)for(unsigned i=Codec::Header;i<Codec::QuantEnd;i++)
   tiles[t][i]=uint8_t(int((37*i+13*t+trial*53+(i%16)*59)%255)-127);
  tile=tiles[0];
  if(trial)FreshInputs(*this);
  if(converted){
   original=tiles;
   for(auto& bytes:tiles){
    std::array<uint8_t,Codec::PageBytes> source={},wire={};
    std::copy(bytes.begin(),bytes.end(),source.begin());
    assert(Codec::PreparePage(source.data(),0,Codec::TileBytes,wire.data(),wire.size()));
    std::copy_n(wire.begin(),Codec::TileBytes,bytes.begin());
   }
  }
 }
 bool Active()const override{
  const unsigned mode=memory.at(FormatWord);
  return mode==Raw || (inUnit && converted && mode==Sm7);
 }
 bool NativeInt8Unpack()const override{return inUnit && converted && config[0]==0x423e;}
 bool DirectNativeQ8Sfpu()const override{return NativeInt8Unpack();}
 bool DirectQ4Panel()const override{return NativeInt8Unpack();} // this observer name just means zero MOVs
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
#ifdef PIONEER_Q38_OFFBOARD_REAL_Q8
 const bool learned=argc==7 && argv[6][0]=='/';
#else
 const bool learned=false;
#endif
 assert(argc==6 || pilot || learned);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 unsigned count=0,words=0;
 const unsigned oldEntry=unsigned(std::stoul(argv[4],nullptr,16)),candidate=unsigned(std::stoul(argv[5],nullptr,16));
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
