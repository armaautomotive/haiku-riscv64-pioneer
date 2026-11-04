/* SPDX-License-Identifier: MIT */
// PRIVATE original READ ONLY;no original/prepared/decoded weight bytes out.
#define PIONEER_Q8_STREAM_IMAGE_ONLY
#include "pioneer_q38_q8_fpu_stream_image_test.cpp"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
#include <fcntl.h>
#include <unistd.h>
static bool Same(const struct stat& a,const struct stat& b) {
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
int main(int argc,char** argv) {
	assert(argc==4);static_cast<void>(&NoMore);int fd=open(argv[3],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);
	struct stat initial={},current={};assert(!fstat(fd,&initial) && S_ISREG(initial.st_mode) && initial.st_uid==geteuid()
		&& initial.st_nlink==1 && !(initial.st_mode&0022) && uint64_t(initial.st_size)==PioneerQwen38Gguf::FullBytes);
	FILE* f=fdopen(fd,"rb");assert(f);PioneerQwen38Gguf source;assert(source.Open(f));
	PioneerModelWeightCache::Digest digest;std::string error;
	assert(PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
		&& PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
	assert(!fstat(fd,&current) && Same(initial,current) && source.Stable());uint64_t epoch=source.Epoch();unsigned cases=0;uint64_t weights=0;
	for(const char* name:{"blk.0.attn_qkv.weight","blk.0.ssm_out.weight","blk.3.attn_q.weight","blk.0.ssm_alpha.weight"}) {
		const auto* tensor=source.Find(name);assert(tensor);unsigned index=unsigned(tensor-source.Tensors().data()),k=unsigned(tensor->inputs);
		assert(tensor->type==8 && SL::Width(k) && (tensor->outputs%32==0 || tensor->outputs==48));
		std::vector<unsigned> bands=tensor->outputs==48?std::vector<unsigned>{0,32}:std::vector<unsigned>{0,unsigned((tensor->outputs/2)&~uint64_t(31)),unsigned(tensor->outputs)-32};
		for(unsigned first:bands) {
			unsigned rows=unsigned(std::min<uint64_t>(32,tensor->outputs-first));assert(rows==16 || rows==32);
			StreamData data(k,0);
			for(unsigned block=0;block<k/256;block++)data.blocks[block].rows=rows;
			for(unsigned block=0;block<k/256;block++)for(unsigned row=0;row<rows;row++)
				assert(source.ReadPacked(index,(uint64_t(first+row)*(k/256)+block)*272,272,data.blocks[block].original.data()+row*272,272));
			assert(!fstat(fd,&current) && Same(initial,current) && source.Stable() && source.Epoch()==epoch);data.Refresh();
			auto image=std::make_unique<StreamRV32>(k==5120?argv[1]:argv[2],k);data.Stage(image->bus);
			assert(image->Execute() && image->Matches(data.expected));cases++;weights+=uint64_t(k)*rows;
			assert(!fstat(fd,&current) && Same(initial,current) && source.Stable() && source.Epoch()==epoch);
			printf("PASS_PRIVATE_ACTUAL_Q8_STREAM tensor%u width%u first%u rows%u two_complete_jobs/all64_words exact,persistent accumulation,actual NoC/shared physical L1/Dst/compiled RV32;diagnostic BF16 inputs,NOT native hardware/real tokens/quality/speed\n",index,k,first,rows);fflush(stdout);
		}
	}
	assert(cases==11 && weights==1818624 && !fstat(fd,&current) && Same(initial,current) && source.Stable() && source.Epoch()==epoch && !fclose(f));
	puts("PASS_PRIVATE_ACTUAL_Q8_FULL_K_STREAM cases11 jobs22 exact_words704 original_weights1818624,includes48-row alpha/full32+tail16,same-open full original SHA/nanosecond identity/epoch;NO weight bytes saved/printed,NO native owner/hardware/full-model-quality/speed proof");
}
