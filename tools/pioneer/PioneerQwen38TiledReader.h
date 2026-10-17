/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_TILED_READER_H
#define PIONEER_QWEN38_TILED_READER_H
#include "PioneerQwen38Gguf.h"

// Lossless OFFLINE storage permutation,not a kernel/route/launch authority.
// Each tile contains one original quant block from16 output rows,byte-major
// across those rows. Aligned64B tiles let a future worker read the group in
// one bounded NoC request instead of sixteen scattered GGUF row reads.
// Packed nibbles/scales/signed bytes are UNCHANGED; no weight expansion,
// SM7 rewrite,-128 clipping,re-quantization or host inference occurs here.
// One serialized reader/cache per FILE;do not share it across loader threads.
// A future compute registry MUST explicitly distinguish this from GGUF row
// layout. Neither installed43 nor storage-only44 can execute these tiles.
static inline unsigned PioneerQ38TileBytes(unsigned type)
{
	unsigned bytes=type==8?34:type==12?144:type==14?210:0;
	return bytes?((16*bytes+63)&~63U):0;
}
static inline unsigned PioneerQ38TiledRowBytes(unsigned type,uint64_t inputs)
{
	unsigned tile=PioneerQ38TileBytes(type),block=PioneerQ38BlockElements(type);
	return tile && inputs && inputs<=17408 && !(inputs%block)?unsigned(inputs/block)*(tile/16):0;
}
class PioneerQwen38TiledReader {
	const PioneerQwen38Gguf* bound=nullptr;
	uint64_t epoch=0;
	unsigned cachedTensor=UINT32_MAX,cachedRow=UINT32_MAX;
	std::vector<uint8_t> cached; // at most16 ORIGINAL packed rows,never F32/BF16 weights
public:
	void Reset() { bound=nullptr; epoch=0; cachedTensor=cachedRow=UINT32_MAX; cached.clear(); }
	bool Bind(const PioneerQwen38Gguf& source) {
		if (bound || !source.Stable()) return false;
		bound=&source; epoch=source.Epoch(); return true;
	}
	bool ReadPage(PioneerQwen38Gguf& source,unsigned tensor,unsigned firstRow,unsigned rows,
		unsigned page,uint8_t* output,unsigned capacity)
	{
		if (&source!=bound || source.Epoch()!=epoch || !source.Stable() || !output || capacity<4096
			|| tensor>=source.Tensors().size() || (firstRow&15) || !rows || (rows&15)) return false;
		const auto& t=source.Tensors()[tensor];
		unsigned tileBytes=PioneerQ38TileBytes(t.type),rowBytes=PioneerQ38TiledRowBytes(t.type,t.inputs);
		if (!tileBytes || !rowBytes || firstRow>t.outputs || rows>t.outputs-firstRow) return false;
		uint64_t length=uint64_t(rows)*rowBytes,offset=uint64_t(page)*4096;
		if (offset>=length) return false;
		unsigned blockBytes=PioneerQ38BlockBytes(t.type),blocks=unsigned(t.inputs/PioneerQ38BlockElements(t.type));
		unsigned originalRow=blocks*blockBytes;
		if (!originalRow || uint64_t(originalRow)*16>PioneerQwen38Gguf::MaxPackedRead) return false;
		uint8_t stage[4096]={}; unsigned filled=0;
		while (filled<4096 && offset<length) {
			uint64_t tile=offset/tileBytes;
			unsigned row=firstRow+unsigned(tile/blocks)*16,block=unsigned(tile%blocks),within=unsigned(offset%tileBytes);
			if (cachedTensor!=tensor || cachedRow!=row) {
				cachedTensor=cachedRow=UINT32_MAX;
				cached.resize(size_t(originalRow)*16);
				if (!source.ReadPacked(tensor,uint64_t(row)*originalRow,unsigned(cached.size()),cached.data(),unsigned(cached.size()))) {
					cached.clear(); return false;
				}
				cachedTensor=tensor; cachedRow=row;
			}
			unsigned take=tileBytes-within;
			if (take>4096-filled) take=4096-filled;
			for (unsigned i=0;i<take;i++) {
				unsigned byte=within+i;
				if (byte<16*blockBytes)
					stage[filled+i]=cached[(byte%16)*originalRow+block*blockBytes+byte/16];
			}
			filled+=take; offset+=take;
		}
		if (source.Epoch()!=epoch || !source.Stable()) return false;
		memcpy(output,stage,4096); return true; // includes ALL tile/final-page zero padding
	}
};
#endif
