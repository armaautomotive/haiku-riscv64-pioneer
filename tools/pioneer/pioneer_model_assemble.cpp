/* SPDX-License-Identifier: MIT */
// Concatenate bounded downloaded PUBLIC model parts into a FRESH file only.
// NO model execution/GPU/driver access. Receiver SHA256 remains mandatory.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef PIONEER_ASSEMBLE_TEST_BYTES
static const uint64_t ModelBytes=UINT64_C(18973870528);
#else
// Host-only ASan/UBSan fixtures. Production has NO size override option.
static const uint64_t ModelBytes=PIONEER_ASSEMBLE_TEST_BYTES;
#endif
int main(int argc,char** argv)
{
	if (argc<4 || argc>19) { std::fprintf(stderr,"Usage: %s FRESH_OUTPUT PREFIX PART...;full SHA256 required afterward\n",argv[0]); return 2; }
	int source[17]; struct stat before[17]; unsigned count=unsigned(argc-2); uint64_t total=0;
	for (unsigned i=0;i<count;i++) {
		source[i]=open(argv[2+i],O_RDONLY|O_NOFOLLOW);
		if (source[i]<0 || fstat(source[i],&before[i]) || !S_ISREG(before[i].st_mode) || before[i].st_size<=0
			|| uint64_t(before[i].st_size)>ModelBytes-total) { std::fprintf(stderr,"STOP:invalid/missing/oversized source part%u\n",i); return 1; }
		total+=uint64_t(before[i].st_size);
	}
	if (total!=ModelBytes) { std::fprintf(stderr,"STOP:parts total%llu expected%llu\n",(unsigned long long)total,(unsigned long long)ModelBytes); return 1; }
	int output=open(argv[1],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
	if (output<0) { std::fprintf(stderr,"STOP:fresh regular output required,error%d\n",errno); return 1; }
	unsigned char* buffer=static_cast<unsigned char*>(std::malloc(1024*1024)); if (!buffer) return 1;
	uint64_t copied=0,next=UINT64_C(1)<<30;
	for (unsigned i=0;i<count;i++) {
		uint64_t left=uint64_t(before[i].st_size);
		while (left) {
			size_t wanted=left>1024*1024?1024*1024:size_t(left);
			ssize_t got=read(source[i],buffer,wanted);
			if (got<=0) { std::fprintf(stderr,"STOP:source read failed\n"); return 1; }
			size_t written=0;
			while (written<size_t(got)) {
				ssize_t n=write(output,buffer+written,size_t(got)-written);
				if (n<=0) { std::fprintf(stderr,"STOP:output write failed\n"); return 1; }
				written+=size_t(n);
			}
			left-=uint64_t(got); copied+=uint64_t(got);
			if (copied>=next) { std::printf("ASSEMBLE %llu/%llu\n",(unsigned long long)copied,(unsigned long long)ModelBytes); std::fflush(stdout); next+=UINT64_C(1)<<30; }
		}
		struct stat after;
		if (fstat(source[i],&after) || after.st_size!=before[i].st_size || after.st_mtime!=before[i].st_mtime || after.st_ino!=before[i].st_ino) {
			std::fprintf(stderr,"STOP:source changed during assembly\n"); return 1;
		}
		close(source[i]);
	}
	struct stat final;
	if (copied!=ModelBytes || fstat(output,&final) || uint64_t(final.st_size)!=ModelBytes || fsync(output) || close(output)) {
		std::fprintf(stderr,"STOP:output length/sync failed\n"); return 1;
	}
	std::free(buffer);
	std::printf("ASSEMBLY_LENGTH_COMPLETE_FULL_SHA256_STILL_REQUIRED\n"); return 0;
}
