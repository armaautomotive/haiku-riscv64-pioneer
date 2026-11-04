/* SPDX-License-Identifier: MIT */
// PRIVATE original model READ ONLY;no original/derived weight bytes printed.
#define PIONEER_MIXED_FPU_MACHINE_ONLY
#include "pioneer_q38_mixed_fpu_decode_test.cpp"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
#include <fcntl.h>
#include <unistd.h>
static bool Same(const struct stat& a,const struct stat& b) {
	return a.st_dev==b.st_dev && a.st_ino==b.st_ino && a.st_size==b.st_size
		&& a.st_uid==b.st_uid && a.st_gid==b.st_gid && a.st_mode==b.st_mode && a.st_nlink==b.st_nlink
#ifdef __APPLE__
		&& a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec && a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec
		&& a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
		&& a.st_mtim.tv_sec==b.st_mtim.tv_sec && a.st_mtim.tv_nsec==b.st_mtim.tv_nsec
		&& a.st_ctim.tv_sec==b.st_ctim.tv_sec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
int main(int argc,char** argv)
{
	assert(argc==2 && std::fegetround()==FE_TONEAREST);
	int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);
	struct stat initial={},current={};assert(!fstat(fd,&initial) && S_ISREG(initial.st_mode)
		&& initial.st_uid==geteuid() && initial.st_nlink==1 && !(initial.st_mode&0022)
		&& uint64_t(initial.st_size)==PioneerQwen38Gguf::FullBytes);
	FILE* file=fdopen(fd,"rb");assert(file);PioneerQwen38Gguf source;assert(source.Open(file));
	PioneerModelWeightCache::Digest digest;std::string error;
	assert(PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
		&& PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
	assert(!fstat(fd,&current) && Same(initial,current) && source.Stable());const uint64_t epoch=source.Epoch();
	MixedFpuPrograms programs;uint64_t total=0;unsigned matrices=0,bands=0,q6=0,q8=0;
	const char* names[]={"blk.0.attn_qkv.weight","blk.0.ssm_out.weight","blk.3.attn_q.weight","blk.3.attn_output.weight","output.weight"};
	for(const char* name:names) {
		const auto* tensor=source.Find(name);assert(tensor);
		const unsigned ordinal=unsigned(tensor-source.Tensors().data()),width=unsigned(tensor->inputs),type=tensor->type;
		assert(tensor->rank==2 && (type==8 || type==14) && (width==5120 || width==6144) && tensor->outputs>=32 && !(tensor->outputs%32));
		const unsigned packed=type==14?210:272,rowBytes=unsigned(tensor->bytes/tensor->outputs);
		unsigned first[]={0U,unsigned((tensor->outputs/2)&~uint64_t(31)),unsigned(tensor->outputs)-32};
		for(unsigned band=0;band<3;band++) {
			assert(first[band]+32<=tensor->outputs);uint64_t before=total;
			for(unsigned block=0;block<width/256;block++) {
				std::array<std::array<uint8_t,272>,32> original{};std::array<std::array<float,256>,32> reference{};
				for(unsigned row=0;row<32;row++) {
					assert(source.ReadPacked(ordinal,uint64_t(first[band]+row)*rowBytes+uint64_t(block)*packed,packed,original[row].data(),original[row].size()));
					if(type==14)assert(PioneerDecodeQ6K(original[row].data(),reference[row].data()));
					else for(unsigned b=0;b<8;b++)assert(PioneerDecodeQ8_0(original[row].data()+b*34,reference[row].data()+b*32));
				}
				MixedFpuMachine machine;machine.Execute(type==14?programs.q6Prepare:programs.q8Prepare);
				for(unsigned column=0;column<256;column++) {
					for(unsigned row=0;row<32;row++) {
						const auto& p=original[row];
						if(type==14) {
							unsigned half=column/128,group=(column%128)/32,lane=column%32;
							machine.lanes[0][row]=p[half*64+(group&1)*32+lane];machine.lanes[3][row]=p[128+half*32+lane];
							int scale=p[192+column/16];if(scale>=128)scale-=256;
							assert(PioneerQ38BinaryScale(uint16_t(p[208]|(unsigned(p[209])<<8)),scale,machine.lanes[1][row]));
						} else {
							unsigned b=(column/32)*34;machine.lanes[0][row]=p[b+2+column%32];
							machine.lanes[1][row]=PioneerFmaBits(PioneerHalfFloat(uint16_t(p[b]|(unsigned(p[b+1])<<8))));
						}
					}
					machine.Execute(type==14?programs.q6[(column%128)/32]:programs.q8);assert(machine.okay);
					for(unsigned row=0;row<32;row++){assert(machine.lanes[0][row]==MixedRound(PioneerFmaBits(reference[row][column])));total++;}
				}
				assert(!fstat(fd,&current) && Same(initial,current) && source.Stable() && source.Epoch()==epoch);
			}
			assert(total-before==uint64_t(width)*32);bands++;
			printf("PASS_PRIVATE_MIXED_FPU_BAND tensor%u type%u width%u first%u rows32 weights%llu exact original-decode/BF16-RNE;NO weight bytes printed,NO hardware/model-quality/speed proof\n",ordinal,type,width,first[band],(unsigned long long)(total-before));fflush(stdout);
		}
		matrices++;if(type==14)q6++;else q8++;
	}
	assert(matrices==5 && bands==15 && q6==2 && q8==3 && total==2654208);
	assert(!fstat(fd,&current) && Same(initial,current) && source.Stable() && source.Epoch()==epoch && !fclose(file));
	printf("PASS_PRIVATE_MIXED_FPU_BINDING matrices%u bands%u Q6families%u Q8families%u weights%llu same-open full original SHA/nanosecond identity/epoch;NO card/Assistant/cache/driver changes or full-model-quality/speed proof\n",matrices,bands,q6,q8,(unsigned long long)total);
}
