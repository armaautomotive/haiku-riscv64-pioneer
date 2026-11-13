/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_MODEL_BAND_SOURCE_H
#define PIONEER_NATIVE_MODEL_BAND_SOURCE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Learned-band CPU reference is a separate diagnostic,never driver/Work/validation
#endif
#if defined(PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE) && (PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE != 1 \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error Dense Q8 learned-band source is a private diagnostic only
#endif
#if defined(PIONEER_NATIVE_RECURRENT_BAND_SOURCE) && (PIONEER_NATIVE_RECURRENT_BAND_SOURCE != 1 \
 || defined(PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error Recurrent Q8 learned-band source is a private diagnostic only
#endif
#if defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE) && (PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE != 1 \
 || defined(PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE) || defined(PIONEER_NATIVE_RECURRENT_BAND_SOURCE) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error Recurrent output Q8 learned-band source is a private diagnostic only
#endif
#include "PioneerModelWeightCache.h"
#include "PioneerQ38NativeWeightTiles.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include "PioneerNativeBfpModelBandMetadata.h" // PRIVATE generated predictions
#include <array>
#include <cfenv>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
// Full same-open source authentication and FRESH original debug reference.
// NO cache creation/repair,card calls,CPU Work fallback or model inference.
class PioneerNativeModelBandSource {
	using Cache=PioneerModelWeightCache;
	using Meta=PioneerNativeBfpModelBandMetadata;
	std::unique_ptr<FILE,decltype(&fclose)> file{nullptr,&fclose};
	PioneerQwen38Gguf model;uint64_t epoch=0;bool attempted=false,prepared=false;
	static bool Fail(std::string& error,const char* text){error=text;return false;}
	bool Stable()const{return file && model.Ready() && model.Epoch()==epoch && model.Stable();}
public:
	bool Ready()const{return prepared && Stable();}
	bool Prepare(const char* path,std::string& error) {
		if(attempted)return Fail(error,"Learned source preparation is one-shot.");
		attempted=true;
		if(!path || std::fegetround()!=FE_TONEAREST)return Fail(error,"Original path/default rounding required.");
		const int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return Fail(error,"Original read-only open refused.");
		struct stat info={};
		if(fstat(fd,&info) || !S_ISREG(info.st_mode) || info.st_nlink!=1 || info.st_uid!=geteuid() || (info.st_mode&0022)
			|| uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return Fail(error,"Original identity/permissions/size refused.");}
		FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return Fail(error,"Original stream refused.");}file.reset(stream);
		if(!model.Open(file.get()))return Fail(error,"Complete original layout refused.");
		epoch=model.Epoch();
		Cache::Digest digest={};
		if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error) || !Stable()
			|| Cache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return Fail(error,"Original full SHA/stability refused.");
#ifdef PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE
		constexpr const char* name="blk.0.ssm_out.weight";
		constexpr unsigned matrixWidth=6144,matrixRows=5120,tensorIndex=16,tensorType=8;
		const unsigned rows[3]={0,2560,5088};
#elif defined(PIONEER_NATIVE_RECURRENT_BAND_SOURCE)
		constexpr const char* name="blk.0.attn_gate.weight";
		constexpr unsigned matrixWidth=5120,matrixRows=6144,tensorIndex=3,tensorType=8;
		const unsigned rows[3]={0,3072,6112};
#elif defined(PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE)
		constexpr const char* name="blk.3.attn_k.weight";
		constexpr unsigned matrixWidth=5120,matrixRows=1024,tensorIndex=45,tensorType=8;
		const unsigned rows[3]={0,512,992};
#else
		constexpr const char* name="blk.0.ffn_gate.weight";
		constexpr unsigned matrixWidth=5120,matrixRows=17408,tensorIndex=7,tensorType=12;
		const unsigned rows[3]={0,8704,17376};
#endif
		const auto* tensor=model.Find(name);
		if(!tensor || tensor->inputs!=matrixWidth || tensor->outputs!=matrixRows || tensor->type!=tensorType || unsigned(tensor-model.Tensors().data())!=tensorIndex
			|| Meta::Samples!=3 || Meta::Width!=matrixWidth || Meta::Rows!=matrixRows || Meta::Tensor!=tensorIndex)return Fail(error,"Pinned learned matrix identity changed.");
		using Codec=PioneerBlackholeBlockFloat;
		for(unsigned sample=0;sample<3;sample++) {
			if(Meta::First(sample)!=rows[sample])return Fail(error,"Compiled sample changed.");
			std::vector<uint8_t> weights(matrixWidth/32*1104,0),inputs(matrixWidth/32*80,0);std::array<uint32_t,32> expected{};
			for(unsigned tile=0;tile<matrixWidth/32;tile++) {
				std::array<float,1024> original{},decoded{};uint8_t* packed=weights.data()+tile*1104+16;
				if(!Stable() || !PioneerQ38NativeWeightTiles::ReadFloatTile(model,tensorIndex,uint64_t(rows[sample]/32)*(matrixWidth/32)+tile,original.data(),1024)
					|| !Codec::PackTile(Codec::Format::Bfp8B,original.data(),1024,packed,1088)
					|| !Codec::UnpackTile(Codec::Format::Bfp8B,packed,1088,decoded.data(),1024) || !Stable())return Fail(error,"Fresh learned tile/source refused.");
				uint16_t input[32];
				for(unsigned c=0;c<32;c++) {
					const unsigned i=tile*32+c;input[c]=uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0));
					inputs[tile*80+16+c*2]=uint8_t(input[c]);inputs[tile*80+17+c*2]=uint8_t(input[c]>>8);
				}
				for(unsigned row=0;row<32;row++)for(unsigned half=0;half<2;half++) {
					uint16_t a[16],b[16];
					for(unsigned k=0;k<16;k++) {
						uint32_t bits=0;const float v=decoded[row*32+half*16+k];memcpy(&bits,&v,4);
						if(bits&65535)return Fail(error,"Fresh learned weight is not BF16.");
						a[k]=uint16_t(bits>>16);b[k]=input[half*16+k];
					}
					for(unsigned phase=0;phase<4;phase++)if(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[row],expected[row]))return Fail(error,"Learned reference arithmetic refused.");
				}
			}
			weights.insert(weights.end(),inputs.begin(),inputs.end());const auto payload=Cache::Hex(Cache::Hash(weights.data(),weights.size()));
			const auto* golden=Meta::Golden(sample);const char* pin=Meta::PayloadSHA(sample);
			if(!golden || !pin || payload!=pin || memcmp(expected.data(),golden,128) || !Stable())return Fail(error,"Compiled payload/reference does not match fresh authenticated original.");
		}
		if(!Stable())return Fail(error,"Original changed before learned reference publication.");
		prepared=true;error.clear();return true;
	}
};
#endif
