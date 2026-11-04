/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_Q4_FPU_SOURCE_H
#define PIONEER_NATIVE_Q4_FPU_SOURCE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Q4 FPU source reference is a separate diagnostic,never driver/Work/validation
#endif
#include "PioneerQwen38Gguf.h"
#include "PioneerModelWeightCache.h"
#include "PioneerQ38Q4FpuBlockData.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include "PioneerNativeQ4FpuData.h" // PRIVATE fixed compiled operands
#include "PioneerNativeQ4FpuMetadata.h" // PRIVATE independent predictions
#include <array>
#include <cfenv>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
// Diagnostic-only fresh original reference and exact compiled-data binding.
// No GPU/cache calls or Work fallback. Source stays on this SAME open FD.
class PioneerNativeQ4FpuSource {
	using Cache=PioneerModelWeightCache;using Meta=PioneerNativeQ4FpuMetadata;
	using L=PioneerNativeQ4FpuLayout;using Data=PioneerNativeQ4FpuData;
	std::unique_ptr<FILE,decltype(&fclose)> file{nullptr,&fclose};
	PioneerQwen38Gguf model;uint64_t epoch=0;bool attempted=false,prepared=false;
	static bool Fail(std::string& error,const char* text){error=text;return false;}
	bool Stable()const{return file && model.Ready() && model.Epoch()==epoch && model.Stable();}
	static uint32_t Word(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
public:
	bool Ready()const{return prepared && Stable();}
	bool Prepare(const char* path,std::string& error) {
		static_assert(Data::Version==1 && Data::Role==85 && Data::Samples==3 && Data::Width==256 && Data::Rows==32,"exact diagnostic85 private recipe");
		if(attempted)return Fail(error,"Q4 source preparation is one-shot.");
		attempted=true;
		if(!path || std::fegetround()!=FE_TONEAREST)return Fail(error,"Original path/default rounding required.");
		int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return Fail(error,"Original read-only open refused.");
		struct stat info{};
		if(fstat(fd,&info) || !S_ISREG(info.st_mode) || info.st_nlink!=1 || info.st_uid!=geteuid() || (info.st_mode&0022)
			|| uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return Fail(error,"Original identity/permission/size refused.");}
		FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return Fail(error,"Original stream refused.");}file.reset(stream);
		if(!model.Open(file.get()))return Fail(error,"Complete original851-tensor layout refused.");
		epoch=model.Epoch();Cache::Digest sha{};
		if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error) || !Stable()
			|| Cache::Hex(sha)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return Fail(error,"Original full SHA/stability refused.");
		const auto* tensor=model.Find("blk.0.ffn_gate.weight");
		if(!tensor || tensor-model.Tensors().data()!=7 || tensor->type!=12 || tensor->inputs!=5120 || tensor->outputs!=17408
			|| Meta::Tensor!=7 || Meta::Samples!=3 || Meta::Width!=256 || Meta::MatrixWidth!=5120 || Meta::Rows!=32)return Fail(error,"Pinned original matrix changed.");
		const unsigned rows[3]={0,8704,17376},blocks[3]={0,10,19};
		for(unsigned sample=0;sample<3;sample++) {
			if(Meta::First(sample)!=rows[sample] || Meta::Block(sample)!=blocks[sample])return Fail(error,"Compiled Q4 selection changed.");
			std::array<uint8_t,4608> original{};std::array<uint8_t,6144> packed{};std::array<uint8_t,512> input{};
			std::array<uint32_t,32> expected{};
			for(unsigned row=0;row<32;row++)if(!model.ReadPacked(7,uint64_t(rows[sample]+row)*20*144+blocks[sample]*144,
				144,original.data()+row*144,144))return Fail(error,"Fresh original Q4 block read refused.");
			if(!Stable() || !PioneerQ38Q4FpuBlockData::Prepare(original.data(),4608,packed.data(),6144))return Fail(error,"Fresh Q4 preparation refused.");
			for(unsigned c=0;c<256;c++){uint16_t bits=uint16_t(0x3e80+(c*19+sample*11)%384+((c&3)==2?0x8000:0));input[c*2]=uint8_t(bits);input[c*2+1]=uint8_t(bits>>8);}
			for(unsigned a=L::DataBegin;a<L::DataEnd;a+=4) {
				unsigned i=L::PreparedIndex(a);uint32_t word=0;
				if(i!=UINT32_MAX)word=Word(packed.data()+i);
				else{i=L::InputIndex(a);if(i!=UINT32_MAX)word=Word(input.data()+i);}
				if(Data::Word(sample,a)!=word)return Fail(error,"Compiled Q4 payload/header/gap/input differs from original.");
			}
			for(unsigned row=0;row<32;row++) {
				float weights[256];if(!PioneerDecodeQ4K(original.data()+row*144,weights))return Fail(error,"Original Q4 decode refused.");
				for(unsigned first=0;first<256;first+=16) {
					uint16_t a[16],b[16];for(unsigned k=0;k<16;k++) {
						uint32_t bits;memcpy(&bits,&weights[first+k],4);
						if((bits&0x7f800000)==0x7f800000 || ((bits&0x7fffffff) && !(bits&0x7f800000)))return Fail(error,"Reference non-finite/denormal refused.");
						unsigned high=bits>>16,low=bits&65535;high+=low>32768 || (low==32768 && (high&1));a[k]=uint16_t(high);
						b[k]=uint16_t(input[(first+k)*2]|(unsigned(input[(first+k)*2+1])<<8));
					}
					for(unsigned phase=0;phase<4;phase++)if(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[row],expected[row]))return Fail(error,"Fresh rounded reference refused.");
				}
			}
			if(!Meta::Golden(sample) || memcmp(expected.data(),Meta::Golden(sample),128) || !Stable())return Fail(error,"Compiled prediction differs from original/stability.");
		}
		if(!Stable())return Fail(error,"Original changed before Q4 publication.");
		prepared=true;error.clear();return true;
	}
};
#endif
