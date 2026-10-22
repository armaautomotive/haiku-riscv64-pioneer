/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREDECODED_Q4_READER_H
#define PIONEER_Q38_PREDECODED_Q4_READER_H
#include "PioneerQ38PredecodedQ4Page.h"
#include <memory>
#include <new>
// OFFLINE DATA reader ONLY. Model must provide immutable tensor descriptors,
// Epoch/Stable and bounded ReadPacked. These are coherence,NOT authentication.
// A future uploader must independently authenticate the SAME source and own its
// buffers. This class grants no layout/ABI/allocation/device/cleanup authority.
// One serialized reader per source;source must outlive the reader. No rebind or
// retry after a failed source read/coherence check. Never used by normal loader.
template<class Model> class PioneerQ38PredecodedQ4Reader {
 static constexpr unsigned MaxRowBytes=(17408/256)*144;
 static constexpr unsigned MaxCacheBytes=16*MaxRowBytes;
 Model* bound=nullptr;
 uint64_t epoch=0;
 bool attempted=false,failed=false;
 std::unique_ptr<uint8_t[]> cache;
 unsigned cachedTensor=UINT32_MAX,cachedRow=UINT32_MAX;
 struct Fragment {unsigned tensor,firstRow,rows,blocks,rowBytes,tiles;};
 void Fault(){failed=true;cachedTensor=cachedRow=UINT32_MAX;cache.reset();}
 bool Ready() {
  if(!bound || failed)return false;
  if(bound->Epoch()!=epoch || !bound->Stable()){Fault();return false;}
  return true;
 }
 bool Describe(Model& source,unsigned tensor,unsigned firstRow,unsigned rows,Fragment& output) {
  if(&source!=bound || !Ready() || tensor>=source.Tensors().size()
   || (firstRow&15) || !rows || (rows&15))return false;
  const auto& t=source.Tensors()[tensor];
  if(t.type!=12 || t.rank!=2 || (t.inputs!=5120 && t.inputs!=6144 && t.inputs!=17408)
   || !t.outputs || t.outputs>UINT32_MAX || firstRow>t.outputs || rows>t.outputs-firstRow)return false;
  unsigned blocks=unsigned(t.inputs/256),rowBytes=blocks*144;
  if(t.bytes!=uint64_t(t.outputs)*rowBytes || rowBytes>MaxRowBytes)return false;
  uint64_t tiles=uint64_t(rows/16)*blocks;
  if(!tiles || tiles>PioneerQ38PredecodedQ4Page::MaxTiles || !Ready())return false;
  output={tensor,firstRow,rows,blocks,rowBytes,unsigned(tiles)};return true;
 }
 bool ReadTile(const Fragment& f,unsigned tile,uint8_t* output,unsigned capacity) {
  if(!output || capacity!=2304 || tile>=f.tiles || !Ready())return false;
  unsigned group=tile/f.blocks,block=tile%f.blocks;
  if(group>=f.rows/16)return false;
  unsigned row=f.firstRow+group*16;
  if(cachedTensor!=f.tensor || cachedRow!=row) {
   cachedTensor=cachedRow=UINT32_MAX;
   if(!cache)cache.reset(new(std::nothrow) uint8_t[MaxCacheBytes]);
   if(!cache){Fault();return false;}
   unsigned bytes=f.rowBytes*16;
   if(bytes>MaxCacheBytes || !bound->ReadPacked(f.tensor,uint64_t(row)*f.rowBytes,bytes,cache.get(),MaxCacheBytes)
    || !Ready()){Fault();return false;}
   cachedTensor=f.tensor;cachedRow=row;
  }
  for(unsigned byte=0;byte<2304;byte++)
   output[byte]=cache[(byte%16)*f.rowBytes+block*144+byte/16];
  return Ready();
 }
 struct TileSource {
  PioneerQ38PredecodedQ4Reader& reader;const Fragment& fragment;
  bool Ready(){return reader.Ready();}
  bool ReadTile(unsigned tile,uint8_t* out,unsigned capacity){return reader.ReadTile(fragment,tile,out,capacity);}
 };
public:
 PioneerQ38PredecodedQ4Reader()=default;
 PioneerQ38PredecodedQ4Reader(const PioneerQ38PredecodedQ4Reader&)=delete;
 PioneerQ38PredecodedQ4Reader& operator=(const PioneerQ38PredecodedQ4Reader&)=delete;
 bool Bind(Model& source) {
  if(attempted)return false;
  attempted=true;
  if(!source.Stable() || !source.Epoch()){Fault();return false;}
  bound=&source;epoch=source.Epoch();return Ready();
 }
 bool FragmentBytes(Model& source,unsigned tensor,unsigned firstRow,unsigned rows,uint32_t& output) {
  Fragment f={};if(!Describe(source,tensor,firstRow,rows,f))return false;
  output=f.tiles*PioneerQ38PredecodedQ4::Bytes;return true;
 }
 bool ReadPage(Model& source,unsigned tensor,unsigned firstRow,unsigned rows,uint64_t page,uint8_t* output,unsigned capacity) {
  if(!output || capacity<4096)return false;
  Fragment f={};if(!Describe(source,tensor,firstRow,rows,f))return false;
  TileSource tiles={*this,f};
  return PioneerQ38PredecodedQ4Page::Read(tiles,f.tiles,page,output,capacity);
 }
};
#endif
