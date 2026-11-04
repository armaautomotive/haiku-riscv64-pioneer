/* SPDX-License-Identifier: MIT */
// CPU-only authenticated17-band preparation. PRIVATE weights/headers remain
// outside Git. NO device/installed cache/Work or source mutation capability.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(__FAST_MATH__)
#error Diagnostic87 exporter is not a kernel/Work/CPU-validation capability
#endif
#include "PioneerQwen38Gguf.h"
#include "PioneerModelWeightCache.h"
#include "PioneerQ38Q6FpuBlockData.h"
#include "PioneerQ38Q8FpuBlockData.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include "PioneerNativeMixedFpuStreamLayout.h"
#include <array>
#include <cfenv>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
namespace {
using Cache=PioneerModelWeightCache;using V=PioneerNativeMixedFpuStreamVariant;using L=PioneerNativeMixedFpuStreamLayout;
void Require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
uint32_t Word(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
bool Same(const struct stat& a,const struct stat& b) {
	return a.st_dev==b.st_dev && a.st_ino==b.st_ino && a.st_size==b.st_size && a.st_uid==b.st_uid
		&& a.st_gid==b.st_gid && a.st_mode==b.st_mode && a.st_nlink==b.st_nlink
#ifdef __APPLE__
		&& a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec && a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec
		&& a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
		&& a.st_mtim.tv_sec==b.st_mtim.tv_sec && a.st_mtim.tv_nsec==b.st_mtim.tv_nsec
		&& a.st_ctim.tv_sec==b.st_ctim.tv_sec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
FILE* NewFile(const std::string& path) {
	int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);Require(fd>=0,"Private output exists or cannot be created.");
	FILE* f=fdopen(fd,"wb");if(!f){close(fd);throw std::runtime_error("Private stream failed.");}return f;
}
void Finish(FILE* f) {
	bool okay=!ferror(f) && !fflush(f) && !fsync(fileno(f)),closed=!fclose(f);
	Require(okay && closed,"Private output incomplete;DO NOT COMPILE.");
}
void Bytes(const std::string& path,const void* p,size_t n){FILE* f=NewFile(path);Require(fwrite(p,1,n,f)==n,"Private short write;DO NOT COMPILE.");Finish(f);}
void Array(FILE* f,const char* name,const uint8_t* bytes,unsigned n) {
	Require(!(n&3),"Unaligned generated array.");fprintf(f," static const uint32_t %s[%u]={\n",name,n/4);
	for(unsigned i=0;i<n;i+=4)fprintf(f,"%s0x%08x%s",i%32?"":"  ",Word(bytes+i),i+4==n?"\n":i%32==28?",\n":",");
	fputs(" };\n",f);
}
struct Sample {
	unsigned tensor=0,type=0,width=0,first=0,realRows=0;
	std::vector<uint8_t> original,prepared,input;std::array<uint8_t,128> expected{};
};
const char* Name(unsigned s){return s<3?"blk.3.attn_output.weight":s<6?"output.weight":s<9?"blk.0.attn_qkv.weight":s<12?"blk.0.ssm_out.weight":s<15?"blk.3.attn_q.weight":"blk.0.ssm_alpha.weight";}
template<class F> void Function(FILE* file,const char* name,unsigned missing,F value) {
	fprintf(file," static unsigned %s(unsigned s){static const unsigned t[17]={",name);
	for(unsigned s=0;s<17;s++)fprintf(file,"%s%u",s?",":"",value(s));
	fprintf(file,"};return s<17?t[s]:%uU;}\n",missing);
}
int Run(int argc,char** argv) {
	if(argc!=3){fputs("Use ORIGINAL_GGUF NEW_EMPTY_PRIVATE_DIRECTORY;CPU_ONLY.\n",stderr);return 2;}
	Require(std::fegetround()==FE_TONEAREST,"RNE host domain required.");
	struct stat dir{};Require(!lstat(argv[2],&dir) && S_ISDIR(dir.st_mode) && dir.st_uid==geteuid() && (dir.st_mode&07777)==0700,"Output must be owned private directory,not a symlink.");
	DIR* directory=opendir(argv[2]);Require(directory,"Cannot inspect private output.");
	bool empty=true;while(auto* p=readdir(directory))if(strcmp(p->d_name,".") && strcmp(p->d_name,".."))empty=false;
	Require(!closedir(directory) && empty,"Private output is not empty;never overwrite.");
	int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);Require(fd>=0,"Original model unavailable.");
	struct stat original{},now{};Require(!fstat(fd,&original) && S_ISREG(original.st_mode) && original.st_uid==geteuid()
		&& original.st_nlink==1 && !(original.st_mode&0022) && uint64_t(original.st_size)==PioneerQwen38Gguf::FullBytes,"Original identity refused.");
	FILE* opened=fdopen(fd,"rb");if(!opened){close(fd);throw std::runtime_error("Original stream unavailable.");}
	std::unique_ptr<FILE,decltype(&fclose)> f(opened,fclose);PioneerQwen38Gguf model;
	Require(model.Open(f.get()),"Original851-tensor model layout refused.");uint64_t epoch=model.Epoch();
	auto current=[&](){return !fstat(fd,&now) && Same(original,now) && model.Stable() && model.Epoch()==epoch;};
	Cache::Digest sha{};std::string error;puts("AUTHENTICATING_COMPLETE_SAME_OPEN_ORIGINAL;CPU_ONLY_NO_DEVICE");fflush(stdout);
	Require(Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error) && Cache::Hex(sha)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747" && current(),"Full original SHA/identity/epoch refused.");
	std::array<Sample,17> samples;
	for(unsigned s=0;s<17;s++) {
		auto& p=samples[s];p.tensor=V::Tensor(s);p.type=V::Type(s);p.width=V::Width(s);p.first=V::First(s);p.realRows=V::RealRows(s);
		const unsigned blocks=p.width/256,oneRow=p.type==14?210:272,originalBytes=p.realRows*oneRow,preparedBytes=V::PreparedBytes(s);
		const auto& t=model.Tensors()[p.tensor];Require(t.name==Name(s) && t.type==p.type && t.rank==2 && t.inputs==p.width
			&& t.outputs==V::Outputs(s) && p.first+p.realRows<=t.outputs,"Pinned original Q6/Q8 matrix refused.");
		p.original.resize(blocks*originalBytes);p.prepared.resize(blocks*preparedBytes);p.input.resize(blocks*512);uint32_t total[32]={};
		for(unsigned block=0;block<blocks;block++) {
			auto* raw=p.original.data()+block*originalBytes;
			for(unsigned row=0;row<p.realRows;row++)Require(model.ReadPacked(p.tensor,(uint64_t(p.first+row)*blocks+block)*oneRow,oneRow,raw+row*oneRow,oneRow),"Original full-K band read refused.");
			bool prepared=p.type==14?PioneerQ38Q6FpuBlockData::Prepare(raw,originalBytes,p.prepared.data()+block*preparedBytes,preparedBytes)
				:PioneerQ38Q8FpuBlockData::PrepareRows(raw,originalBytes,p.realRows,p.prepared.data()+block*preparedBytes,preparedBytes);
			Require(prepared,"Q6/Q8 preparation refused.");uint16_t input[256];
			for(unsigned c=0;c<256;c++){input[c]=uint16_t(0x3e80+(c*19+block*11)%384+((c&3)==2?0x8000:0));p.input[block*512+c*2]=uint8_t(input[c]);p.input[block*512+c*2+1]=uint8_t(input[c]>>8);}
			// Separate diagnostic oracle,NEVER compiled into driver or Work.
			for(unsigned row=0;row<32;row++) {
				float weights[256]={};
				if(row<p.realRows) {
					if(p.type==14)Require(PioneerDecodeQ6K(raw+row*210,weights),"Original Q6 decode refused.");
					else for(unsigned g=0;g<8;g++)Require(PioneerDecodeQ8_0(raw+row*272+g*34,weights+g*32),"Original Q8 decode refused.");
				}
				for(unsigned first=0;first<256;first+=16) {
					uint16_t a[16],b[16];for(unsigned k=0;k<16;k++) {
						uint32_t bits;memcpy(&bits,&weights[first+k],4);
						Require((bits&0x7f800000)!=0x7f800000 && (!(bits&0x7fffffff) || (bits&0x7f800000)),"Non-finite/denormal reference refused.");
						uint32_t high=bits>>16,low=bits&65535;high+=low>32768 || (low==32768 && (high&1));a[k]=uint16_t(high);b[k]=input[first+k];
					}
					for(unsigned phase=0;phase<4;phase++)Require(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,total[row],total[row]),"Rounded reference refused.");
				}
			}
			Require(current(),"Original nanosecond identity/epoch changed.");
		}
		for(unsigned r=0;r<32;r++)for(unsigned b=0;b<4;b++)p.expected[r*4+b]=uint8_t(total[r]>>(8*b));
	}
	Require(current(),"Original changed before private publication.");const std::string path=argv[2];
	FILE* data=NewFile(path+"/PioneerNativeMixedFpuStreamData.h");
	fputs("/* PRIVATE GENERATED learned weights;DO NOT COMMIT OR REDISTRIBUTE. */\n#ifndef PIONEER_NATIVE_MIXED_FPU_STREAM_DATA_H\n#define PIONEER_NATIVE_MIXED_FPU_STREAM_DATA_H\n#include <stdint.h>\nstruct PioneerNativeMixedFpuStreamData {\n static constexpr unsigned Version=1,Rows=32,Samples=17,Role=87,Formats=2;\n",data);
	Function(data,"Width",0,[&](unsigned s){return samples[s].width;});Function(data,"Type",0,[&](unsigned s){return samples[s].type;});
	Function(data,"RealRows",0,[&](unsigned s){return samples[s].realRows;});Function(data,"Tensor",UINT32_MAX,[&](unsigned s){return samples[s].tensor;});
	Function(data,"First",UINT32_MAX,[&](unsigned s){return samples[s].first;});Function(data,"WeightWords",0,[&](unsigned s){return unsigned(samples[s].prepared.size()/4);});
	Function(data,"Words",0,[&](unsigned s){return unsigned((samples[s].prepared.size()+samples[s].input.size())/4);});
	fputs(" static uint32_t Word(unsigned sample,unsigned index){if(sample>=17 || index>=Words(sample))return 0;\n",data);
	for(unsigned s=0;s<17;s++){char name[16];snprintf(name,sizeof(name),"p%u",s);Array(data,name,samples[s].prepared.data(),unsigned(samples[s].prepared.size()));snprintf(name,sizeof(name),"i%u",s);Array(data,name,samples[s].input.data(),unsigned(samples[s].input.size()));}
	fputs(" if(index<WeightWords(sample)){switch(sample){\n",data);for(unsigned s=0;s<17;s++)fprintf(data," case %u:return p%u[index];\n",s,s);
	fputs(" }}else{index-=WeightWords(sample);switch(sample){\n",data);for(unsigned s=0;s<17;s++)fprintf(data," case %u:return i%u[index];\n",s,s);
	fputs(" }}return 0;}\n};\n#endif\n",data);Finish(data);
	FILE* meta=NewFile(path+"/PioneerNativeMixedFpuStreamMetadata.h");
	fputs("/* PRIVATE GENERATED diagnostic predictions,NOT Work/model quality. */\n#ifndef PIONEER_NATIVE_MIXED_FPU_STREAM_METADATA_H\n#define PIONEER_NATIVE_MIXED_FPU_STREAM_METADATA_H\n#include <stdint.h>\nstruct PioneerNativeMixedFpuStreamMetadata {\n static constexpr unsigned Rows=32,Samples=17,Role=87,Formats=2;\n",meta);
	Function(meta,"Tensor",UINT32_MAX,[&](unsigned s){return samples[s].tensor;});Function(meta,"Width",0,[&](unsigned s){return samples[s].width;});
	Function(meta,"Type",0,[&](unsigned s){return samples[s].type;});Function(meta,"RealRows",0,[&](unsigned s){return samples[s].realRows;});
	Function(meta,"First",UINT32_MAX,[&](unsigned s){return samples[s].first;});fputs(" static const uint32_t* Golden(unsigned s){\n",meta);
	for(unsigned s=0;s<17;s++){char name[16];snprintf(name,sizeof(name),"g%u",s);Array(meta,name,samples[s].expected.data(),128);}
	fputs(" switch(s){\n",meta);for(unsigned s=0;s<17;s++)fprintf(meta," case %u:return g%u;\n",s,s);fputs(" }return nullptr;}\n};\n#endif\n",meta);Finish(meta);
	for(unsigned s=0;s<17;s++) {
		const auto& p=samples[s];std::string prefix=path+"/sample"+std::to_string(s);
		Bytes(prefix+".original.bin",p.original.data(),p.original.size());Bytes(prefix+".prepared.bin",p.prepared.data(),p.prepared.size());
		Bytes(prefix+".input.bin",p.input.data(),p.input.size());Bytes(prefix+".expected.bin",p.expected.data(),128);
		printf("PRIVATE_MIXED_STREAM_SAMPLE%u tensor%u type%u first%u rows%u K%u original_sha%s prepared_sha%s input_sha%s prediction_sha%s;NO_WEIGHT_BYTES_PRINTED\n",s,p.tensor,p.type,p.first,p.realRows,p.width,
			Cache::Hex(Cache::Hash(p.original.data(),p.original.size())).c_str(),Cache::Hex(Cache::Hash(p.prepared.data(),p.prepared.size())).c_str(),Cache::Hex(Cache::Hash(p.input.data(),p.input.size())).c_str(),Cache::Hex(Cache::Hash(p.expected.data(),128)).c_str());
	}
	Require(current(),"Original changed during export;DO NOT COMPILE partial outputs.");
	puts("PASS_PRIVATE_MIXED_STREAM_EXPORT samples17 Q6/Q8 fullK5120/6144 rows32/padded16,original-authenticated/unchanged;NEW recipes,not installed caches;NO DEVICE/HARDWARE/QUALITY/SPEED_PROOF");return 0;
}
}
int main(int argc,char** argv){try{return Run(argc,argv);}catch(const std::exception& e){fprintf(stderr,"MIXED_STREAM_EXPORT_REFUSED:%s\n",e.what());return 1;}}
