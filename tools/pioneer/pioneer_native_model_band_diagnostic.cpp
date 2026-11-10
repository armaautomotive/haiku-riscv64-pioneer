/* SPDX-License-Identifier: MIT */
// Three fixed learned-weight diagnostic samples. NOT model/Work inference.
#if defined(PIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC) != defined(PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE)
#error Dense diagnostic protocol and authenticated source must be selected together
#endif
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeModelBandProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerNativeModelBandSource.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
using Meta=PioneerNativeBfpModelBandMetadata;
constexpr const char* Original="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";
bool Result(const PioneerNativeModelBandRequest& r,unsigned sample,uint64_t generation,unsigned state) {
	if(r.version!=1 || r.sample!=sample || r.expected_tag!=PioneerNativeModelBandTag || r.expected_generation!=generation
		|| r.nonce!=1 || r.reserved || r.accept!=unsigned(state==PIONEER_NATIVE_BFP_CLOSED)
		|| r.result.state!=state || r.result.cold || r.result.started!=1 || r.result.completed!=1
		|| !r.result.polls || r.result.polls>20000 || r.result.phase!=0x7000c
		|| r.result.count!=0x04001400 || r.result.marker!=0x3342464e)return false;
	for(unsigned pc:r.result.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
	const unsigned restored=unsigned(state==PIONEER_NATIVE_BFP_CLOSED);
	return r.result.held==restored && r.result.l1_restored==restored && r.result.tlb_restored==restored;
}
void Dump(FILE* stream,const PioneerNativeModelBandRequest& r,unsigned sample) {
	const auto* golden=Meta::Golden(sample);
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)
		fprintf(stream,"NATIVE_LEARNED_BAND_RETURNED_WORD sample%u first%u pass%u row%u got%08x expected%08x\n",sample,Meta::First(sample),pass,row,r.result.output[pass*32+row],golden[row]);
}
bool Delivered(){return !fflush(stdout) && !ferror(stdout);}
}
int main(int argc,char** argv) {
	if(argc!=2 || strcmp(argv[1],"--fixed-learned-bands")){fputs("Use --fixed-learned-bands for three fixed DEBUG samples,NOT model/Work inference.\n",stderr);return 2;}
	if(geteuid()!=0)return 1;
	PioneerNativeModelBandSource source;std::string error;
	puts("AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET");if(!Delivered())return 1;
	if(!source.Prepare(Original,error) || !source.Ready()){fprintf(stderr,"NATIVE_LEARNED_SOURCE_REFUSED: %s;NO_DEVICE_OPEN\n",error.c_str());return 1;}
	printf("AUTHENTICATED_FIXED_MODEL c600de03 tensor%u width5120 rows%u samples3 payload_and_reference_exact;DEBUG_NOT_WORK\n",Meta::Tensor,Meta::Rows);if(!Delivered())return 1;
	for(unsigned sample=0;sample<3;sample++) {
		if(!source.Ready())return 1;
		int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
		tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeModelBandTag;
		if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1
			|| metadata.expected_tag || metadata.expected_generation || metadata.reserved[0] || metadata.reserved[1] || metadata.reserved[2]
			|| metadata.driver!=55 || metadata.build_tag!=PioneerNativeModelBandTag || !metadata.generation
			|| metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1) {
			fputs("NATIVE_LEARNED_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;
		}
		PioneerNativeModelBandRequest r={};r.version=1;r.sample=PioneerNativeModelBandNone;r.expected_tag=PioneerNativeModelBandTag;r.expected_generation=metadata.generation;
		if(ioctl(fd,PIONEER_NATIVE_MODEL_BAND_SUPPORT,&r,sizeof(r)) || !PioneerNativeModelBandRequestValid(r)
			|| r.sample!=PioneerNativeModelBandNone || r.nonce || r.accept || r.expected_generation!=metadata.generation || !source.Ready()) {
			fputs("NATIVE_LEARNED_SUPPORT_SOURCE_REFUSED;NO_RUN\n",stderr);close(fd);return 1;
		}
		r={};r.version=1;r.sample=sample;r.expected_tag=PioneerNativeModelBandTag;r.expected_generation=metadata.generation;
		if(ioctl(fd,PIONEER_NATIVE_MODEL_BAND_RUN,&r,sizeof(r)) || !Result(r,sample,metadata.generation,PIONEER_NATIVE_BFP_RESULT) || !source.Ready()) {
			fprintf(stderr,"NATIVE_LEARNED_RUN_UNPROVEN sample%u state%u cold%u started%u completed%u polls%u pc%x/%x phase%x count%x marker%x;CLOSE_ONLY_NO_END_OR_RETRY\n",sample,
				r.result.state,r.result.cold,r.result.started,r.result.completed,r.result.polls,r.result.pc[0],r.result.pc[1],r.result.phase,r.result.count,r.result.marker);
			close(fd);return 1;
		}
		const auto* golden=Meta::Golden(sample);
		for(unsigned word=0;word<64;word++)if(r.result.output[word]!=golden[word%32]) {
			fprintf(stderr,"NATIVE_LEARNED_NUMERICAL_MISMATCH sample%u pass%u row%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",sample,word/32,word%32,r.result.output[word],golden[word%32]);
			Dump(stderr,r,sample);close(fd);return 1;
		}
		if(!source.Ready()){fputs("NATIVE_LEARNED_SOURCE_CHANGED;CLOSE_ONLY_NO_END\n",stderr);close(fd);return 1;}
		PioneerNativeModelBandRequest end={};end.version=1;end.sample=sample;end.expected_tag=PioneerNativeModelBandTag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
		if(ioctl(fd,PIONEER_NATIVE_MODEL_BAND_END,&end,sizeof(end)) || !Result(end,sample,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
			|| memcmp(end.result.output,r.result.output,sizeof(r.result.output)) || !source.Ready()) {
			fputs("NATIVE_LEARNED_END_SOURCE_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
		}
		if(close(fd))return 1;
		Dump(stdout,r,sample);
		printf("PASS_NATIVE_LEARNED_BAND sample%u tensor%u first%u width5120 phases4 dual_jobs2 exact_words64 polls%u generation%llu;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM\n",sample,Meta::Tensor,Meta::First(sample),r.result.polls,(unsigned long long)metadata.generation);
		if(!Delivered())return 1; // never start next sample on a failed channel
	}
	puts("PASS_NATIVE_LEARNED_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM");
	return Delivered()?0:1;
}
