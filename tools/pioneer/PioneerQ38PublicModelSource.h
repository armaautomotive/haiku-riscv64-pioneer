/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PUBLIC_MODEL_SOURCE_H
#define PIONEER_Q38_PUBLIC_MODEL_SOURCE_H
#include "PioneerQwen38PackedPlan.h"
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#ifdef PIONEER_Q38_NATIVE_Q8_LAYOUT
#if !defined(PIONEER_Q38_WORK_INFERENCE) || !defined(PIONEER_Q38_SPAN_PRELOAD) || !defined(PIONEER_Q38_SPAN_FAST) || defined(PIONEER_Q38_CPU_VALIDATION)
#error New SM7 source requires its separate fast-span Work producer;NEVER CPU validation or legacy upload
#endif
#include "PioneerQ38NativeQ8Data.h"
#endif
// Fixed PUBLIC27B only. Authenticate the FULL already-open file description
// BEFORE any accelerator FD/threaded model context exists. No chat template,
// executable,network request,arbitrary path or caller-supplied digest.
class PioneerQ38PublicModelSource {
	FILE* file=nullptr;
	PioneerQwen38Gguf model;
	struct stat identity={};
	uint64_t verifiedEpoch=0;
	bool authenticated=false,attempted=false,borrowed=false;
	static bool Same(const struct stat& a,const struct stat& b) {
		if (a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime) return false;
#if defined(__APPLE__)
		return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
		return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
	}
public:
	PioneerQ38PublicModelSource()=default;
	~PioneerQ38PublicModelSource() { if (file) fclose(file); }
	PioneerQ38PublicModelSource(const PioneerQ38PublicModelSource&)=delete;
	PioneerQ38PublicModelSource& operator=(const PioneerQ38PublicModelSource&)=delete;
	bool Ready() const {
		struct stat current={};
		return authenticated && file && model.Epoch()==verifiedEpoch && model.Stable()
			&& !fstat(fileno(file),&current) && Same(identity,current);
	}
	const PioneerQwen38Gguf& Model() const { return model; }
#ifdef PIONEER_Q38_BENCHMARK_HOST
	// POSIX stdio active-handle barrier before a SINGLE-THREADED benchmark
	// parent forks. SAME already fully authenticated open description only;
	// never a pathname reopen,digest bypass,model load or GPU capability.
	// PIONEER_Q38_BENCHMARK_FORK_BARRIER_BEGIN
	bool PrepareBenchmarkFork() {
		return !borrowed && Ready() && !fseeko(file,0,SEEK_SET) && Ready();
	}
	// PIONEER_Q38_BENCHMARK_FORK_BARRIER_END
#endif
#ifdef PIONEER_Q38_Q8_DOMAIN_DIAGNOSTIC
#ifdef PIONEER_Q38_WORK_INFERENCE
#error Quant-byte inventory must NEVER enter Work inference
#endif
	// CPU-only read-only inventory of PUBLIC Q8 bytes,not learned products,
	// an execution fallback or any accelerator/source-format capability.
	// PIONEER_Q38_Q8_DOMAIN_READ_BEGIN
	bool ReadQ8DomainChunk(unsigned tensor,uint64_t offset,unsigned bytes,uint8_t* output,unsigned capacity) {
		if(!Ready() || tensor>=model.Tensors().size() || !output || !bytes || bytes>34*16384
			|| capacity<bytes || offset%34 || bytes%34)return false;
		const auto& t=model.Tensors()[tensor];
		return t.type==8 && t.rank==2 && model.ReadPacked(tensor,offset,bytes,output,capacity) && Ready();
	}
	// PIONEER_Q38_Q8_DOMAIN_READ_END
#endif
#ifdef PIONEER_Q38_CPU_VALIDATION
	// Diagnostic build ONLY. Same authenticated file description;never
	// reopens a pathname or replaces accelerator results with CPU outputs.
	bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity) {
		return Ready() && model.ReadPacked(tensor,offset,bytes,out,capacity) && Ready();
	}
#endif
	// Pinned llama_model_load_from_file_ptr borrows (does NOT close) this
	// FILE. Load synchronously BEFORE session.Open/any page reads. The
	// source outlives the llama model; never reopen a possibly replaced path.
	FILE* BorrowForModelLoad() {
		if (borrowed || !Ready() || fseeko(file,0,SEEK_SET)) return nullptr;
		borrowed=true; return file;
	}
	bool Open() {
		if (attempted) return false;
		attempted=true;
		file=fopen("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf","rb");
		if (!file || fstat(fileno(file),&identity) || !S_ISREG(identity.st_mode)
			|| uint64_t(identity.st_size)!=PioneerQwen38Gguf::FullBytes || fseeko(file,0,SEEK_SET)) return false;
		int result[2]; if (pipe(result)) return false;
		pid_t child=fork();
		if (!child) {
			if (dup2(fileno(file),0)<0 || dup2(result[1],1)<0) _exit(126);
			for (int fd:{fileno(file),result[0],result[1]}) if (fd>2) close(fd);
			char name[]="sha256sum"; char* argv[]={name,nullptr}; execv("/bin/sha256sum",argv); _exit(127);
		}
		close(result[1]); if (child<0) { close(result[0]); return false; }
		char digest[96]={}; unsigned used=0; bool okay=true;
		for (;;) {
			char data[96]; ssize_t count=read(result[0],data,sizeof(data));
			if (count<0 && errno==EINTR) continue;
			if (count<=0) { if (count<0) okay=false; break; }
			if (unsigned(count)>sizeof(digest)-1-used) okay=false;
			else { memcpy(digest+used,data,size_t(count)); used+=unsigned(count); }
		}
		close(result[0]); int status=0; pid_t done;
		do { done=waitpid(child,&status,0); } while (done<0 && errno==EINTR);
		struct stat after={};
		if (!okay || done!=child || !WIFEXITED(status) || WEXITSTATUS(status) || used!=68
			|| strcmp(digest,"c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747  -\n")
			|| fstat(fileno(file),&after) || !Same(identity,after) || !model.Open(file)) return false;
		verifiedEpoch=model.Epoch(); authenticated=true; return Ready();
	}
	// PIONEER_Q38_AUTHENTICATED_WEIGHT_PAGE_BEGIN
	bool ReadWeightPage(const PioneerQwen38PackedPlan& plan,unsigned fragment,unsigned page,uint8_t* output,unsigned capacity) {
		if (!Ready() || !output || capacity<4096 || !plan.MatchesSource(model)) return false;
		uint8_t stage[4096];
		if (!plan.ReadWeightPage(model,fragment,page,stage,sizeof(stage)) || !Ready()) return false;
#ifdef PIONEER_Q38_NATIVE_Q8_LAYOUT
		const auto& parts=plan.Parts();if(fragment>=parts.size())return false;
		const auto part=parts[fragment];const auto& tensors=model.Tensors();
		if(part.tensor>=tensors.size())return false;
		if(tensors[part.tensor].type==8) {
			uint64_t relative=uint64_t(page)*4096;if(relative>=part.bytes)return false;
			unsigned valid=unsigned(part.bytes-relative<4096?part.bytes-relative:4096);uint8_t encoded[4096];
			if(!PioneerQ38NativeQ8Data::PreparePage(stage,relative,valid,encoded,sizeof(encoded)) || !Ready())return false;
			memcpy(output,encoded,sizeof(encoded));return true;
		}
#endif
		memcpy(output,stage,sizeof(stage)); return true;
	}
	// PIONEER_Q38_AUTHENTICATED_WEIGHT_PAGE_END
};
#endif
