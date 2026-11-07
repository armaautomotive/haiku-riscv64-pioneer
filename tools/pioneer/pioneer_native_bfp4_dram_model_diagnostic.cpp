/* SPDX-License-Identifier: MIT */
// Three fixed authenticated GDDR->L1/FPU DEBUG samples,NEVER Work inference.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfp4DramProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerNativeBfp4DramModelSource.h"
#include "PioneerNativeBfp4BandTiming.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
using Meta=PioneerNativeBfp4DramModelMetadata;
using Variant=PioneerNativeBfp4DramVariant;
constexpr const char* Original="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";
bool Result(const PioneerNativeBfp4DramRequest& r,unsigned sample,uint64_t generation,unsigned state) {
	const auto& c=r.result.compute;const unsigned restored=unsigned(state==PIONEER_NATIVE_BFP_CLOSED);
	if(r.version!=1 || r.sample!=sample || r.expected_tag!=PioneerNativeBfp4DramVariant::Tag || r.expected_generation!=generation
		|| r.nonce!=1 || r.reserved || r.accept!=restored || c.state!=state || c.cold || c.started!=1 || c.completed!=1
		|| !c.polls || c.polls>20000 || c.phase!=0x7000c || c.count!=Variant::Count || c.marker!=Variant::Marker
		|| (c.controls[0]&1) || c.controls[1] || c.controls[2])return false;
	for(unsigned pc:c.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
	if(PioneerNativeBfp4BandTiming::State(r.result.timing,5120,0)!=1
		|| PioneerNativeBfp4BandTiming::State(r.result.timing+8,5120,1)!=1
		|| !PioneerNativeBfp4BandTiming::Later(r.result.timing+12,r.result.timing+6))return false;
	return c.held==restored && c.l1_restored==restored && c.tlb_restored==restored
		&& r.result.phase==(restored?10U:5U) && r.result.gddr_restored==restored && r.result.niu_restored==restored
		&& r.result.clock_restored==restored && r.result.requests==640 && r.result.flits==3200
		&& r.result.saved_l1==34892 && r.result.saved_gddr==25600;
}
void Dump(FILE* stream,const PioneerNativeBfp4DramRequest& r,unsigned sample) {
	const auto* golden=Meta::Golden(sample);
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)
		fprintf(stream,"NATIVE_GDDR_BAND_RETURNED_WORD sample%u first%u pass%u row%u got%08x expected%08x\n",sample,(sample==0?0U:sample==1?8704U:17376U),pass,row,r.result.compute.output[pass*32+row],golden[row]);
}
bool Delivered(){return !fflush(stdout) && !ferror(stdout);}
}
int main(int argc,char** argv) {
	if(argc!=2 || strcmp(argv[1],"--fixed-bfp4-gddr-learned-bands")){fputs("Use --fixed-bfp4-gddr-learned-bands for three fixed DEBUG samples, NOT model/Work inference.\n",stderr);return 2;}
	if(geteuid()!=0)return 1;
	PioneerNativeBfp4DramModelSource source;std::string error;
	puts("AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET");if(!Delivered())return 1;
	if(!source.Prepare(Original,error) || !source.Ready()){fprintf(stderr,"NATIVE_GDDR_SOURCE_REFUSED: %s;NO_DEVICE_OPEN\n",error.c_str());return 1;}
	puts("AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 rows17408 samples3 payload_and_reference_exact;DEBUG_NOT_WORK");if(!Delivered())return 1;
	uint64_t seriesGeneration=0;
	for(unsigned sample=0;sample<3;sample++) {
		if(!source.Ready())return 1;
		const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
		tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeBfp4DramVariant::Tag;
		if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1 || metadata.expected_tag || metadata.expected_generation
			|| metadata.reserved[0] || metadata.reserved[1] || metadata.reserved[2] || metadata.driver!=55 || metadata.build_tag!=PioneerNativeBfp4DramVariant::Tag
			|| !metadata.generation || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1) {
			fputs("NATIVE_GDDR_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;
		}
		if(seriesGeneration && metadata.generation!=seriesGeneration){fputs("NATIVE_GDDR_EPOCH_CHANGED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;}
		seriesGeneration=metadata.generation;
		PioneerNativeBfp4DramRequest r={};r.version=1;r.sample=PioneerNativeBfp4DramNone;r.expected_tag=PioneerNativeBfp4DramVariant::Tag;r.expected_generation=metadata.generation;
		if(ioctl(fd,PIONEER_NATIVE_BFP4_DRAM_SUPPORT,&r,sizeof(r)) || !PioneerNativeBfp4DramRequestValid(r)
			|| r.sample!=PioneerNativeBfp4DramNone || r.nonce || r.accept || r.expected_generation!=metadata.generation || !source.Ready()) {
			fputs("NATIVE_GDDR_SUPPORT_SOURCE_REFUSED;NO_RUN\n",stderr);close(fd);return 1;
		}
		r={};r.version=1;r.sample=sample;r.expected_tag=PioneerNativeBfp4DramVariant::Tag;r.expected_generation=metadata.generation;
		if(ioctl(fd,PIONEER_NATIVE_BFP4_DRAM_RUN,&r,sizeof(r)) || !Result(r,sample,metadata.generation,PIONEER_NATIVE_BFP_RESULT) || !source.Ready()) {
			const auto& c=r.result.compute;
			fprintf(stderr,"NATIVE_GDDR_RUN_UNPROVEN sample%u state%u cold%u started%u completed%u polls%u pc%x/%x phase%x count%x marker%x journal_phase%u requests%u flits%u saved_l1%u saved_gddr%u;CLOSE_ONLY_NO_END_OR_RETRY\n",
				sample,c.state,c.cold,c.started,c.completed,c.polls,c.pc[0],c.pc[1],c.phase,c.count,c.marker,r.result.phase,r.result.requests,r.result.flits,r.result.saved_l1,r.result.saved_gddr);
			close(fd);return 1;
		}
		const auto* golden=Meta::Golden(sample);
		for(unsigned word=0;word<64;word++)if(r.result.compute.output[word]!=golden[word%32]) {
			fprintf(stderr,"NATIVE_GDDR_NUMERICAL_MISMATCH sample%u pass%u row%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",sample,word/32,word%32,r.result.compute.output[word],golden[word%32]);
			Dump(stderr,r,sample);close(fd);return 1;
		}
		if(!source.Ready()){fputs("NATIVE_GDDR_SOURCE_CHANGED;CLOSE_ONLY_NO_END\n",stderr);close(fd);return 1;}
		PioneerNativeBfp4DramRequest end={};end.version=1;end.sample=sample;end.expected_tag=PioneerNativeBfp4DramVariant::Tag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
		if(ioctl(fd,PIONEER_NATIVE_BFP4_DRAM_END,&end,sizeof(end)) || !Result(end,sample,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
			|| memcmp(end.result.compute.output,r.result.compute.output,sizeof(r.result.compute.output))
			|| memcmp(end.result.compute.controls,r.result.compute.controls,sizeof(r.result.compute.controls))
			|| memcmp(end.result.timing,r.result.timing,sizeof(r.result.timing))
			|| end.result.compute.polls!=r.result.compute.polls || !source.Ready()) {
			fputs("NATIVE_GDDR_END_SOURCE_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
		}
		if(close(fd))return 1;
		Dump(stdout,r,sample);
		for(unsigned pass=0;pass<2;pass++){
			const auto* t=r.result.timing+pass*8;
			const uint64_t start=uint64_t(t[4])|(uint64_t(t[5])<<32),finish=uint64_t(t[6])|(uint64_t(t[7])<<32);
			printf("BFP4_GDDR_BAND_CYCLES sample%u pass%u cycles%llu;INCLUDES_GDDR_AND_COMPUTE_NOT_MODEL_SPEED\n",sample,pass,(unsigned long long)(finish-start));
		}
		printf("PASS_NATIVE_BFP4_GDDR_BAND sample%u tensor7 first%u width5120 phases2 dual_jobs2 exact_words64 polls%u generation%llu requests640 flits3200 saved_l134892 saved_gddr25600;EXPLICIT_END_GDDR_L1_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM\n",
			sample,(sample==0?0U:sample==1?8704U:17376U),r.result.compute.polls,(unsigned long long)metadata.generation);
		if(!Delivered())return 1; // no next sample on failed output channel
	}
	puts("PASS_NATIVE_BFP4_GDDR_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM");
	return Delivered()?0:1;
}
