/* SPDX-License-Identifier: MIT */
// Check exact data and intake contracts on synthetic tiles and every real
// Q6 fragment of the fixed model. No firmware, device or timing test.
#include "../PioneerQ38NativeQ6Geometry.h"
#include "../PioneerQ38NativeQ6Unpack.h"
#include "../PioneerQwen38Panel.h"
#include "../PioneerModelWeightCache.h"
#include <array>
#include <cassert>
#include <fcntl.h>
#include <memory>
#include <unistd.h>

namespace T=PioneerQ38NativeQ6Tiles;
namespace N=PioneerQ38NativeQ6Unpack;
using Raw=std::array<uint8_t,T::RawBytes>;
using Tile=std::array<uint8_t,T::TileBytes>;
using Page=std::array<uint8_t,4096>;

struct Commands {
 const Tile& tile;
 unsigned start,base=0,c0=0x4231,c1=0xa03c8000,c73=0x13,count=0,drains=0,step=0;
 bool idle=true,armed=false;
 std::array<unsigned,256> words={};
 void Drain(){idle=true;drains++;}
 void Config(unsigned reg,unsigned value){
  assert(idle);count++;
  switch(reg){case 0:c0=value;break;case 1:c1=value;break;case 73:c73=value;break;
   case 76:base=(value+1)*16;assert(armed && base>=start+320 && base<=start+4160);break;
   default:assert(false);
  }
 }
 void Op(unsigned op){
  assert(c0==0x423e && c1==0xa03c0000 && c73==3);count++;idle=false;
  if(op==0x8a000002){assert(!armed);armed=true;return;}
  constexpr unsigned program[]={0x3700000f,0x5e23fc00,0x5120000a,0x5420000f,0x420000c1};
  assert(armed && base && step<5 && op==program[step]);
  if(++step!=5)return;
  step=0;
  for(unsigned i=0;i<256;i++){
   const unsigned b=tile.at(base-start+i);
   words[i]=PioneerInt8Operand(b&128?-int(b&127):int(b));
  }
 }
};

static unsigned panels=0,refusals=0;
static bool SameFile(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size
  || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}

static void Check(const Raw& raw){
 Tile tile={};Raw restored={};
 assert(T::Encode(raw.data(),raw.size(),tile.data(),tile.size()) && T::Decode(tile.data(),tile.size(),restored.data(),restored.size()));
 assert(raw==restored && std::equal(raw.begin()+3072,raw.begin()+3360,tile.begin()));
 for(unsigned at=288;at<320;at++)assert(!tile[at]);
 for(unsigned address:{T::TwinFetch::Buffer0,T::TwinFetch::Buffer1}){
  Commands e{tile,address};N::Enter(e);
  for(unsigned p=0;p<16;p++){
   PioneerQ38Panel original={};assert(PioneerQ38UnpackPanel(14,raw.data(),raw.size(),p,&original));
   auto expected=e.count;assert(N::Panel(e,14,address,p) && e.count==expected+6 && e.idle && !e.step);
   for(unsigned i=0;i<256;i++)assert(e.words[i]==((original.operand[i/2]>>((i&1)*16))&65535));
   for(unsigned r=0;r<16;r++){
    unsigned s=tile[p*16+r],half=tile[256+r]|unsigned(tile[272+r])<<8;
    uint32_t scale=0;
    assert(PioneerQ38PanelScale(uint16_t(half),int(s)-int((s&128)*2),scale));
    assert(scale==original.coefficient[r] && !original.minimum[r]);
   }
   panels++;
  }
  auto expected=e.count;assert(!N::Panel(e,8,address,0) && !N::Panel(e,14,address+64,0) && !N::Panel(e,14,address,16));
  assert(e.count==expected && e.idle);refusals+=3;
  N::Restore(e);assert(e.c0==0x4231 && e.c1==0xa03c8000 && e.c73==0x13 && e.idle);
 }
}

static void Geometry(){
 using F=T::TwinFetch;
 for(unsigned inputs:{5120U,6144U})for(unsigned groups=1;groups<=16;groups++){
  const unsigned blocks=inputs/256,base=F::UploadEnd-T::TileBytes*groups*blocks;
  for(unsigned g=0;g<groups;g++)for(unsigned b=0;b<blocks;b++){
   F f={};assert(F::Describe(inputs,groups,g,b,base,f));
   assert(f.source==base+(g*blocks+b)*T::TileBytes && f.first==(b==0) && f.hasNext==(b+1<blocks));
   assert(f.raw==(b&1?unsigned(F::Buffer1):unsigned(F::Buffer0)));
   assert(f.nextSource==(f.hasNext?f.source+T::TileBytes:0U) && (!f.hasNext || f.nextRaw!=f.raw));
   for(unsigned p=0;p<16;p++){
    unsigned at=0;assert(F::PanelAddress(f.raw,p,at) && at==f.raw+320+p*256 && at+256<=f.raw+T::TileBytes);
   }
  }
  F refused{};refused.raw=123;
  assert(!F::Describe(inputs,groups,0,0,base+64,refused) && refused.raw==123);
 }
 unsigned slots=0,at=0;
 for(unsigned address=F::Buffer0-1;address<=F::Limit;address++)if(F::PanelAddress(address,0,at))slots++;
 assert(slots==2);
}

