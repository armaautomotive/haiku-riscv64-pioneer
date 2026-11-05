/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_Q4_WEIGHT_CACHE_H
#define PIONEER_Q38_NATIVE_Q4_WEIGHT_CACHE_H
#include "PioneerModelWeightCache.h"
#include "PioneerQwen38PackedPlan.h"
#include "PioneerQ38Q4EscapeTiles.h"

// Separate PREPARATION format,NOT the current Work loader/device ABI.
// Losslessly replace Q4 primary bytes and append sparse correction data in
// the SAME seven-bank proposal. All original metadata/other allocations
// remain unchanged. No learned FP products,CPU inference or device IO.
// A fixed-size cached manifest makes a verified warm hit do ZERO encoding.
// Each atomic bundle contains BOTH primary and its correction index/payload.
class PioneerQ38NativeQ4WeightCache {
 using Cache=PioneerModelWeightCache;
 using Codec=PioneerQ38Q4EscapeTiles;
 struct Entry {
  bool q4=false,prepared=false;
  unsigned tiles=0,indexBytes=0,sideBytes=0,address=0,allocated=0;
  Cache::Key manifestKey,bundleKey;
 };
 Cache cache;
 PioneerQwen38Gguf* model=nullptr;
 PioneerQwen38PackedPlan plan;
 Cache::Ready sourceReady;
 uint64_t epoch=0,used[7]={};
 std::vector<Entry> entries;
 unsigned next=0,expected=0,prepared=0,active=UINT32_MAX;
 std::unique_ptr<Cache::Reader> reader;
 bool attempted=false;
 unsigned tilePart=UINT32_MAX,tileNumber=UINT32_MAX,pagePart=UINT32_MAX,pageNumber=UINT32_MAX;
 std::array<uint8_t,4096> originalPage{};
 std::array<uint8_t,2816> encoded{};
 static constexpr unsigned Magic=0x31453451,ManifestHeader=64,OffsetMask=0x3fffffff;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
 static unsigned Align16(unsigned n){return (n+15)&~15U;}
 static unsigned Word(const uint8_t* p){return unsigned(p[0])|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
 static void Put(uint8_t* p,unsigned v){for(unsigned i=0;i<4;i++)p[i]=uint8_t(v>>(i*8));}
 bool Ready()const{
  return model && sourceReady && sourceReady() && model->Epoch()==epoch && model->Stable()
   && plan.MatchesSource(*model) && plan.Tiled();
 }
 bool Tile(unsigned fragment,unsigned tile,uint64_t& converted,std::string& error){
  if(!Ready() || fragment>=entries.size() || !entries[fragment].q4 || tile>=entries[fragment].tiles)
   return Fail(error,"Invalid native Q4 source tile.");
  if(tilePart==fragment && tileNumber==tile)return true;
  std::array<uint8_t,2304> raw{};unsigned copied=0;
  while(copied<raw.size()){
   const uint64_t at=uint64_t(tile)*2304+copied;const unsigned page=unsigned(at/4096),within=unsigned(at%4096);
   if(pagePart!=fragment || pageNumber!=page){
    pagePart=pageNumber=UINT32_MAX;
    if(!plan.ReadWeightPage(*model,fragment,page,originalPage.data(),originalPage.size()))return Fail(error,"Cannot read original Q4 tiled page.");
    pagePart=fragment;pageNumber=page;
   }
   const unsigned take=std::min(unsigned(raw.size())-copied,4096-within);
   memcpy(raw.data()+copied,originalPage.data()+within,take);copied+=take;
  }
  tilePart=tileNumber=UINT32_MAX;
  if(!Ready() || !Codec::Encode(raw.data(),raw.size(),encoded.data(),encoded.size()))return Fail(error,"Cannot losslessly encode original Q4 tile.");
  tilePart=fragment;tileNumber=tile;converted++;return true;
 }
 bool Manifest(unsigned fragment,std::vector<uint8_t>& bytes,uint64_t& converted,std::string& error){
  const auto& e=entries[fragment];const auto& p=plan.Parts()[fragment];
  bytes.assign(ManifestHeader+e.indexBytes,0);unsigned cursor=e.indexBytes;
  for(unsigned tile=0;tile<e.tiles;tile++){
   if(!Tile(fragment,tile,converted,error))return false;
   const unsigned count=Codec::NonzeroRows(encoded.data()+2304),n=Codec::MaskPayloadBytes(count);
   const unsigned kind=!count?Codec::Zero:n==512?Codec::Dense:Codec::SparseRows;
   if(cursor>OffsetMask-Align16(n))return Fail(error,"Native Q4 correction offset exceeds format.");
   Put(bytes.data()+ManifestHeader+tile*4,(kind<<30)|(count?cursor:0));cursor+=Align16(n);
  }
  const unsigned header[]={Magic,1,p.tensor,p.firstRow,p.rows,unsigned(model->Tensors()[p.tensor].inputs),
   2304,e.tiles,e.indexBytes,cursor,p.bytes,0,0,0,0,0};
  for(unsigned i=0;i<16;i++)Put(bytes.data()+i*4,header[i]);
  return Ready();
 }
 bool CheckManifest(unsigned fragment,const std::vector<uint8_t>& bytes,unsigned& side,
  std::vector<unsigned>& nonzero,std::string& error)const{
  const auto& e=entries[fragment];const auto& p=plan.Parts()[fragment];
  if(bytes.size()!=ManifestHeader+e.indexBytes)return Fail(error,"Native Q4 manifest length mismatch.");
  const unsigned header[]={Magic,1,p.tensor,p.firstRow,p.rows,unsigned(model->Tensors()[p.tensor].inputs),
   2304,e.tiles,e.indexBytes,Word(bytes.data()+36),p.bytes,0,0,0,0,0};
  for(unsigned i=0;i<16;i++)if(Word(bytes.data()+i*4)!=header[i])return Fail(error,"Native Q4 manifest geometry mismatch.");
  side=header[9];
  if(side<e.indexBytes || (side&15) || uint64_t(side)>uint64_t(e.indexBytes)+uint64_t(e.tiles)*512 || side>OffsetMask)
   return Fail(error,"Native Q4 manifest correction length invalid.");
  unsigned previous=e.indexBytes,previousKind=3;nonzero.clear();
  for(unsigned tile=0;tile<e.tiles;tile++){
   const unsigned index=Word(bytes.data()+ManifestHeader+tile*4),kind=index>>30,offset=index&OffsetMask;
   if(kind==Codec::Zero){if(offset)return Fail(error,"Noncanonical empty Q4 mask index.");continue;}
   if(kind>Codec::SparseRows || offset<e.indexBytes || offset>=side || (offset&15))return Fail(error,"Invalid Q4 mask index.");
   if(nonzero.empty()){if(offset!=e.indexBytes)return Fail(error,"Q4 correction gap before first mask.");}
   else if(!Span(previousKind,offset-previous) || offset<=previous)return Fail(error,"Q4 correction index overlap/gap.");
   nonzero.push_back(tile);previous=offset;previousKind=kind;
  }
  if(nonzero.empty()?side!=e.indexBytes:!Span(previousKind,side-previous))return Fail(error,"Q4 correction tail invalid.");
  for(unsigned i=e.tiles*4;i<e.indexBytes;i++)if(bytes[ManifestHeader+i])return Fail(error,"Nonzero Q4 index padding.");
  return Ready();
 }
 static bool Span(unsigned kind,unsigned bytes){return kind==Codec::Dense?bytes==512
  :kind==Codec::SparseRows && bytes>=48 && bytes<=512 && !(bytes&15);}
 bool OpenBundle(unsigned fragment,std::string& error){
  if(active==fragment)return reader && reader->IsOpen();
  reader.reset();active=UINT32_MAX;auto nextReader=std::make_unique<Cache::Reader>();
  if(!cache.OpenExisting(entries[fragment].bundleKey,[this]{return Ready();},*nextReader,error))return false;
  reader=std::move(nextReader);active=fragment;return true;
 }
public:
 struct Statistics{uint64_t fragments=0,manifestHits=0,bundleHits=0,convertedTiles=0,bytes=0;};
 struct CorrectionProposal{unsigned fragment,bank;uint32_t proposedAddress,bytes,allocated;};
 static std::string Recipe(const std::string& fingerprint){return
  "pioneer-q38-native-q4-exact-v1/SM3-forced-exp129/q=min(q,14)-7+7+(q==15)/"
  "original256B-metadata/native-K16N16-primary/16B-aligned-sparse16bit-row-masks/"
  "4B-index-kind2-offset30/zero-no-payload/dense512-fallback/no-products/"
  "manifest64-v1/atomic-primary-plus-sidecar/no-device-addresses/source-fingerprint="+fingerprint;
 }
 // Snapshot the complete original plan,including state and all source banks.
 // Caller authenticates the SAME open model first;Ready binds that identity.
 bool Bind(const char* directory,PioneerQwen38Gguf& source,const PioneerQwen38PackedPlan& original,
  const Cache::Digest& originalSHA,const Cache::Digest& conversionSHA,const Cache::Ready& ready,std::string& error){
  if(attempted)return Fail(error,"Native Q4 cache binding is one-shot.");attempted=true;
  if(!ready || !ready() || !source.Stable() || !original.MatchesSource(source) || !original.Tiled())
   return Fail(error,"Native Q4 cache requires authenticated original tiled source.");
  plan=original;entries.resize(plan.Parts().size());
  const auto& parts=plan.Parts();
  for(unsigned i=0;i<parts.size();i++){
   const auto& p=parts[i];if(p.kind!=PIONEER_Q38_WEIGHT || source.Tensors()[p.tensor].type!=12)continue;
   if(!p.bytes || p.bytes%2304 || (p.firstRow&15) || (p.rows&15) || p.bank>=7)return Fail(error,"Invalid original Q4 fragment.");
   auto& e=entries[i];e.q4=true;e.tiles=p.bytes/2304;e.indexBytes=Align16(e.tiles*4);
   e.manifestKey={originalSHA,conversionSHA,uint64_t(i)*2,ManifestHeader+e.indexBytes};
   e.bundleKey={originalSHA,conversionSHA,uint64_t(i)*2+1,0};expected++;
  }
  if(!expected || !cache.Open(directory,error) || !ready() || !source.Stable())return false;
  model=&source;epoch=source.Epoch();sourceReady=ready;
  for(unsigned bank=0;bank<7;bank++)used[bank]=plan.Used(bank);
  while(next<entries.size() && !entries[next].q4)next++;
  return Ready();
 }
 const PioneerQwen38PackedPlan& OriginalPlan()const{return plan;}
 unsigned NextFragment()const{return Ready()?next:UINT32_MAX;}
 bool AllPrepared()const{return Ready() && expected && prepared==expected;}
 uint64_t Used(unsigned bank)const{return Ready() && bank<7?used[bank]:0;}
 bool KeyFor(unsigned fragment,bool manifest,Cache::Key& key)const{
  if(!Ready() || fragment>=entries.size() || !entries[fragment].q4 || (!manifest && !entries[fragment].prepared))return false;
  key=manifest?entries[fragment].manifestKey:entries[fragment].bundleKey;return true;
 }
 bool Correction(unsigned fragment,CorrectionProposal& output)const{
  if(!Ready() || fragment>=entries.size() || !entries[fragment].prepared)return false;
  const auto& e=entries[fragment];const auto& p=plan.Parts()[fragment];
  output={fragment,p.bank,e.address,e.sideBytes,e.allocated};return true;
 }
 bool PrepareNext(Statistics& stats,std::string& error,const Cache::Progress& progress={}){
  if(!Ready() || next>=entries.size())return Fail(error,"No next native Q4 fragment.");
  const unsigned fragment=next;auto& e=entries[fragment];const auto p=plan.Parts()[fragment];
  std::vector<uint8_t> layout;Cache::Reader manifest;bool manifestHit=false,bundleHit=false;
  auto describe=[&](uint64_t offset,unsigned count,uint8_t* output,std::string& why){
   if(layout.empty() && !Manifest(fragment,layout,stats.convertedTiles,why))return false;
   if(offset>layout.size() || count>layout.size()-offset)return Fail(why,"Invalid Q4 manifest conversion range.");
   memcpy(output,layout.data()+offset,count);return Ready();
  };
  if(!cache.GetOrCreate(e.manifestKey,[this]{return Ready();},describe,manifest,manifestHit,error,progress))return false;
  layout.resize(size_t(e.manifestKey.bytes));
  for(unsigned at=0;at<layout.size();){
   const unsigned count=std::min(Cache::ChunkBytes,unsigned(layout.size())-at);
   if(!manifest.Read(at,count,layout.data()+at,count,error))return false;at+=count;
  }
  unsigned sideBytes=0;std::vector<unsigned> nonzero;
  if(!CheckManifest(fragment,layout,sideBytes,nonzero,error))return false;
  const uint64_t allocated=(uint64_t(sideBytes)+4095)&~UINT64_C(4095);
  if(used[p.bank]>PioneerQwen38PackedPlan::Capacity || allocated>PioneerQwen38PackedPlan::Capacity-used[p.bank])
   return Fail(error,"Exact Q4 corrections exceed original source-bank cap.");
  e.bundleKey.bytes=uint64_t(p.bytes)+sideBytes;auto bundle=std::make_unique<Cache::Reader>();
  auto produce=[&](uint64_t offset,unsigned count,uint8_t* output,std::string& why){
   if(!Ready() || offset>=e.bundleKey.bytes || !count || count>e.bundleKey.bytes-offset)return Fail(why,"Invalid Q4 bundle range.");
   memset(output,0,count);unsigned copied=0;
   while(copied<count){
    const uint64_t at=offset+copied;
    if(at<p.bytes){
     const unsigned tile=unsigned(at/2304),within=unsigned(at%2304),take=std::min(count-copied,2304-within);
     if(!Tile(fragment,tile,stats.convertedTiles,why))return false;
     memcpy(output+copied,encoded.data()+within,take);copied+=take;continue;
    }
    const unsigned relative=unsigned(at-p.bytes);
    if(relative<e.indexBytes){
     const unsigned take=std::min(count-copied,e.indexBytes-relative);
     memcpy(output+copied,layout.data()+ManifestHeader+relative,take);copied+=take;continue;
    }
    const auto after=std::upper_bound(nonzero.begin(),nonzero.end(),relative,[&](unsigned value,unsigned tile){
     return value<(Word(layout.data()+ManifestHeader+tile*4)&OffsetMask);
    });
    if(after==nonzero.begin())return Fail(why,"Q4 correction record missing.");
    const unsigned tile=*(after-1),index=Word(layout.data()+ManifestHeader+tile*4),start=index&OffsetMask;
    const unsigned end=after==nonzero.end()?sideBytes:Word(layout.data()+ManifestHeader+*after*4)&OffsetMask;
    if(relative<start || relative>=end || !Tile(fragment,tile,stats.convertedTiles,why))return false;
    std::array<uint8_t,512> packed{};Codec::Mask description;
    if(!Codec::PackMask(encoded.data()+2304,512,packed.data(),packed.size(),description)
     || description.kind!=(index>>30) || Align16(description.bytes)!=end-start)
     return Fail(why,"Original Q4 mask no longer matches manifest.");
    const unsigned within=relative-start,take=std::min(count-copied,end-relative);
    if(within<description.bytes)memcpy(output+copied,packed.data()+within,std::min(take,description.bytes-within));
    copied+=take;
   }
   return Ready();
  };
  if(!cache.GetOrCreate(e.bundleKey,[this]{return Ready();},produce,*bundle,bundleHit,error,progress) || !Ready())return false;
  e.sideBytes=sideBytes;e.address=uint32_t(PioneerQwen38PackedPlan::Base+used[p.bank]);e.allocated=unsigned(allocated);
  used[p.bank]+=allocated;e.prepared=true;prepared++;next++;
  while(next<entries.size() && !entries[next].q4)next++;
  // Retain the JUST verified reader instead of hashing the whole same
  // bundle a second time on its first upload-page read. Reads still hash
  // their actual chunks and recheck source/file identity independently.
  reader=std::move(bundle);active=fragment;
  stats.fragments++;stats.manifestHits+=manifestHit;stats.bundleHits+=bundleHit;stats.bytes+=e.bundleKey.bytes;
  return Ready();
 }
 // Read EXISTING complete verified bundles only. Missing/corrupt data fails
 // without repair or conversion after a future accelerator session opens.
 bool ReadPage(unsigned fragment,unsigned page,bool correction,uint8_t* output,unsigned capacity,std::string& error){
  if(!Ready() || !output || capacity<4096 || fragment>=entries.size() || !entries[fragment].prepared)
   return Fail(error,"Native Q4 fragment is not prepared.");
  const auto& e=entries[fragment];const auto& p=plan.Parts()[fragment];
  const uint64_t relative=uint64_t(page)*4096;const unsigned bytes=correction?e.sideBytes:p.bytes;
  const unsigned allocated=correction?e.allocated:p.allocated;
  if(page>=allocated/4096 || relative>=bytes || !OpenBundle(fragment,error))return Fail(error,"Invalid native Q4 upload page.");
  std::array<uint8_t,4096> stage{};const unsigned count=unsigned(std::min(UINT64_C(4096),uint64_t(bytes)-relative));
  if(!reader->Read((correction?uint64_t(p.bytes):0)+relative,count,stage.data(),stage.size(),error) || !Ready())return false;
  memcpy(output,stage.data(),stage.size());return true;
 }
};
#endif
