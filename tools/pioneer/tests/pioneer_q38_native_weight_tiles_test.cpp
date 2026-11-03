/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38NativeWeightTiles.h"
#include "../PioneerModelWeightCache.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
struct Source {
	std::vector<PioneerQ38Tensor> tensors;
	std::vector<uint8_t> raw;
	uint64_t epoch=1;unsigned reads=0,fail=0,change=0;bool ready=true;
	bool Ready() const { return ready; }
	bool Stable() const { return ready; }
	uint64_t Epoch() const { return epoch; }
	const std::vector<PioneerQ38Tensor>& Tensors() const { return tensors; }
	bool ReadPacked(unsigned index,uint64_t offset,unsigned bytes,void* out,unsigned capacity) {
		reads++;if(index || !out || capacity<bytes || offset>raw.size() || bytes>raw.size()-offset || reads==fail)return false;
		memcpy(out,raw.data()+offset,bytes);if(reads==change)epoch++;return true;
	}
	static int Scale(unsigned row,unsigned block,unsigned group) { return int(((row+block+group)*13)%256)-128; }
	Source(unsigned type,unsigned inputs,unsigned outputs) {
		PioneerQ38Tensor t;t.rank=2;t.type=type;t.inputs=inputs;t.outputs=outputs;
		t.bytes=uint64_t(inputs/PioneerQ38BlockElements(type))*PioneerQ38BlockBytes(type)*outputs;tensors.push_back(t);raw.resize(size_t(t.bytes));
		const unsigned blocks=inputs/PioneerQ38BlockElements(type),size=PioneerQ38BlockBytes(type);
		for(unsigned row=0;row<outputs;row++)for(unsigned block=0;block<blocks;block++) {
			auto* p=raw.data()+(row*blocks+block)*size;
			if(type==8) { p[1]=0x38;for(unsigned i=0;i<32;i++)p[2+i]=uint8_t((row+block+i)%256); }
			if(type==12) {
				p[1]=0x34;p[3]=0x30;
				unsigned s[8],m[8];for(unsigned g=0;g<8;g++) { s[g]=(row+g*9+block)%64;m[g]=(row*3+g*5+block)%64; }
				for(unsigned g=0;g<4;g++) { p[4+g]=uint8_t(s[g]|((s[g+4]>>4)<<6));p[8+g]=uint8_t(m[g]|((m[g+4]>>4)<<6));p[12+g]=uint8_t((s[g+4]&15)|((m[g+4]&15)<<4)); }
				for(unsigned g=0;g<4;g++)for(unsigned i=0;i<32;i++)p[16+g*32+i]=uint8_t(((i+row+block+g*2)&15)|(((i+row+block+g*2+1)&15)<<4));
			}
			if(type==14) {
				p[209]=0x2c;
				for(unsigned i=0;i<256;i++) {
					const unsigned q=(i+row+block)%64,half=i/128,plane=(i%128)/32,lane=i%32;
					p[half*64+(plane&1)*32+lane]|=uint8_t((q&15)<<((plane/2)*4));
					p[128+half*32+lane]|=uint8_t((q>>4)<<(plane*2));
				}
				for(unsigned g=0;g<16;g++)p[192+g]=uint8_t(Scale(row,block,g));
			}
		}
	}
	float Expected(unsigned row,unsigned column) const {
		const unsigned type=tensors[0].type,block=column/PioneerQ38BlockElements(type),i=column%PioneerQ38BlockElements(type);
		if(type==8) { unsigned q=(row+block+i)%256;return 0.5f*(int(q)-int((q&128)*2)); }
		if(type==12) { const unsigned g=i/32;return (0.25f*((row+g*9+block)%64))*((i%32+row+block+g)&15)-(0.125f*((row*3+g*5+block)%64)); }
		return (0.0625f*Scale(row,block,i/16))*(int((i+row+block)%64)-32);
	}
};
#ifndef PIONEER_NATIVE_WEIGHT_TILES_FIXTURE_ONLY
int main(int argc,char** argv) {
	assert(argc==2);unsigned checked=0;
	for(unsigned type:{8U,12U,14U})for(unsigned inputs:{5120U,6144U,17408U})for(unsigned outputs:{1U,17U,48U,64U}) {
		Source source(type,inputs,outputs);PioneerQ38NativeWeightTiles::Layout layout;
		assert(PioneerQ38NativeWeightTiles::Describe(source.tensors[0],layout));
		for(uint64_t tile:{UINT64_C(0),layout.rowTiles*layout.columnTiles/2,layout.rowTiles*layout.columnTiles-1}) {
			std::array<float,1024> actual;assert(PioneerQ38NativeWeightTiles::ReadFloatTile(source,0,tile,actual.data(),1024));
			const unsigned firstRow=unsigned(tile/layout.columnTiles)*32,firstColumn=unsigned(tile%layout.columnTiles)*32;
			for(unsigned r=0;r<32;r++)for(unsigned c=0;c<32;c++)assert(actual[r*32+c]==(firstRow+r<outputs?source.Expected(firstRow+r,firstColumn+c):0));
			std::array<uint8_t,1088> expected,actualBytes;
			assert(PioneerBlackholeBlockFloat::PackTile(layout.format,actual.data(),1024,expected.data(),expected.size()));
			const unsigned bytes=PioneerBlackholeBlockFloat::Bytes(layout.format);
			assert(PioneerQ38NativeWeightTiles::ReadRange(source,0,tile*bytes,bytes,actualBytes.data(),actualBytes.size()));
			assert(!memcmp(expected.data(),actualBytes.data(),bytes));checked++;
		}
	}
	for(unsigned missing:{1U,16U,32U})for(bool epoch:{false,true}) {
		Source source(12,5120,48);if(epoch)source.change=missing;else source.fail=missing;
		std::array<uint8_t,65536> output;output.fill(0xa7);const auto before=output;
		assert(!PioneerQ38NativeWeightTiles::ReadRange(source,0,0,65536,output.data(),output.size()) && output==before && source.reads==missing);checked++;
	}
	for(int mode:{FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO}) {
		Source source(12,5120,48);std::array<float,1024> output;output.fill(19);const auto before=output;
		assert(!std::fesetround(mode));
		assert(!PioneerQ38NativeWeightTiles::ReadFloatTile(source,0,0,output.data(),output.size()) && output==before && !source.reads);
		assert(!std::fesetround(FE_TONEAREST));checked++;
	}
	Source source(12,5120,48);PioneerQ38NativeWeightTiles::Layout layout;assert(PioneerQ38NativeWeightTiles::Describe(source.tensors[0],layout));
	PioneerModelWeightCache cache;std::string error;assert(cache.Open((std::string(argv[1])+"/converted-tiles").c_str(),error));
	PioneerModelWeightCache::Key key;key.sourceSHA256=PioneerModelWeightCache::Hash(source.raw.data(),source.raw.size());
	const char recipe[]="unit-native-Q4K-to-BFP4_B-output-major-v1";key.conversionSHA256=PioneerModelWeightCache::Hash(recipe,sizeof(recipe)-1);key.bytes=layout.bytes;
	auto ready=[&] { return source.Ready() && source.Stable() && source.Epoch()==1; };
	unsigned converted=0;auto produce=[&](uint64_t offset,unsigned count,uint8_t* output,std::string&) {
		converted++;return PioneerQ38NativeWeightTiles::ReadRange(source,0,offset,count,output,count);
	};
	{
		PioneerModelWeightCache::Reader r;bool hit=true;assert(cache.GetOrCreate(key,ready,produce,r,hit,error) && !hit && converted>0);
		std::array<uint8_t,4096> cached,live;
		for(uint64_t offset=0;offset<layout.bytes;offset+=4096) {
			unsigned count=unsigned(std::min(UINT64_C(4096),layout.bytes-offset));
			assert(r.Read(offset,count,cached.data(),cached.size(),error));
			assert(PioneerQ38NativeWeightTiles::ReadRange(source,0,offset,count,live.data(),live.size()));
			assert(!memcmp(cached.data(),live.data(),count));
		}
	}
	{
		const unsigned before=converted,reads=source.reads;PioneerModelWeightCache::Reader r;bool hit=false;
		assert(cache.GetOrCreate(key,ready,produce,r,hit,error) && hit && converted==before && source.reads==reads);checked++;
	}
	printf("PASS CPU-only original Q8/Q4/Q6 native-weight tiles:%u checks;independent GGUF formulas,widths,padded rows,source loss/epoch atomicity,real converter-to-disk roundtrip and ZERO conversion on hit;NO GPU/model-quality/speed proof\n",checked);
}
#endif
