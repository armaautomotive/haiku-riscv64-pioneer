/* SPDX-License-Identifier: MIT */
// Three fixed authenticated compressed-Q4/FPU DEBUG samples,NEVER Work inference.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeQ4FpuProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerNativeQ4FpuSource.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
using Meta=PioneerNativeQ4FpuMetadata;
using Variant=PioneerNativeQ4FpuVariant;
constexpr const char* Original="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";
bool Result(const PioneerNativeQ4FpuRequest& r,unsigned sample,uint64_t generation,unsigned state) {
	const auto& c=r.result.compute;const unsigned restored=unsigned(state==PIONEER_NATIVE_BFP_CLOSED);
	if(r.version!=1 || r.sample!=sample || r.expected_tag!=PioneerNativeQ4FpuVariant::Tag || r.expected_generation!=generation
		|| r.nonce!=1 || r.reserved || r.accept!=restored || c.state!=state || c.cold || c.started!=1 || c.completed!=1
		|| !c.polls || c.polls>20000 || c.phase!=0x7000c || c.count!=Variant::Count || c.marker!=Variant::Marker
		|| (c.controls[0]&1) || c.controls[1] || c.controls[2])return false;
	for(unsigned pc:c.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
	return c.held==restored && c.l1_restored==restored && c.tlb_restored==restored
		&& r.result.phase==(restored?8U:4U) && r.result.niu_restored==restored
		&& r.result.clock_restored==restored && r.result.saved_l1==12372;
}
void Dump(FILE* stream,const PioneerNativeQ4FpuRequest& r,unsigned sample) {
	const auto* golden=Meta::Golden(sample);
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)
		fprintf(stream,"NATIVE_Q4_FPU_BLOCK_RETURNED_WORD sample%u first%u kblock%u pass%u row%u got%08x expected%08x\n",sample,Meta::First(sample),Meta::Block(sample),pass,row,r.result.compute.output[pass*32+row],golden[row]);
}
bool Delivered(){return !fflush(stdout) && !ferror(stdout);}
}
int main(int argc,char** argv) {
	if(argc!=2 || strcmp(argv[1],"--fixed-q4-fpu-learned-blocks")){fputs("Use --fixed-q4-fpu-learned-blocks for three fixed DEBUG samples,NOT model/Work inference.\n",stderr);return 2;}
	if(geteuid()!=0)return 1;
	PioneerNativeQ4FpuSource source;std::string error;
	puts("AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_Q4_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET");if(!Delivered())return 1;
	if(!source.Prepare(Original,error) || !source.Ready()){fprintf(stderr,"NATIVE_Q4_FPU_SOURCE_REFUSED: %s;NO_DEVICE_OPEN\n",error.c_str());return 1;}
	puts("AUTHENTICATED_FIXED_Q4_MODEL c600de03 tensor7 matrix_width5120 block_width256 rows32 samples3 payload_and_reference_exact;DEBUG_NOT_WORK");if(!Delivered())return 1;
	uint64_t seriesGeneration=0;
	for(unsigned sample=0;sample<3;sample++) {
		if(!source.Ready())return 1;
		const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
		tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeQ4FpuVariant::Tag;
		if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1 || metadata.expected_tag || metadata.expected_generation
			|| metadata.reserved[0] || metadata.reserved[1] || metadata.reserved[2] || metadata.driver!=55 || metadata.build_tag!=PioneerNativeQ4FpuVariant::Tag
			|| !metadata.generation || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1) {
			fputs("NATIVE_Q4_FPU_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;
		}
		if(seriesGeneration && metadata.generation!=seriesGeneration){fputs("NATIVE_Q4_FPU_EPOCH_CHANGED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;}
		seriesGeneration=metadata.generation;
		PioneerNativeQ4FpuRequest r={};r.version=1;r.sample=PioneerNativeQ4FpuNone;r.expected_tag=PioneerNativeQ4FpuVariant::Tag;r.expected_generation=metadata.generation;
		if(ioctl(fd,PIONEER_NATIVE_Q4_FPU_SUPPORT,&r,sizeof(r)) || !PioneerNativeQ4FpuRequestValid(r)
			|| r.sample!=PioneerNativeQ4FpuNone || r.nonce || r.accept || r.expected_generation!=metadata.generation || !source.Ready()) {
			fputs("NATIVE_Q4_FPU_SUPPORT_SOURCE_REFUSED;NO_RUN\n",stderr);close(fd);return 1;
		}
		r={};r.version=1;r.sample=sample;r.expected_tag=PioneerNativeQ4FpuVariant::Tag;r.expected_generation=metadata.generation;
		if(ioctl(fd,PIONEER_NATIVE_Q4_FPU_RUN,&r,sizeof(r)) || !Result(r,sample,metadata.generation,PIONEER_NATIVE_BFP_RESULT) || !source.Ready()) {
			const auto& c=r.result.compute;
			fprintf(stderr,"NATIVE_Q4_FPU_RUN_UNPROVEN sample%u state%u cold%u started%u completed%u polls%u pc%x/%x phase%x count%x marker%x journal_phase%u saved_l1%u;CLOSE_ONLY_NO_END_OR_RETRY\n",
				sample,c.state,c.cold,c.started,c.completed,c.polls,c.pc[0],c.pc[1],c.phase,c.count,c.marker,r.result.phase,r.result.saved_l1);
			close(fd);return 1;
		}
		const auto* golden=Meta::Golden(sample);
		for(unsigned word=0;word<64;word++)if(r.result.compute.output[word]!=golden[word%32]) {
			fprintf(stderr,"NATIVE_Q4_FPU_NUMERICAL_MISMATCH sample%u pass%u row%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",sample,word/32,word%32,r.result.compute.output[word],golden[word%32]);
			Dump(stderr,r,sample);close(fd);return 1;
		}
		if(!source.Ready()){fputs("NATIVE_Q4_FPU_SOURCE_CHANGED;CLOSE_ONLY_NO_END\n",stderr);close(fd);return 1;}
		PioneerNativeQ4FpuRequest end={};end.version=1;end.sample=sample;end.expected_tag=PioneerNativeQ4FpuVariant::Tag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
		if(ioctl(fd,PIONEER_NATIVE_Q4_FPU_END,&end,sizeof(end)) || !Result(end,sample,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
			|| memcmp(end.result.compute.output,r.result.compute.output,sizeof(r.result.compute.output))
			|| memcmp(end.result.compute.controls,r.result.compute.controls,sizeof(r.result.compute.controls))
			|| end.result.compute.polls!=r.result.compute.polls || !source.Ready()) {
			fputs("NATIVE_Q4_FPU_END_SOURCE_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
		}
		if(close(fd))return 1;
		Dump(stdout,r,sample);
		printf("PASS_NATIVE_Q4_FPU_BLOCK sample%u tensor7 first%u kblock%u width256 phases4 dual_jobs2 exact_words64 polls%u generation%llu saved_l112372;EXPLICIT_END_L1_NIU_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM\n",
			sample,Meta::First(sample),Meta::Block(sample),r.result.compute.polls,(unsigned long long)metadata.generation);
		if(!Delivered())return 1; // no next sample on failed output channel
	}
	puts("PASS_NATIVE_Q4_FPU_BLOCKS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM");
	return Delivered()?0:1;
}
