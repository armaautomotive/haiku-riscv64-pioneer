/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_WEIGHT_TILES_H
#define PIONEER_Q38_NATIVE_WEIGHT_TILES_H
#include "PioneerBlackholeBlockFloat.h"
#include "PioneerQwen38Gguf.h"
#include <array>
#include <cfenv>
#include <cstring>

// CPU-only NEW numerical/layout path for an independently authenticated
// source. NO current packed plan,driver,arena or Assistant mode selects it.
// Tile order is output-row-major then K tile,each32x32 in native face order.
// Q4_K -> BFP4_B;Q8_0/Q6_K -> BFP8_B. This is REQUANTIZATION,not the old
// lossless byte-major representation. Quality and native math need their
// own qualification before any hardware uses the cached bytes.
struct PioneerQ38NativeWeightTiles {
	using Codec=PioneerBlackholeBlockFloat;
	struct Layout {
		Codec::Format format=Codec::Format(0);
		uint64_t rowTiles=0,columnTiles=0,bytes=0;
	};
	static bool Describe(const PioneerQ38Tensor& tensor,Layout& output) {
		if(tensor.rank!=2 || (tensor.type!=8 && tensor.type!=12 && tensor.type!=14)
			|| !tensor.inputs || !tensor.outputs || tensor.inputs>UINT32_MAX || tensor.outputs>UINT32_MAX
			|| tensor.inputs%PioneerQ38BlockElements(tensor.type))return false;
		Layout next;next.format=tensor.type==12?Codec::Format::Bfp4B:Codec::Format::Bfp8B;
		next.rowTiles=(tensor.outputs+31)/32;next.columnTiles=(tensor.inputs+31)/32;
		if(next.rowTiles>UINT64_MAX/next.columnTiles/Codec::Bytes(next.format))return false;
		next.bytes=next.rowTiles*next.columnTiles*Codec::Bytes(next.format);output=next;return true;
	}
	template<class Source> static bool ReadFloatTile(Source& source,unsigned tensorIndex,uint64_t tile,
		float* output,unsigned capacity) {
		if(!output || capacity<1024 || reinterpret_cast<uintptr_t>(output)%alignof(float)
			|| std::fegetround()!=FE_TONEAREST || !source.Ready() || !source.Stable() || tensorIndex>=source.Tensors().size())return false;
		const auto tensor=source.Tensors()[tensorIndex];const uint64_t epoch=source.Epoch();Layout layout;
		if(!Describe(tensor,layout) || tile>=layout.rowTiles*layout.columnTiles)return false;
		const uint64_t firstRow=tile/layout.columnTiles*32,firstColumn=tile%layout.columnTiles*32;
		const unsigned blockElements=PioneerQ38BlockElements(tensor.type),blockBytes=PioneerQ38BlockBytes(tensor.type);
		const uint64_t rowBytes=tensor.inputs/blockElements*blockBytes;
		if(rowBytes>UINT64_MAX/tensor.outputs || tensor.bytes!=rowBytes*tensor.outputs)return false;
		std::array<float,1024> stage{};std::array<float,256> values{};std::array<uint8_t,210> packed{};
		for(unsigned row=0;row<32 && firstRow+row<tensor.outputs;row++) {
			const uint64_t offset=(firstRow+row)*rowBytes+(firstColumn/blockElements)*blockBytes;
			if(!source.ReadPacked(tensorIndex,offset,blockBytes,packed.data(),packed.size())
				|| source.Epoch()!=epoch || !source.Ready() || !source.Stable())return false;
			const bool okay=tensor.type==8?PioneerDecodeQ8_0(packed.data(),values.data()):
				tensor.type==12?PioneerDecodeQ4K(packed.data(),values.data()):PioneerDecodeQ6K(packed.data(),values.data());
			if(!okay)return false;
			const unsigned within=unsigned(firstColumn%blockElements);
			for(unsigned column=0;column<32 && firstColumn+column<tensor.inputs;column++)stage[row*32+column]=values[within+column];
		}
		if(source.Epoch()!=epoch || !source.Ready() || !source.Stable())return false;
		std::memcpy(output,stage.data(),sizeof(stage));return true;
	}
	template<class Source> static bool ReadRange(Source& source,unsigned tensorIndex,uint64_t offset,unsigned bytes,
		uint8_t* output,unsigned capacity) {
		if(!output || !bytes || bytes>65536 || capacity<bytes || !source.Ready() || !source.Stable()
			|| tensorIndex>=source.Tensors().size())return false;
		const auto tensor=source.Tensors()[tensorIndex];const uint64_t epoch=source.Epoch();Layout layout;
		if(!Describe(tensor,layout) || offset>layout.bytes || bytes>layout.bytes-offset)return false;
		std::array<float,1024> values{};std::array<uint8_t,Codec::MaxBytes> tile;
		std::array<uint8_t,65536> stage;unsigned copied=0;const unsigned tileBytes=Codec::Bytes(layout.format);
		while(copied<bytes) {
			const uint64_t position=offset+copied,index=position/tileBytes;const unsigned within=unsigned(position%tileBytes);
			if(!ReadFloatTile(source,tensorIndex,index,values.data(),values.size())
				|| !Codec::PackTile(layout.format,values.data(),values.size(),tile.data(),tile.size()))return false;
			const unsigned take=std::min(bytes-copied,tileBytes-within);
			std::memcpy(stage.data()+copied,tile.data()+within,take);copied+=take;
		}
		if(source.Epoch()!=epoch || !source.Ready() || !source.Stable())return false;
		std::memcpy(output,stage.data(),bytes);return true;
	}
};
#endif
