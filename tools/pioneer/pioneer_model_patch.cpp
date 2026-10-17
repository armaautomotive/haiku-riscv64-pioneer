/* SPDX-License-Identifier: MIT */
// Repair ONLY three fixed64MiB leaves in this task's UNPUBLISHED model.
// Requires a separately SHA-authenticated replacement and exact byte-for-
// byte OLD-range agreement with the preserved corrupt component BEFORE
// ANY write. Originals/other ranges remain untouched. Whole model SHA is
// mandatory afterward;this program NEVER publishes/executes a model.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
static_assert(sizeof(off_t)>=8,"full model offsets require64 bits");
struct PatchFd { int fd=-1; ~PatchFd() { if (fd>=0) close(fd); } };
static bool PatchSame(int fd,const struct stat& before)
{
	struct stat after;
	return !fstat(fd,&after) && after.st_dev==before.st_dev && after.st_ino==before.st_ino
		&& after.st_size==before.st_size && after.st_mtime==before.st_mtime && after.st_ctime==before.st_ctime && after.st_nlink==1;
}
static bool PatchRead(int fd,void* data,size_t bytes,uint64_t at)
{
	for (size_t n=0;n<bytes;) {
		ssize_t got=pread(fd,static_cast<char*>(data)+n,bytes-n,off_t(at+n));
		if (got<0 && errno==EINTR) continue;
		if (got<=0) return false;
		n+=size_t(got);
	}
	return true;
}
static bool PatchRun(const char* target,const char* original,const char* replacement,
	uint64_t total,uint64_t originalBytes,uint64_t absolute,uint64_t relative,uint64_t length)
{
	if (!length || absolute>total || length>total-absolute || relative>originalBytes || length>originalBytes-relative) return false;
	PatchFd out,old,input;
	out.fd=open(target,O_RDWR|O_NOFOLLOW); old.fd=open(original,O_RDONLY|O_NOFOLLOW); input.fd=open(replacement,O_RDONLY|O_NOFOLLOW);
	struct stat identity[3]; int descriptors[]={out.fd,old.fd,input.fd}; uint64_t sizes[]={total,originalBytes,length};
	for (unsigned i=0;i<3;i++) {
		if (descriptors[i]<0 || fstat(descriptors[i],&identity[i]) || !S_ISREG(identity[i].st_mode)
			|| identity[i].st_nlink!=1 || uint64_t(identity[i].st_size)!=sizes[i]) return false;
		for (unsigned j=0;j<i;j++) if (identity[i].st_dev==identity[j].st_dev && identity[i].st_ino==identity[j].st_ino) return false;
	}
	const size_t block=1024*1024;
	char* buffers=static_cast<char*>(malloc(2*block)); if (!buffers) return false;
	bool okay=true;
	// Complete preflight of the ENTIRE old leaf,including its last byte.
	// No interleaved compare-and-write that could damage an earlier block.
	for (uint64_t n=0;n<length && okay;n+=block) {
		size_t bytes=length-n<block?size_t(length-n):block;
		okay=PatchRead(out.fd,buffers,bytes,absolute+n) && PatchRead(old.fd,buffers+block,bytes,relative+n)
			&& !memcmp(buffers,buffers+block,bytes);
	}
	for (unsigned i=0;i<3 && okay;i++) okay=PatchSame(descriptors[i],identity[i]);
	if (!okay) { free(buffers); return false; }
	for (uint64_t n=0;n<length && okay;n+=block) {
		size_t bytes=length-n<block?size_t(length-n):block;
		okay=PatchRead(input.fd,buffers,bytes,n);
		for (size_t written=0;written<bytes && okay;) {
			ssize_t got=pwrite(out.fd,buffers+written,bytes-written,off_t(absolute+n+written));
			if (got<0 && errno==EINTR) continue;
			if (got<=0) okay=false; else written+=size_t(got);
		}
	}
	struct stat final;
	okay=okay && PatchSame(old.fd,identity[1]) && PatchSame(input.fd,identity[2]) && !fstat(out.fd,&final)
		&& final.st_dev==identity[0].st_dev && final.st_ino==identity[0].st_ino && final.st_nlink==1 && uint64_t(final.st_size)==total
		&& !fsync(out.fd);
	free(buffers);
	if (okay) { int fd=out.fd; out.fd=-1; okay=close(fd)==0; }
	return okay;
}
int main(int argc,char** argv)
{
#ifdef PIONEER_PATCH_TEST_ABI
	if (argc!=5 || strcmp(argv[1],"--apply")) return 2;
	bool okay=PatchRun(argv[2],argv[3],argv[4],6*1024*1024,4*1024*1024,2*1024*1024,1024*1024,2*1024*1024);
#else
	if (argc!=4 || strcmp(argv[1],"--apply")) return 2;
	const char* target="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf.assembled-20261017";
	struct stat published;
	if (!lstat("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",&published) || errno!=ENOENT) return 1;
	struct Spec { const char* id; const char* original; uint64_t relative,absolute; };
	const Spec specs[]={
		{"0","/boot/home/models/qwen38-parallel-20261017/part0-startpos",1610612736,4656888560},
		{"3","/boot/home/models/qwen38-parallel-20261017/part3-startpos",1342177280,10361301118},
		{"4","/boot/home/models/qwen38-parallel-20261017/part4-startpos",1409286144,12419359320}};
	const Spec* selected=nullptr;
	for (const auto& spec:specs) if (!strcmp(argv[2],spec.id)) selected=&spec;
	if (!selected) return 2;
	bool okay=PatchRun(target,selected->original,argv[3],UINT64_C(18973870528),UINT64_C(1990949338),selected->absolute,selected->relative,UINT64_C(67108864));
#endif
	if (!okay) { fprintf(stderr,"STOP:fixed leaf repair refused/failed;UNPUBLISHED_MODEL_STILL_REQUIRES_WHOLE_SHA\n"); return 1; }
	puts("FIXED_LEAF_PATCH_COMPLETE_WHOLE_SHA_STILL_REQUIRED;ORIGINAL_BAD_COMPONENT_PRESERVED;NO_MODEL_EXECUTION");
	return 0;
}