static void Actual(const char* name){
 int fd=open(name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
 assert(fd>=0);
 std::unique_ptr<FILE,decltype(&fclose)> file(fdopen(fd,"rb"),&fclose);assert(file);
 PioneerModelWeightCache::Digest digest={};std::string error;
 struct stat before={},after={};assert(!fstat(fd,&before) && S_ISREG(before.st_mode));
 assert(PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error));
 assert(PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 PioneerQwen38Gguf model;PioneerQwen38PackedPlan original;
 assert(model.Open(file.get()) && original.Build(model,8192,true,true));
 std::vector<PioneerQ38PackedPart> expanded;assert(PioneerQ38NativeQ6Geometry::Build(model,original,expanded));
 std::set<unsigned> tensors;unsigned cases=0;uint64_t used[7]={},increase=0;
 const auto& parts=original.Parts();
 for(unsigned f=0;f<expanded.size();f++){
  auto a=parts[f],b=expanded[f];
  assert(a.kind==b.kind && a.layer==b.layer && a.role==b.role && a.bank==b.bank && a.firstRow==b.firstRow && a.rows==b.rows
   && b.proposedAddress==original.Base+used[b.bank]);
  used[b.bank]+=b.allocated;
  if(a.bytes==b.bytes){assert(a.allocated==b.allocated && a.rowBytes==b.rowBytes);continue;}
  assert(a.kind==PIONEER_Q38_WEIGHT && model.Tensors().at(a.tensor).type==14 && a.bytes%T::RawBytes==0);
  assert(b.bytes/T::TileBytes==a.bytes/T::RawBytes && b.allocated>=a.allocated);
  increase+=b.allocated-a.allocated;tensors.insert(a.tensor);
  const unsigned count=a.bytes/T::RawBytes;
  for(unsigned index:{0U,count/2,count-1}){
   Raw tile={};unsigned begin=index*T::RawBytes;
   for(unsigned at=0;at<T::RawBytes;){
    Page page={};const unsigned absolute=begin+at,within=absolute%4096;
    assert(original.ReadWeightPage(model,f,absolute/4096,page.data(),page.size()));
    const unsigned take=std::min(4096-within,T::RawBytes-at);std::copy_n(page.begin()+within,take,tile.begin()+at);at+=take;
   }
   Check(tile);cases++;
  }
 }
 assert(cases==357 && tensors.size()==17 && model.Stable() && !fstat(fd,&after) && SameFile(before,after));
 uint64_t total=0;
 for(unsigned bank=0;bank<7;bank++){
  assert(used[bank]<=original.Capacity);total+=used[bank];
  printf("bank%u original=%llu proposed=%llu remaining=%llu\n",bank,(unsigned long long)original.Used(bank),
         (unsigned long long)used[bank],(unsigned long long)(original.Capacity-used[bank]));
 }
 printf("Actual Q6 model: %u checked tiles, %zu tensors, 119 fragments; growth=%llu B, total=%llu B with states unchanged in size\n",
        cases,tensors.size(),(unsigned long long)increase,(unsigned long long)total);
}

int main(int argc,char** argv){
 assert(argc==2 && argv[1][0]=='/');setvbuf(stdout,nullptr,_IOLBF,0);Geometry();
 Raw raw={};uint32_t random=0xc130af92;
 for(unsigned trial=0;trial<64;trial++){
  for(unsigned i=0;i<3344;i++){
   random^=random<<13;random^=random>>17;random^=random<<5;raw[i]=uint8_t(random);
  }
  for(unsigned i=3344;i<3360;i++)raw[i]=uint8_t(0x7b&(random>>(i%24)));
  Check(raw);
 }
 Tile wire;wire.fill(0xac);const auto marker=wire;
 raw.back()=1;assert(!T::Encode(raw.data(),raw.size(),wire.data(),wire.size()) && wire==marker);raw.back()=0;
 raw[3344]=0x7c;assert(!T::Encode(raw.data(),raw.size(),wire.data(),wire.size()) && wire==marker);raw[3344]=0;
 assert(!T::Encode(raw.data(),raw.size()-1,wire.data(),wire.size()) && wire==marker);
 Tile aliased={};std::copy(raw.begin(),raw.end(),aliased.begin());const auto aliasedBefore=aliased;
 assert(!T::Encode(aliased.data(),raw.size(),aliased.data(),aliased.size()) && aliased==aliasedBefore);
 assert(T::Encode(raw.data(),raw.size(),wire.data(),wire.size()));
 const auto wireBefore=wire;
 assert(!T::Decode(wire.data(),wire.size(),wire.data()+64,raw.size()) && wire==wireBefore);
 const Raw before=raw;
 for(unsigned b:{32U,127U,128U,161U,255U}){
  wire.back()=uint8_t(b);assert(!T::Decode(wire.data(),wire.size(),raw.data(),raw.size()) && raw==before);
 }
 Actual(argv[1]);assert(panels==13472 && refusals==2526);
 printf("PASS native Q6 exact data and modeled unpack: %u panels, %u inert command refusals, original scales; no device\n",panels,refusals);
}
