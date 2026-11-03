/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_BFP_BAND_DATA_H
#define PIONEER_Q38_NATIVE_BFP_BAND_DATA_H
#include "PioneerQ38NativeBfpBand.h"
#include "PioneerModelWeightCache.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <vector>
// CPU diagnostic/prelaunch DATA only, not a GPU uploader/allocator/capability.
// This is NOT a proposed host-per-projection inference path. A future native
// service must stage from resident GDDR itself; this defines/tests its format.
struct PioneerQ38NativeBfpBandData {
	struct Packet {
		unsigned width=0,matrixRows=0,firstRow=0,validRows=0,phases=0;
		PioneerModelWeightCache::Key key{};uint64_t sourceEpoch=0;
		std::vector<uint8_t> weights,inputs;
	};
	static bool Geometry(unsigned width,unsigned rows,unsigned first,const uint16_t* input,unsigned count,unsigned phases,std::string& error) {
		if(!PioneerQ38NativeBfpBand::Width(width) || !rows || rows>248320 || first>=rows || first%32
			|| !input || reinterpret_cast<uintptr_t>(input)%alignof(uint16_t) || count!=width || (phases!=1 && phases!=4)
			|| reinterpret_cast<uintptr_t>(input)>UINTPTR_MAX-size_t(count)*sizeof(uint16_t)) {
			error="Invalid native band geometry,explicit fidelity or BF16 input span.";return false;
		}
		return true;
	}
	static bool Capture(unsigned width,unsigned rows,unsigned first,const uint16_t* input,unsigned count,unsigned phases,
		std::vector<uint16_t>& output,std::string& error) {
		if(!Geometry(width,rows,first,input,count,phases,error))return false;
		std::vector<uint16_t> snapshot(input,input+count);
		for(unsigned i=0;i<count;i++) {
			const unsigned exponent=(snapshot[i]>>7)&255;
			if(exponent==255 || (!exponent && (snapshot[i]&127))) {error="Native band input must be finite normal/zero BF16.";return false;}
		}
		output=std::move(snapshot);return true;
	}
private:
	static bool Canonical(const uint8_t* tile,unsigned validRows) {
		for(unsigned group=0;group<64;group++) {
			const unsigned exponent=tile[group],row=(group/32)*16+(group%16);bool top=false;
			if((exponent && (exponent<8 || exponent==255)) || (row>=validRows && exponent))return false;
			for(unsigned column=0;column<16;column++) {
				const unsigned code=tile[64+group*16+column],magnitude=code&127;
				if((!magnitude && code) || (!exponent && code) || (row>=validRows && code))return false;
				top|=magnitude>=64;
			}
			if(bool(exponent)!=top)return false;
		}
		return true;
	}
public:
	// Reader must already be opened for the exact authenticated source/recipe
	// key by the bound source below. Generic Reader support is for synthetic
	// tests only,not a declaration that arbitrary bytes are authenticated.
	template<class Reader> static bool Assemble(Reader& reader,unsigned width,unsigned rows,unsigned first,
		const uint16_t* input,unsigned count,unsigned phases,const std::function<bool()>& ready,Packet& output,std::string& error) {
		std::vector<uint16_t> captured;
		if(!Capture(width,rows,first,input,count,phases,captured,error))return false;
		const unsigned tiles=width/32,validRows=std::min(32U,rows-first);
		const uint64_t bandBytes=uint64_t(tiles)*1088,totalBytes=((uint64_t(rows)+31)/32)*bandBytes,start=uint64_t(first/32)*bandBytes;
		if(!ready || !ready() || !reader.IsOpen() || reader.Size()!=totalBytes) {error="Native band source/key/size is unavailable.";return false;}
		std::vector<uint8_t> raw(size_t(bandBytes),0);
		for(unsigned copied=0;copied<raw.size();) {
			const unsigned take=unsigned(std::min(size_t(65536),raw.size()-copied));
			if(!ready()){error="Original source changed before native band read.";return false;}
			if(!reader.Read(start+copied,take,raw.data()+copied,take,error))return false;
			if(!ready() || !reader.IsOpen()){error="Original/cache source changed during native band read.";return false;}
			copied+=take;
		}
		Packet next;next.width=width;next.matrixRows=rows;next.firstRow=first;next.validRows=validRows;next.phases=phases;
		next.weights.resize(size_t(tiles)*1104,0);next.inputs.resize(size_t(tiles)*80,0);
		for(unsigned tile=0;tile<tiles;tile++) {
			if(!ready() || !Canonical(raw.data()+tile*1088,validRows)) {error="Native cached tile/source is noncanonical or has nonzero padded rows.";return false;}
			std::memcpy(next.weights.data()+tile*1104+16,raw.data()+tile*1088,1088);
			for(unsigned c=0;c<32;c++) {const uint16_t value=captured[tile*32+c];next.inputs[tile*80+16+c*2]=uint8_t(value);next.inputs[tile*80+17+c*2]=uint8_t(value>>8);}
		}
		if(!ready() || !reader.IsOpen()) {error="Source/cache changed before native band publication.";return false;}
		output=std::move(next);error.clear();return true;
	}
};
#endif
