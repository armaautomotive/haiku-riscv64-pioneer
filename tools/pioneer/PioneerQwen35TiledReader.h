/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_TILED_READER_H
#define PIONEER_QWEN35_TILED_READER_H
#include "PioneerQwen35Gguf.h"

// Lossless packed-byte permutation only, with explicit 4B widths and Q5_K.
// The result is not authority to upload, execute, or substitute host inference.
static inline unsigned PioneerQ35TileBytes(unsigned type) {
 unsigned bytes=PioneerQ35BlockBytes(type);
 return type && bytes?((16*bytes+63)&~63U):0;
}
static inline unsigned PioneerQ35TiledRowBytes(unsigned type,uint64_t inputs) {
 unsigned tile=PioneerQ35TileBytes(type),block=PioneerQ35Block(type);
 return tile && (inputs==2560 || inputs==4096 || inputs==9216) && !(inputs%block)
  ?unsigned(inputs/block)*(tile/16):0;
}
class PioneerQwen35TiledReader {
 const PioneerQwen35Gguf* bound=nullptr;
 uint64_t epoch=0;
 unsigned cachedTensor=UINT32_MAX,cachedRow=UINT32_MAX;
 std::vector<uint8_t> cached;
public:
 void Reset() { bound=nullptr; epoch=0; cachedTensor=cachedRow=UINT32_MAX; cached.clear(); }
 bool Bind(const PioneerQwen35Gguf& source) {
  if (bound || !source.Stable()) return false;
  bound=&source; epoch=source.Epoch(); return true;
 }
 bool ReadPage(PioneerQwen35Gguf& source,unsigned tensor,unsigned firstRow,unsigned rows,
  unsigned page,uint8_t* output,unsigned capacity) {
  if (&source!=bound || source.Epoch()!=epoch || !source.Stable() || !output || capacity<4096
   || tensor>=source.Tensors().size() || (firstRow&15) || !rows || (rows&15)) return false;
  const auto& t=source.Tensors()[tensor];
  unsigned tileBytes=PioneerQ35TileBytes(t.type),rowBytes=PioneerQ35TiledRowBytes(t.type,t.inputs);
  if (!tileBytes || !rowBytes || firstRow>t.outputs || rows>t.outputs-firstRow) return false;
  uint64_t length=uint64_t(rows)*rowBytes,offset=uint64_t(page)*4096;
  if (offset>=length) return false;
  unsigned blockBytes=PioneerQ35BlockBytes(t.type),blocks=unsigned(t.inputs/PioneerQ35Block(t.type));
  unsigned originalRow=blocks*blockBytes;
  if (!originalRow || uint64_t(originalRow)*16>PioneerQwen35Gguf::MaxPackedRead) return false;
  uint8_t stage[4096]={}; unsigned filled=0;
  while (filled<4096 && offset<length) {
   uint64_t tile=offset/tileBytes;
   unsigned row=firstRow+unsigned(tile/blocks)*16,block=unsigned(tile%blocks),within=unsigned(offset%tileBytes);
   if (cachedTensor!=tensor || cachedRow!=row) {
    cachedTensor=cachedRow=UINT32_MAX; cached.resize(size_t(originalRow)*16);
    if (!source.ReadPacked(tensor,uint64_t(row)*originalRow,unsigned(cached.size()),cached.data(),unsigned(cached.size()))) {
     cached.clear(); return false;
    }
    cachedTensor=tensor; cachedRow=row;
   }
   unsigned take=std::min(tileBytes-within,4096-filled);
   for (unsigned i=0;i<take;i++) {
    unsigned byte=within+i;
    if (byte<16*blockBytes) stage[filled+i]=cached[(byte%16)*originalRow+block*blockBytes+byte/16];
   }
   filled+=take; offset+=take;
  }
  if (source.Epoch()!=epoch || !source.Stable()) return false;
  memcpy(output,stage,4096); return true;
 }
};
#endif
