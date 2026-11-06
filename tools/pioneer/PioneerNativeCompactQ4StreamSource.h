/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_COMPACT_Q4_STREAM_SOURCE_H
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_SOURCE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Compact Q4 full-K source reference is a separate diagnostic,never driver/Work/validation
#endif
#include "PioneerQwen38Gguf.h"
#include "PioneerModelWeightCache.h"
#include "PioneerQ38Q4FpuCompactData.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include "PioneerNativeCompactQ4StreamData.h" // PRIVATE fixed compiled operands
#include "PioneerNativeCompactQ4StreamMetadata.h" // PRIVATE independent predictions
#ifdef PIONEER_NATIVE_FACTORED_Q4_COMPARE
#include "PioneerQ38FactoredQ4Reference.h"
#endif
#include <array>
#include <cfenv>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
// Diagnostic-only fresh original reference and exact compiled-data binding.
// No GPU/cache calls or Work fallback. Source stays on this SAME open FD.
class PioneerNativeCompactQ4StreamSource {
	using Cache=PioneerModelWeightCache;using Meta=PioneerNativeCompactQ4StreamMetadata;
	using Data=PioneerNativeCompactQ4StreamData;
	std::unique_ptr<FILE,decltype(&fclose)> file{nullptr,&fclose};
	struct stat identity={};
	PioneerQwen38Gguf model;uint64_t epoch=0;bool attempted=false,prepared=false;
#ifdef PIONEER_NATIVE_FACTORED_Q4_COMPARE
	std::array<std::array<uint32_t,32>,9> factored{};
#endif
	static bool Fail(std::string& error,const char* text){error=text;return false;}
	static bool Same(const struct stat& a,const struct stat& b){
		if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_uid!=b.st_uid || a.st_gid!=b.st_gid || a.st_mode!=b.st_mode || a.st_nlink!=b.st_nlink || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
		return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
		return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
	}
	bool Stable()const{struct stat now={};return file && !fstat(fileno(file.get()),&now) && Same(identity,now) && model.Ready() && model.Epoch()==epoch && model.Stable();}
	static uint32_t Word(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
public:
	bool Ready()const{return prepared && Stable();}
#ifdef PIONEER_NATIVE_FACTORED_Q4_COMPARE
	const uint32_t* FactoredGolden(unsigned sample)const{return sample<9 && Ready()?factored[sample].data():nullptr;}
#endif
	bool Prepare(const char* path,std::string& error) {
		static_assert(Data::Version==1 && Data::Role==104 && Data::PreparedBytes==4608 && Data::Samples==9 && Data::Rows==32 && Meta::Samples==9 && Meta::Rows==32,"exact diagnostic104 private recipe");
		if(attempted)return Fail(error,"Q4 source preparation is one-shot.");
		attempted=true;
		if(!path || std::fegetround()!=FE_TONEAREST)return Fail(error,"Original path/default rounding required.");
		int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return Fail(error,"Original read-only open refused.");
		struct stat info{};
		if(fstat(fd,&info) || !S_ISREG(info.st_mode) || info.st_nlink!=1 || info.st_uid!=geteuid() || (info.st_mode&0022)
			|| uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return Fail(error,"Original identity/permission/size refused.");}
		FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return Fail(error,"Original stream refused.");}file.reset(stream);identity=info;
		if(!model.Open(file.get()))return Fail(error,"Complete original851-tensor layout refused.");
		epoch=model.Epoch();Cache::Digest sha{};
		if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error) || !Stable()
			|| Cache::Hex(sha)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return Fail(error,"Original full SHA/stability refused.");
		for(unsigned sample=0;sample<9;sample++) {
			const unsigned width=sample<6?5120:17408,tensor=sample<3?7:sample<6?8:6,blocks=width/256;
			const unsigned first=sample%3==0?0:sample%3==1?(sample<6?8704:2560):(sample<6?17376:5088);
			const char* name=sample<3?"blk.0.ffn_gate.weight":sample<6?"blk.0.ffn_up.weight":"blk.0.ffn_down.weight";
			const auto* t=model.Find(name);
			if(!t || unsigned(t-model.Tensors().data())!=tensor || t->type!=12 || t->inputs!=width || t->outputs!=(sample<6?17408:5120)
				|| Meta::Tensor(sample)!=tensor || Meta::Width(sample)!=width || Meta::First(sample)!=first
				|| Data::Width(sample)!=width || Data::Words(sample)!=blocks*1280)return Fail(error,"Pinned original full-K selection changed.");
			std::array<uint32_t,32> expected{};
			for(unsigned block=0;block<blocks;block++) {
				std::array<uint8_t,4608> original{};std::array<uint8_t,4608> packed{};std::array<uint8_t,512> input{};
				for(unsigned row=0;row<32;row++)if(!model.ReadPacked(tensor,(uint64_t(first+row)*blocks+block)*144,144,original.data()+row*144,144))
					return Fail(error,"Fresh original full-K Q4 read refused.");
				if(!Stable() || !PioneerQ38Q4FpuCompactData::Prepare(original.data(),4608,packed.data(),4608))return Fail(error,"Fresh Q4 preparation refused.");
				for(unsigned c=0;c<256;c++){uint16_t bits=uint16_t(0x3e80+(c*19+block*11)%384+((c&3)==2?0x8000:0));input[c*2]=uint8_t(bits);input[c*2+1]=uint8_t(bits>>8);}
				for(unsigned i=0;i<1152;i++)if(Data::Word(sample,block*1152+i)!=Word(packed.data()+i*4))return Fail(error,"Compiled Q4 payload/metadata differs from original.");
				for(unsigned i=0;i<128;i++)if(Data::Word(sample,blocks*1152+block*128+i)!=Word(input.data()+i*4))return Fail(error,"Compiled Q4 input differs from original.");
#ifdef PIONEER_NATIVE_FACTORED_Q4_COMPARE
				uint16_t bf16[256];for(unsigned c=0;c<256;c++)bf16[c]=uint16_t(input[c*2])|uint16_t(input[c*2+1])<<8;
				if(!PioneerQ38FactoredQ4Reference::Block(original.data(),bf16,factored[sample].data()))return Fail(error,"Fresh factored Q4 reference refused.");
#endif
				for(unsigned row=0;row<32;row++) {
					float weights[256];if(!PioneerDecodeQ4K(original.data()+row*144,weights))return Fail(error,"Original Q4 decode refused.");
					for(unsigned first=0;first<256;first+=16) {
						uint16_t a[16],b[16];for(unsigned k=0;k<16;k++) {
							uint32_t bits;memcpy(&bits,&weights[first+k],4);
							if((bits&0x7f800000)==0x7f800000 || ((bits&0x7fffffff) && !(bits&0x7f800000)))return Fail(error,"Reference non-finite/denormal refused.");
							unsigned high=bits>>16,low=bits&65535;high+=low>32768 || (low==32768 && (high&1));a[k]=uint16_t(high);
							b[k]=uint16_t(input[(first+k)*2]|(unsigned(input[(first+k)*2+1])<<8));
						}
						for(unsigned phase=0;phase<4;phase++)if(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[row],expected[row]))return Fail(error,"Fresh full-K rounded reference refused.");
					}
				}
			}
			if(!Meta::Golden(sample) || memcmp(expected.data(),Meta::Golden(sample),128) || !Stable())return Fail(error,"Compiled full-K prediction differs from original/stability.");
		}
		if(!Stable())return Fail(error,"Original changed before Q4 publication.");
		prepared=true;error.clear();return true;
	}
};
#endif
