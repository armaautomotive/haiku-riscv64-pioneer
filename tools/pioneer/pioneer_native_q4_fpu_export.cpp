/* SPDX-License-Identifier: MIT */
// CPU-only authenticated original -> PRIVATE fixed diagnostic85 operands.
// No cache/model mutation,device access,learned products in driver or Work role.
#include "PioneerQwen38Gguf.h"
#include "PioneerModelWeightCache.h"
#include "PioneerQ38Q4FpuBlockData.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
namespace {
using Cache=PioneerModelWeightCache;
constexpr unsigned First[3]={0,8704,17376},Block[3]={0,10,19};
void Require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint32_t Word(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
FILE* NewFile(const std::string& path) {
	int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
	Require(fd>=0,"Private output exists or cannot be created.");
	FILE* f=fdopen(fd,"wb");if(!f){close(fd);throw std::runtime_error("Private stream failed.");}return f;
}
void Finish(FILE* f) {
	bool okay=!ferror(f) && !fflush(f) && !fsync(fileno(f));bool closed=!fclose(f);
	Require(okay && closed,"Private output incomplete;DO NOT COMPILE.");
}
void Bytes(const std::string& path,const void* p,size_t n) {
	FILE* f=NewFile(path);Require(fwrite(p,1,n,f)==n,"Private output short write;DO NOT COMPILE.");Finish(f);
}
void Array(FILE* f,const char* name,const uint8_t* bytes,unsigned n) {
	Require(!(n&3),"Unaligned generated array.");fprintf(f," static const uint32_t %s[%u]={\n",name,n/4);
	for(unsigned i=0;i<n;i+=4)fprintf(f,"%s0x%08x%s",i%32?"":"  ",Word(bytes+i),i+4==n?"\n":i%32==28?",\n":",");
	fputs(" };\n",f);
}
struct Sample {
	std::array<uint8_t,4608> original{};
	std::array<uint8_t,6144> prepared{};
	std::array<uint8_t,512> input{};
	std::array<uint8_t,128> expected{};
};
int Run(int argc,char** argv) {
	if(argc!=3){fputs("Use ORIGINAL_GGUF NEW_EMPTY_PRIVATE_DIRECTORY;CPU_ONLY.\n",stderr);return 2;}
	struct stat info{};Require(!lstat(argv[2],&info) && S_ISDIR(info.st_mode) && info.st_uid==geteuid()
		&& (info.st_mode&07777)==0700,"Output must be owned private directory,not a symlink.");
	std::unique_ptr<FILE,decltype(&fclose)> f(fopen(argv[1],"rb"),fclose);Require(bool(f),"Original model unavailable.");
	PioneerQwen38Gguf model;Require(model.Open(f.get()),"Original851-tensor model layout refused.");
	uint64_t epoch=model.Epoch();Cache::Digest sha{};std::string error;
	puts("AUTHENTICATING_COMPLETE_SAME_OPEN_ORIGINAL;CPU_ONLY_NO_DEVICE");fflush(stdout);
	Require(Cache::HashFile(fileno(f.get()),PioneerQwen38Gguf::FullBytes,sha,error)
		&& Cache::Hex(sha)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"
		&& model.Stable() && model.Epoch()==epoch,"Full original SHA/epoch refused.");
	const auto* tensor=model.Find("blk.0.ffn_gate.weight");Require(tensor && tensor-model.Tensors().data()==7
		&& tensor->type==12 && tensor->inputs==5120 && tensor->outputs==17408,"Pinned original Q4 matrix refused.");
	std::array<Sample,3> samples;
	for(unsigned s=0;s<3;s++) {
		auto& p=samples[s];
		for(unsigned row=0;row<32;row++)Require(model.ReadPacked(7,uint64_t(First[s]+row)*20*144+Block[s]*144,
			144,p.original.data()+row*144,144),"Original selected block read refused.");
		Require(PioneerQ38Q4FpuBlockData::Prepare(p.original.data(),4608,p.prepared.data(),6144),"Q4 preparation refused.");
		for(unsigned c=0;c<256;c++) {
			uint16_t bits=uint16_t(0x3e80+(c*19+s*11)%384+((c&3)==2?0x8000:0));
			p.input[c*2]=uint8_t(bits);p.input[c*2+1]=uint8_t(bits>>8);
		}
		// Validation-only original-decode/BF16-RNE/rounded FPU oracle. Never
		// compiled into the driver,never silently substitutes Work arithmetic.
		for(unsigned row=0;row<32;row++) {
			float weights[256];Require(PioneerDecodeQ4K(p.original.data()+row*144,weights),"Original decode refused.");uint32_t total=0;
			for(unsigned first=0;first<256;first+=16) {
				uint16_t a[16],b[16];for(unsigned k=0;k<16;k++) {
					uint32_t bits;memcpy(&bits,&weights[first+k],4);
					Require((bits&0x7f800000)!=0x7f800000 && (!(bits&0x7fffffff) || (bits&0x7f800000)),"Non-finite/denormal reference refused.");
					uint32_t high=bits>>16,low=bits&65535;high+=low>32768 || (low==32768 && (high&1));a[k]=uint16_t(high);
					b[k]=uint16_t(p.input[(first+k)*2]|(unsigned(p.input[(first+k)*2+1])<<8));
				}
				for(unsigned phase=0;phase<4;phase++)Require(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,total,total),"Rounded reference refused.");
			}
			for(unsigned byte=0;byte<4;byte++)p.expected[row*4+byte]=uint8_t(total>>(8*byte));
		}
		Require(model.Stable() && model.Epoch()==epoch,"Original epoch changed during preparation.");
	}
	const std::string directory=argv[2];
	FILE* data=NewFile(directory+"/PioneerNativeQ4FpuData.h");
	fputs("/* PRIVATE GENERATED learned weights;DO NOT COMMIT OR REDISTRIBUTE. */\n#ifndef PIONEER_NATIVE_Q4_FPU_DATA_H\n#define PIONEER_NATIVE_Q4_FPU_DATA_H\n#include <stdint.h>\n#include \"PioneerNativeQ4FpuLayout.h\"\nstruct PioneerNativeQ4FpuData {\n static constexpr unsigned Version=1,Width=256,Rows=32,Samples=3,Role=85;\n static uint32_t Word(unsigned sample,unsigned address){\n if(sample>=3 || (address&3))return 0;\n",data);
	for(unsigned s=0;s<3;s++){char name[16];snprintf(name,sizeof(name),"p%u",s);Array(data,name,samples[s].prepared.data(),6144);snprintf(name,sizeof(name),"i%u",s);Array(data,name,samples[s].input.data(),512);}
	fputs(" unsigned i=PioneerNativeQ4FpuLayout::PreparedIndex(address);\n if(i!=UINT32_MAX)return sample==0?p0[i/4]:sample==1?p1[i/4]:p2[i/4];\n i=PioneerNativeQ4FpuLayout::InputIndex(address);\n if(i!=UINT32_MAX)return sample==0?i0[i/4]:sample==1?i1[i/4]:i2[i/4];\n return 0;}\n};\n#endif\n",data);Finish(data);
	FILE* meta=NewFile(directory+"/PioneerNativeQ4FpuMetadata.h");
	fputs("/* PRIVATE GENERATED diagnostic predictions,NOT Work/model quality. */\n#ifndef PIONEER_NATIVE_Q4_FPU_METADATA_H\n#define PIONEER_NATIVE_Q4_FPU_METADATA_H\n#include <stdint.h>\nstruct PioneerNativeQ4FpuMetadata {\n static constexpr unsigned Tensor=7,Width=256,MatrixWidth=5120,Rows=32,Samples=3;\n static unsigned First(unsigned s){const unsigned v[3]={0,8704,17376};return s<3?v[s]:~0U;}\n static unsigned Block(unsigned s){const unsigned v[3]={0,10,19};return s<3?v[s]:~0U;}\n static const uint32_t* Golden(unsigned s){\n",meta);
	for(unsigned s=0;s<3;s++){char name[16];snprintf(name,sizeof(name),"g%u",s);Array(meta,name,samples[s].expected.data(),128);}
	fputs(" return s==0?g0:s==1?g1:s==2?g2:nullptr;}\n};\n#endif\n",meta);Finish(meta);
	for(unsigned s=0;s<3;s++) {
		const auto& p=samples[s];const std::string prefix=directory+"/sample"+std::to_string(s);
		Bytes(prefix+".original.bin",p.original.data(),4608);Bytes(prefix+".prepared.bin",p.prepared.data(),6144);
		Bytes(prefix+".input.bin",p.input.data(),512);Bytes(prefix+".expected.bin",p.expected.data(),128);
		printf("PRIVATE_Q4_SAMPLE%u tensor7 first%u kblock%u original_sha%s prepared_sha%s input_sha%s prediction_sha%s;NO_LEARNED_BYTES_PRINTED\n",
			s,First[s],Block[s],Cache::Hex(Cache::Hash(p.original.data(),4608)).c_str(),Cache::Hex(Cache::Hash(p.prepared.data(),6144)).c_str(),
			Cache::Hex(Cache::Hash(p.input.data(),512)).c_str(),Cache::Hex(Cache::Hash(p.expected.data(),128)).c_str());
	}
	Require(model.Stable() && model.Epoch()==epoch,"Original changed during export;DO NOT COMPILE partial outputs.");
	puts("PASS_PRIVATE_Q4_EXPORT samples3 K256 outputs32 each,original-authenticated/unchanged;6144B recipe1,not BFP8 cache;NO DEVICE/HARDWARE/MODEL_QUALITY/SPEED_PROOF");return 0;
}
}
int main(int argc,char** argv){try{return Run(argc,argv);}catch(const std::exception& e){fprintf(stderr,"Q4_EXPORT_REFUSED:%s\n",e.what());return 1;}}
