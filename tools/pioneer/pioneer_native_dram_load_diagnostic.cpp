/* SPDX-License-Identifier: MIT */
// One fixed LOAD-ONLY band;END verifies native bytes. NEVER math/model/Work.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeDramLoadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerNativeModelBandSource.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
bool Result(const PioneerNativeDramLoadRequest& r,uint64_t generation,unsigned state) {
	const auto& c=r.result.compute;const unsigned closed=unsigned(state==PIONEER_NATIVE_BFP_CLOSED);
	if(r.version!=1 || r.sample || r.expected_tag!=PioneerNativeDramLoadTag || r.expected_generation!=generation
		|| r.nonce!=1 || r.reserved || r.accept!=closed || c.state!=state || c.cold || c.started!=1 || c.completed!=1
		|| !c.polls || c.polls>20000 || c.phase!=0x7000c || c.count!=PioneerNativeDramLoadCount || c.marker!=PioneerNativeDramLoadMarker
		|| (c.controls[0]&1) || c.controls[1] || c.controls[2] || r.result.mismatch_kind || r.result.mismatch_offset)return false;
	for(unsigned pc:c.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
	for(unsigned word:c.output)if(word!=0x7fc00001)return false; // sentinel ONLY,not model values
	return c.held==closed && c.l1_restored==closed && c.tlb_restored==closed && r.result.phase==(closed?10U:5U)
		&& r.result.gddr_restored==closed && r.result.niu_restored==closed && r.result.clock_restored==closed
		&& r.result.requests==320 && r.result.flits==2880 && r.result.saved_l1==52812 && r.result.saved_gddr==46080
		&& r.result.payload_checked==(closed?46080U:0U) && r.result.headers_checked==(closed?2560U:0U);
}
bool Delivered(){return !fflush(stdout) && !ferror(stdout);}
void Unknown(const char* stage,const PioneerNativeDramLoadRequest& r) {
	fprintf(stderr,"NATIVE_LOAD_ONLY_%s_UNPROVEN state%u cold%u phase%u marker%x count%x payload_checked%u headers_checked%u mismatch_kind%u mismatch_offset%x;CLOSE_ONLY_NO_END_RETRY_RESET_RESTORE;NO_PRIVATE_WORD_DUMP\n",
		stage,r.result.compute.state,r.result.compute.cold,r.result.phase,r.result.compute.marker,r.result.compute.count,
		r.result.payload_checked,r.result.headers_checked,r.result.mismatch_kind,r.result.mismatch_offset);
}
}
int main(int argc,char** argv) {
	if(argc!=2 || strcmp(argv[1],"--fixed-load-only-band")){fputs("Use --fixed-load-only-band for one fixed DEBUG load/byte-check,NO FPU/model/Work inference.\n",stderr);return 2;}
	if(geteuid()!=0)return 1;
	PioneerNativeModelBandSource source;std::string error;
	puts("AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET");if(!Delivered())return 1;
	if(!source.Prepare("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",error) || !source.Ready()) {
		fprintf(stderr,"NATIVE_LOAD_ONLY_SOURCE_REFUSED: %s;NO_DEVICE_OPEN\n",error.c_str());return 1;
	}
	puts("AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 sample0;LOAD_ONLY_NO_FPU_MODEL_OR_WORK");if(!Delivered() || !source.Ready())return 1;
	const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
	tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeDramLoadTag;
	if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1 || metadata.expected_tag || metadata.expected_generation
		|| metadata.reserved[0] || metadata.reserved[1] || metadata.reserved[2] || metadata.driver!=55 || metadata.build_tag!=PioneerNativeDramLoadTag
		|| !metadata.generation || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1) {
		fputs("NATIVE_LOAD_ONLY_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;
	}
	PioneerNativeDramLoadRequest r={};r.version=1;r.sample=PioneerNativeDramLoadNone;r.expected_tag=PioneerNativeDramLoadTag;r.expected_generation=metadata.generation;
	if(ioctl(fd,PIONEER_NATIVE_DRAM_LOAD_SUPPORT,&r,sizeof(r)) || !PioneerNativeDramLoadRequestValid(r)
		|| r.sample!=PioneerNativeDramLoadNone || r.nonce || r.accept || r.expected_generation!=metadata.generation || !source.Ready()) {
		fputs("NATIVE_LOAD_ONLY_SUPPORT_SOURCE_REFUSED;NO_RUN\n",stderr);close(fd);return 1;
	}
	r={};r.version=1;r.sample=0;r.expected_tag=PioneerNativeDramLoadTag;r.expected_generation=metadata.generation;
	if(ioctl(fd,PIONEER_NATIVE_DRAM_LOAD_RUN,&r,sizeof(r)) || !Result(r,metadata.generation,PIONEER_NATIVE_BFP_RESULT) || !source.Ready()) {
		Unknown("RUN",r);close(fd);return 1;
	}
	if(!source.Ready()){fputs("NATIVE_LOAD_ONLY_SOURCE_CHANGED;CLOSE_ONLY_NO_END\n",stderr);close(fd);return 1;}
	// ACK requests explicit byte verification/restore from KNOWN NO-MATH park.
	// It does NOT claim payload correctness or acknowledge a numerical result.
	PioneerNativeDramLoadRequest end={};end.version=1;end.sample=0;end.expected_tag=PioneerNativeDramLoadTag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
	if(ioctl(fd,PIONEER_NATIVE_DRAM_LOAD_END,&end,sizeof(end)) || !Result(end,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
		|| memcmp(end.result.compute.output,r.result.compute.output,sizeof(r.result.compute.output))
		|| memcmp(end.result.compute.controls,r.result.compute.controls,sizeof(r.result.compute.controls))
		|| end.result.compute.polls!=r.result.compute.polls || !source.Ready()) {
		Unknown("END",end);close(fd);return 1;
	}
	if(close(fd))return 1;
	printf("PASS_NATIVE_DRAM_LOAD_ONLY sample0 tensor7 width5120 requests320 flits2880 saved_l152812 saved_gddr46080 payload_checked46080 headers_checked2560 polls%u generation%llu;EXPLICIT_END_GDDR_L1_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FPU_MODEL_QUALITY_OR_SPEED_CLAIM\n",
		r.result.compute.polls,(unsigned long long)metadata.generation);
	return Delivered()?0:1;
}
