/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_WEIGHT_TILES_FIXTURE_ONLY
#include "pioneer_q38_native_weight_tiles_test.cpp"
#include "../PioneerQ38NativeBfp8WeightTiles.h"
#include <cmath>
int main(int argc,char** argv) {
	assert(argc==2);using Tiles=PioneerQ38NativeBfp8WeightTiles;using Codec=Tiles::Codec;
	unsigned cases=0,q4Cases=0;double q4MixedError=0,q4Bfp8Error=0;
	for(unsigned type:{8U,12U,14U})for(unsigned inputs:{5120U,6144U,17408U})for(unsigned outputs:{1U,17U,48U,64U}) {
		Source source(type,inputs,outputs);Tiles::Layout eight,mixed;
		assert(Tiles::Describe(source.tensors[0],eight) && PioneerQ38NativeWeightTiles::Describe(source.tensors[0],mixed));
		assert(eight.format==Codec::Format::Bfp8B && eight.rowTiles==mixed.rowTiles && eight.columnTiles==mixed.columnTiles);
		assert(eight.bytes==eight.rowTiles*eight.columnTiles*1088 && (type==12?eight.bytes*576==mixed.bytes*1088:eight.bytes==mixed.bytes));
		for(uint64_t tile:{UINT64_C(0),eight.rowTiles*eight.columnTiles/2,eight.rowTiles*eight.columnTiles-1}) {
			std::array<float,1024> values{},decoded{};std::array<uint8_t,1088> expected{},actual{},four{};
			assert(PioneerQ38NativeWeightTiles::ReadFloatTile(source,0,tile,values.data(),values.size()));
			const unsigned row=unsigned(tile/eight.columnTiles)*32,column=unsigned(tile%eight.columnTiles)*32;
			for(unsigned r=0;r<32;r++)for(unsigned c=0;c<32;c++)assert(values[r*32+c]==(row+r<outputs?source.Expected(row+r,column+c):0));
			assert(Codec::PackTile(Codec::Format::Bfp8B,values.data(),1024,expected.data(),expected.size()));
			assert(Tiles::ReadRange(source,0,tile*1088,1088,actual.data(),actual.size()) && actual==expected);
			assert(Codec::UnpackTile(Codec::Format::Bfp8B,actual.data(),1088,decoded.data(),decoded.size()));
			if(type==12) {
				for(unsigned i=0;i<1024;i++){double d=double(values[i])-decoded[i];q4Bfp8Error+=d*d;}
				assert(Codec::PackTile(Codec::Format::Bfp4B,values.data(),1024,four.data(),four.size()));
				assert(Codec::UnpackTile(Codec::Format::Bfp4B,four.data(),576,decoded.data(),decoded.size()));
				for(unsigned i=0;i<1024;i++){double d=double(values[i])-decoded[i];q4MixedError+=d*d;}
				q4Cases++;
			}
			cases++;
		}
	}
	assert(q4Cases==36 && q4Bfp8Error<q4MixedError);
	for(unsigned missing:{1U,16U,32U})for(bool epoch:{false,true}) {
		Source source(12,5120,48);if(epoch)source.change=missing;else source.fail=missing;
		std::array<uint8_t,65536> out;out.fill(0xab);const auto before=out;
		assert(!Tiles::ReadRange(source,0,0,65536,out.data(),out.size()) && out==before && source.reads==missing);cases++;
	}
	Source source(12,5120,48);Tiles::Layout layout;assert(Tiles::Describe(source.tensors[0],layout));
	PioneerModelWeightCache cache;std::string error;assert(cache.Open((std::string(argv[1])+"/all-bfp8").c_str(),error));
	PioneerModelWeightCache::Key key;key.sourceSHA256=PioneerModelWeightCache::Hash(source.raw.data(),source.raw.size());
	const char recipe[]="unit-SEPARATE-all-native-BFP8-v2";key.conversionSHA256=PioneerModelWeightCache::Hash(recipe,sizeof(recipe)-1);key.bytes=layout.bytes;
	unsigned conversions=0;auto ready=[&]{return source.Ready() && source.Epoch()==1;};
	auto produce=[&](uint64_t at,unsigned n,uint8_t* out,std::string&){conversions++;return Tiles::ReadRange(source,0,at,n,out,n);};
	{
		PioneerModelWeightCache::Reader reader;bool hit=true;
		assert(cache.GetOrCreate(key,ready,produce,reader,hit,error) && !hit && conversions);
		std::array<uint8_t,65536> cached{},fresh{};
		for(uint64_t offset:{UINT64_C(0),UINT64_C(59999),layout.bytes-65536}) {
			assert(reader.Read(offset,cached.size(),cached.data(),cached.size(),error));
			assert(Tiles::ReadRange(source,0,offset,fresh.size(),fresh.data(),fresh.size()) && cached==fresh);cases++;
		}
	}
	{
		const unsigned calls=conversions,reads=source.reads;PioneerModelWeightCache::Reader reader;bool hit=false;
		assert(cache.GetOrCreate(key,ready,produce,reader,hit,error) && hit && conversions==calls && reads==source.reads);cases++;
	}
	printf("PASS CPU-only separate all-native-BFP8:%u checks,%u synthetic Q4 tiles lower aggregate squared error than BFP4;all Q8/Q4/Q6 independent original formulas,widths,padded rows,atomic source loss/epoch,cross-tile/chunk disk reads and ZERO conversion/packed reads on hit;NO real model quality,bank fit,driver,GPU or speed proof\n",cases,q4Cases);
}
