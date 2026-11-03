/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_BFP8_WEIGHT_TILES_H
#define PIONEER_Q38_NATIVE_BFP8_WEIGHT_TILES_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Unqualified all-BFP8 conversion is CPU preparation only, never driver/Work/validation
#endif
#include "PioneerQ38NativeWeightTiles.h"
// Separate precision/storage recipe. Even original Q4_K weights become BFP8_B
// rather than BFP4_B. This does NOT claim bank fit, physical math, model quality
// or speed. Original GGUF bytes are unchanged. Existing recipes are untouched.
struct PioneerQ38NativeBfp8WeightTiles {
	using Codec=PioneerBlackholeBlockFloat;
	using Layout=PioneerQ38NativeWeightTiles::Layout;
	static bool Describe(const PioneerQ38Tensor& tensor,Layout& output) {
		Layout next;if(!PioneerQ38NativeWeightTiles::Describe(tensor,next))return false;
		next.format=Codec::Format::Bfp8B;
		if(next.rowTiles>UINT64_MAX/next.columnTiles/Codec::Bytes(next.format))return false;
		next.bytes=next.rowTiles*next.columnTiles*Codec::Bytes(next.format);output=next;return true;
	}
	template<class Source> static bool ReadRange(Source& source,unsigned tensorIndex,uint64_t offset,unsigned bytes,
		uint8_t* output,unsigned capacity) {
		if(!output || !bytes || bytes>65536 || capacity<bytes || !source.Ready() || !source.Stable()
			|| tensorIndex>=source.Tensors().size())return false;
		const uint64_t epoch=source.Epoch();Layout layout;
		if(!Describe(source.Tensors()[tensorIndex],layout) || offset>layout.bytes || bytes>layout.bytes-offset)return false;
		std::array<float,1024> values{};std::array<uint8_t,Codec::MaxBytes> tile;
		std::array<uint8_t,65536> stage;unsigned copied=0;const unsigned tileBytes=Codec::Bytes(layout.format);
		while(copied<bytes) {
			const uint64_t position=offset+copied,index=position/tileBytes;const unsigned within=unsigned(position%tileBytes);
			if(!PioneerQ38NativeWeightTiles::ReadFloatTile(source,tensorIndex,index,values.data(),values.size())
				|| !Codec::PackTile(layout.format,values.data(),values.size(),tile.data(),tile.size()))return false;
			const unsigned take=std::min(bytes-copied,tileBytes-within);
			std::memcpy(stage.data()+copied,tile.data()+within,take);copied+=take;
		}
		if(source.Epoch()!=epoch || !source.Ready() || !source.Stable())return false;
		std::memcpy(output,stage.data(),bytes);return true;
	}
};
#endif
