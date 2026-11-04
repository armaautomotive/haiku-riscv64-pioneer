/* SPDX-License-Identifier: MIT */
// CPU-only source/cache verification+PRIVATE generated diagnostic data.
// Never opens a card,changes a model/cache or installs anything.
#include "PioneerQ38NativeBfpBandSource.h"
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
using Packet=PioneerQ38NativeBfpBandData::Packet;
constexpr unsigned First[3]={0,8704,17376};
void Require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void Require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
uint32_t Word(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
FILE* NewFile(const std::string& path) {
	const int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
	Require(fd>=0,"Private output already exists or cannot be created.");
	FILE* file=fdopen(fd,"wb");if(!file){close(fd);throw std::runtime_error("Cannot open private output stream.");}return file;
}
void Finish(FILE* file) {
	const bool ok=!ferror(file) && !fflush(file) && !fsync(fileno(file));
	const bool closed=!fclose(file);Require(ok && closed,"Generated data delivery failed;DO NOT COMPILE.");
}
std::array<uint32_t,32> FreshReference(PioneerQwen38Gguf& model,unsigned tensor,unsigned first,
	const std::vector<uint16_t>& input,const Packet& packet) {
	std::array<uint32_t,32> output{};
	using Codec=PioneerBlackholeBlockFloat;
	for(unsigned tile=0;tile<160;tile++) {
		std::array<float,1024> original{},decoded{};std::array<uint8_t,1088> packed{};
		Require(PioneerQ38NativeWeightTiles::ReadFloatTile(model,tensor,uint64_t(first/32)*160+tile,original.data(),1024),"Fresh original tile read failed.");
		Require(Codec::PackTile(Codec::Format::Bfp8B,original.data(),1024,packed.data(),1088)
			&& !memcmp(packet.weights.data()+tile*1104+16,packed.data(),1088),"Existing cache differs from fresh authenticated original conversion.");
		Require(Codec::UnpackTile(Codec::Format::Bfp8B,packed.data(),1088,decoded.data(),1024),"Fresh BFP8 decode refused.");
		for(unsigned row=0;row<32;row++)for(unsigned half=0;half<2;half++) {
			uint16_t a[16],b[16];
			for(unsigned k=0;k<16;k++) {
				uint32_t bits=0;const float weight=decoded[row*32+half*16+k];memcpy(&bits,&weight,4);
				Require(!(bits&65535),"Decoded BFP8 weight not exactly BF16.");a[k]=uint16_t(bits>>16);b[k]=input[tile*32+half*16+k];
			}
			for(unsigned phase=0;phase<4;phase++)Require(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,output[row],output[row]),"Fresh reference arithmetic refused.");
		}
	}
	return output;
}
void Array(FILE* file,const char* name,const std::vector<uint8_t>& bytes) {
	Require(!(bytes.size()%4),"Word array needs aligned bytes.");
	fprintf(file," static const uint32_t %s[%zu]={\n",name,bytes.size()/4);
	for(size_t i=0;i<bytes.size();i+=4)fprintf(file,"%s0x%08x%s",(i%32)?"":"  ",Word(bytes.data()+i),i+4==bytes.size()?"\n":(i%32==28)?",\n":",");
	fputs(" };\n",file);
}
int Run(int argc,char** argv) {
	if(argc!=4){fputs("Use ORIGINAL_GGUF EXISTING_ALL_BFP8_CACHE NEW_PRIVATE_OUTPUT_DIRECTORY;CPU-only.\n",stderr);return 2;}
	struct stat directory{};Require(!lstat(argv[3],&directory) && S_ISDIR(directory.st_mode) && directory.st_uid==geteuid()
		&& (directory.st_mode&07777)==0700,"Output directory must already be private,owned and not a symlink.");
	std::unique_ptr<FILE,decltype(&fclose)> file(fopen(argv[1],"rb"),&fclose);Require(bool(file),"Cannot read original model.");
	PioneerQwen38Gguf model;Require(model.Open(file.get()),"Complete original851-tensor layout refused.");const uint64_t epoch=model.Epoch();
	std::string error;Cache::Digest original{};
	puts("AUTHENTICATING_SAME_OPEN_COMPLETE_ORIGINAL;CPU_ONLY_NO_DEVICE");fflush(stdout);
	Require(Cache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,original,error)
		&& original==PioneerQ38NativeBfpBandSource::OriginalPin() && model.Stable() && model.Epoch()==epoch,"Complete original SHA/stability refused.");
	Require(!lstat(argv[2],&directory) && S_ISDIR(directory.st_mode) && directory.st_uid==geteuid()
		&& (directory.st_mode&07777)==0700,"Existing private native cache required.");
	Cache cache;Require(cache.Open(argv[2],error),error);PioneerQ38NativeBfpBandSource source(cache);
	Require(source.Bind(model,original,[&]{return model.Stable() && model.Epoch()==epoch;},error),error);
	const auto* tensor=model.Find("blk.0.ffn_gate.weight");Require(tensor && tensor->inputs==5120 && tensor->outputs==17408 && tensor->type==12,"Pinned Q4 matrix shape/type refused.");
	const unsigned index=unsigned(tensor-model.Tensors().data());Require(index==7,"Pinned matrix ordinal changed.");
	std::vector<uint16_t> input(5120);for(unsigned i=0;i<5120;i++)input[i]=uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0));
	std::array<Packet,3> packets;std::array<std::array<uint32_t,32>,3> golden{};std::array<std::string,3> digest;
	for(unsigned sample=0;sample<3;sample++) {
		Require(source.Stage(index,First[sample],input.data(),input.size(),4,packets[sample],error),error);const auto& p=packets[sample];
		Require(p.width==5120 && p.matrixRows==17408 && p.firstRow==First[sample] && p.validRows==32 && p.phases==4
			&& p.key.entry==7 && p.key.sourceSHA256==original && p.key.conversionSHA256==source.ConversionPin()
			&& p.weights.size()==176640 && p.inputs.size()==12800 && p.sourceEpoch==epoch,"Bound packet identity/size refused.");
		golden[sample]=FreshReference(model,index,First[sample],input,p);
		std::vector<uint8_t> joined=p.weights;joined.insert(joined.end(),p.inputs.begin(),p.inputs.end());digest[sample]=Cache::Hex(Cache::Hash(joined.data(),joined.size()));
		printf("VERIFIED_REAL_MODEL_BAND sample%u tensor7 first_row%u width5120 cached_bytes174080 headers_zero full_reference_words32 payload_sha256%s;CPU_ONLY\n",sample,First[sample],digest[sample].c_str());fflush(stdout);
	}
	Require(source.Ready() && model.Stable() && model.Epoch()==epoch,"Source changed before export.");
	const std::string output=argv[3];FILE* meta=NewFile(output+"/PioneerNativeBfpModelBandMetadata.h");
	fputs("/* PRIVATE GENERATED diagnostic predictions;NOT a model/quality/Work certificate. */\n#ifndef PIONEER_NATIVE_BFP_MODEL_BAND_METADATA_H\n#define PIONEER_NATIVE_BFP_MODEL_BAND_METADATA_H\n#include <stdint.h>\nstruct PioneerNativeBfpModelBandMetadata {\n static constexpr unsigned Width=5120,Rows=17408,Tensor=7,Samples=3;\n static unsigned First(unsigned sample){const unsigned rows[3]={0,8704,17376};return sample<3?rows[sample]:~0U;}\n static const uint32_t* Golden(unsigned sample){\n",meta);
	for(unsigned sample=0;sample<3;sample++) {
		fprintf(meta," static const uint32_t g%u[32]={",sample);for(unsigned row=0;row<32;row++)fprintf(meta,"0x%08x%s",golden[sample][row],row==31?"};\n":",");
	}
	fputs(" return sample==0?g0:sample==1?g1:sample==2?g2:nullptr;}\n static const char* PayloadSHA(unsigned sample){\n",meta);
	for(unsigned sample=0;sample<3;sample++)fprintf(meta," if(sample==%u)return \"%s\";\n",sample,digest[sample].c_str());
	fputs(" return nullptr;}\n};\n#endif\n",meta);Finish(meta);
	FILE* data=NewFile(output+"/PioneerNativeBfpModelBandData.h");
	fputs("/* PRIVATE GENERATED learned weights;DO NOT COMMIT/REDISTRIBUTE. */\n#ifndef PIONEER_NATIVE_BFP_MODEL_BAND_DATA_H\n#define PIONEER_NATIVE_BFP_MODEL_BAND_DATA_H\n#include <stdint.h>\nstruct PioneerNativeBfpModelBandData {\n static uint32_t Word(unsigned sample,unsigned width,unsigned address){\n if(sample>=3 || width!=5120 || (address&3))return 0;\n",data);
	for(unsigned sample=0;sample<3;sample++){char name[16];snprintf(name,sizeof(name),"w%u",sample);Array(data,name,packets[sample].weights);}
	Require(packets[0].inputs==packets[1].inputs && packets[0].inputs==packets[2].inputs,"Input snapshots changed between samples.");Array(data,"input",packets[0].inputs);
	fputs(" if(address>=0x8fff0 && address<0x8fff0+176640){const unsigned i=(address-0x8fff0)/4;return sample==0?w0[i]:sample==1?w1[i]:w2[i];}\n if(address>=0x123ff0 && address<0x123ff0+12800)return input[(address-0x123ff0)/4];\n return 0;\n }\n};\n#endif\n",data);Finish(data);
	Require(source.Ready() && model.Stable() && model.Epoch()==epoch,"Source changed during export;DO NOT COMPILE.");
	puts("PASS_PRIVATE_REAL_MODEL_BAND_EXPORT samples3 original_tiles480 every_cached_byte_matches_fresh_original reference_words96;ZERO_CACHE_CONVERSION_OR_REPAIR_NO_DEVICE_NO_MODEL_QUALITY_OR_SPEED_PROOF");return 0;
}
}
int main(int argc,char** argv){try{return Run(argc,argv);}catch(const std::exception& e){fprintf(stderr,"MODEL_BAND_EXPORT_REFUSED: %s;DO_NOT_COMPILE_PARTIAL_OUTPUT\n",e.what());return 1;}}
