/* SPDX-License-Identifier: MIT */
// Nine fixed full-K authenticated compressed-Q4/FPU DEBUG samples,NEVER Work inference.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerNativeCompactQ4StreamSource.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
#include "PioneerQ38Q4KernelTiming.h"
#endif
namespace {
using Meta=PioneerNativeCompactQ4StreamMetadata;
using Variant=PioneerNativeCompactQ4StreamVariant;
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
constexpr uint64_t ExpectedGeneration=43; // requires fresh independent admission
#else
constexpr uint64_t ExpectedGeneration=37; // exact fresh native104 parent epoch
#endif
constexpr const char* Original="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";
bool Result(const PioneerNativeCompactQ4StreamRequest& r,unsigned sample,uint64_t generation,unsigned state) {
	const auto& c=r.result.compute;const unsigned restored=unsigned(state==PIONEER_NATIVE_BFP_CLOSED);
	if(r.version!=1 || r.sample!=sample || r.expected_tag!=PioneerNativeCompactQ4StreamVariant::Tag || r.expected_generation!=generation
		|| r.nonce!=1 || r.reserved || r.accept!=restored || c.state!=state || c.cold || c.started!=1 || c.completed!=1
		|| !c.polls || c.polls>20000 || c.phase!=0x7000c || c.count!=Variant::Count(sample) || c.marker!=Variant::Marker
		|| (c.controls[0]&1) || c.controls[1] || c.controls[2])return false;
	for(unsigned pc:c.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
	return c.held==restored && c.l1_restored==restored && c.tlb_restored==restored
		&& r.result.phase==(restored?10U:5U) && r.result.niu_restored==restored
		&& r.result.clock_restored==restored && r.result.gddr_restored==restored && r.result.saved_l1==26908
		&& r.result.saved_gddr==(Variant::Width(sample)/256)*1280 && r.result.requests==2*(Variant::Width(sample)/256)*26
		&& r.result.flits==2*(Variant::Width(sample)/256)*80;
}
void Dump(FILE* stream,const PioneerNativeCompactQ4StreamRequest& r,unsigned sample) {
	const auto* golden=Meta::Golden(sample);
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)
		fprintf(stream,"NATIVE_COMPACT_Q4_STREAM_RETURNED_WORD sample%u tensor%u first%u width%u pass%u row%u got%08x expected%08x\n",
			sample,Meta::Tensor(sample),Meta::First(sample),Meta::Width(sample),pass,row,r.result.compute.output[pass*32+row],golden[row]);
}
bool Delivered(){return !fflush(stdout) && !ferror(stdout);}
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
void DumpTiming(const PioneerNativeCompactQ4StreamRequest& r,unsigned sample){
	using T=PioneerQ38Q4KernelTiming;
	for(unsigned pass=0;pass<2;pass++){
		const auto* p=r.result.timing+pass*8;const unsigned state=T::State(p,Variant::Width(sample),pass);
		if(state==1){
			const uint64_t start=uint64_t(p[4])|(uint64_t(p[5])<<32),end=uint64_t(p[6])|(uint64_t(p[7])<<32);
			printf("NATIVE_COMPACT_Q4_TIMING sample%u width%u pass%u state1 start%llu end%llu cycles%llu;CARD_CYCLES_ONLY_NO_FREQUENCY_ASSUMPTION\n",
				sample,Variant::Width(sample),pass,(unsigned long long)start,(unsigned long long)end,(unsigned long long)(end-start));
		}else printf("NATIVE_COMPACT_Q4_TIMING sample%u width%u pass%u state%u;UNAVAILABLE_NO_TIMING_OR_CLEANUP_AUTHORITY\n",sample,Variant::Width(sample),pass,state);
	}
}
#endif
}
int main(int argc,char** argv) {
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
	if(argc!=2 || strcmp(argv[1],"--fixed-compact-q4-timing-full-k-stream")){fputs("Use --fixed-compact-q4-timing-full-k-stream for two fixed full-K DEBUG bands,NOT model/Work inference.\n",stderr);return 2;}
#else
	if(argc!=2 || strcmp(argv[1],"--fixed-compact-q4-full-k-stream")){fputs("Use --fixed-compact-q4-full-k-stream for nine fixed full-K DEBUG samples,NOT model/Work inference.\n",stderr);return 2;}
#endif
	if(geteuid()!=0)return 1;
	PioneerNativeCompactQ4StreamSource source;std::string error;
	puts("AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_COMPACT_Q4_STREAM_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET");if(!Delivered())return 1;
	if(!source.Prepare(Original,error) || !source.Ready()){fprintf(stderr,"NATIVE_COMPACT_Q4_STREAM_SOURCE_REFUSED: %s;NO_DEVICE_OPEN\n",error.c_str());return 1;}
	puts("AUTHENTICATED_FIXED_COMPACT_Q4_STREAM_MODEL c600de03 samples9 widths5120_17408 rows32 full_k_payload_and_reference_exact;DEBUG_NOT_WORK");if(!Delivered())return 1;
	uint64_t seriesGeneration=0;
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
	for(unsigned selected=0;selected<2;selected++) {
		const unsigned sample=selected?6:0; // Representative fixed model widths.
#else
	for(unsigned sample=0;sample<9;sample++) {
#endif
		if(!source.Ready())return 1;
		const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
		tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeCompactQ4StreamVariant::Tag;
		if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1 || metadata.expected_tag || metadata.expected_generation
			|| metadata.reserved[0] || metadata.reserved[1] || metadata.reserved[2] || metadata.driver!=55 || metadata.build_tag!=PioneerNativeCompactQ4StreamVariant::Tag
			|| metadata.generation!=ExpectedGeneration || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1) {
			fputs("NATIVE_COMPACT_Q4_STREAM_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;
		}
		if(seriesGeneration && metadata.generation!=seriesGeneration){fputs("NATIVE_COMPACT_Q4_STREAM_EPOCH_CHANGED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;}
		seriesGeneration=metadata.generation;
		PioneerNativeCompactQ4StreamRequest r={};r.version=1;r.sample=PioneerNativeCompactQ4StreamNone;r.expected_tag=PioneerNativeCompactQ4StreamVariant::Tag;r.expected_generation=metadata.generation;
		if(ioctl(fd,PIONEER_NATIVE_COMPACT_Q4_STREAM_SUPPORT,&r,sizeof(r)) || !PioneerNativeCompactQ4StreamRequestValid(r)
			|| r.sample!=PioneerNativeCompactQ4StreamNone || r.nonce || r.accept || r.expected_generation!=metadata.generation || !source.Ready()) {
			fputs("NATIVE_COMPACT_Q4_STREAM_SUPPORT_SOURCE_REFUSED;NO_RUN\n",stderr);close(fd);return 1;
		}
		r={};r.version=1;r.sample=sample;r.expected_tag=PioneerNativeCompactQ4StreamVariant::Tag;r.expected_generation=metadata.generation;
		if(ioctl(fd,PIONEER_NATIVE_COMPACT_Q4_STREAM_RUN,&r,sizeof(r)) || !Result(r,sample,metadata.generation,PIONEER_NATIVE_BFP_RESULT) || !source.Ready()) {
			const auto& c=r.result.compute;
			fprintf(stderr,"NATIVE_COMPACT_Q4_STREAM_RUN_UNPROVEN sample%u state%u cold%u started%u completed%u polls%u pc%x/%x phase%x count%x marker%x journal_phase%u saved_l1%u;CLOSE_ONLY_NO_END_OR_RETRY\n",
				sample,c.state,c.cold,c.started,c.completed,c.polls,c.pc[0],c.pc[1],c.phase,c.count,c.marker,r.result.phase,r.result.saved_l1);
			close(fd);return 1;
		}
		const auto* golden=Meta::Golden(sample);
		for(unsigned word=0;word<64;word++)if(r.result.compute.output[word]!=golden[word%32]) {
			fprintf(stderr,"NATIVE_COMPACT_Q4_STREAM_NUMERICAL_MISMATCH sample%u pass%u row%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",sample,word/32,word%32,r.result.compute.output[word],golden[word%32]);
			Dump(stderr,r,sample);close(fd);return 1;
		}
		if(!source.Ready()){fputs("NATIVE_COMPACT_Q4_STREAM_SOURCE_CHANGED;CLOSE_ONLY_NO_END\n",stderr);close(fd);return 1;}
		PioneerNativeCompactQ4StreamRequest end={};end.version=1;end.sample=sample;end.expected_tag=PioneerNativeCompactQ4StreamVariant::Tag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
		if(ioctl(fd,PIONEER_NATIVE_COMPACT_Q4_STREAM_END,&end,sizeof(end)) || !Result(end,sample,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
			|| memcmp(end.result.compute.output,r.result.compute.output,sizeof(r.result.compute.output))
			|| memcmp(end.result.compute.controls,r.result.compute.controls,sizeof(r.result.compute.controls))
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
			|| memcmp(end.result.timing,r.result.timing,sizeof(r.result.timing))
#endif
			|| end.result.compute.polls!=r.result.compute.polls || !source.Ready()) {
			fputs("NATIVE_COMPACT_Q4_STREAM_END_SOURCE_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
		}
		if(close(fd))return 1;
		Dump(stdout,r,sample);
		printf("PASS_NATIVE_COMPACT_Q4_STREAM_SAMPLE sample%u tensor%u first%u width%u phases4 dual_jobs2 exact_words64 polls%u generation%llu saved_l126908 saved_gddr%u requests%u flits%u;EXPLICIT_END_L1_GDDR_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM\n",
			sample,Meta::Tensor(sample),Meta::First(sample),Meta::Width(sample),r.result.compute.polls,(unsigned long long)metadata.generation,
			r.result.saved_gddr,r.result.requests,r.result.flits);
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
		DumpTiming(r,sample); // AFTER actual clean END and close,never cleanup authority.
#endif
		if(!Delivered())return 1; // no next sample on failed output channel
	}
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
	puts("PASS_NATIVE_COMPACT_Q4_STREAM samples2 exact_words128 clean_ends2;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM");
#else
	puts("PASS_NATIVE_COMPACT_Q4_STREAM samples9 exact_words576 clean_ends9;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM");
#endif
	return Delivered()?0:1;
}
