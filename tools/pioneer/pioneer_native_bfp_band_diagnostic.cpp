/* SPDX-License-Identifier: MIT */
// Separate fixed full-K synthetic diagnostic. NEVER model/Work inference.
#include <Drivers.h>
#if defined(PIONEER_NATIVE_BFP4_TIMED_CLIENT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfp4TimedProtocol.h"
#include "PioneerNativeBfp4BandFixture.h"
#include "PioneerNativeBfp4BandGolden.h"
#include "PioneerNativeBfp4BandTiming.h"
#define PioneerNativeBfpBandTag PioneerNativeBfp4TimedTag
#define PioneerNativeBfpBandRequest PioneerNativeBfp4TimedRequest
#define PioneerNativeBfpBandRequestValid PioneerNativeBfp4TimedRequestValid
#define PioneerNativeBfpBandFixture PioneerNativeBfp4BandFixture
#define PioneerNativeBfpBandGolden PioneerNativeBfp4BandGolden
#define PIONEER_NATIVE_BFP_BAND_SUPPORT PIONEER_NATIVE_BFP4_TIMED_SUPPORT
#define PIONEER_NATIVE_BFP_BAND_RUN PIONEER_NATIVE_BFP4_TIMED_RUN
#define PIONEER_NATIVE_BFP_BAND_END PIONEER_NATIVE_BFP4_TIMED_END
static constexpr unsigned kPhases=2,kMarker=0x5442464e;
#elif defined(PIONEER_NATIVE_BFP4_BAND_CLIENT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfp4BandProtocol.h"
#include "PioneerNativeBfp4BandFixture.h"
#include "PioneerNativeBfp4BandGolden.h"
#define PioneerNativeBfpBandTag PioneerNativeBfp4BandTag
#define PioneerNativeBfpBandRequest PioneerNativeBfp4BandRequest
#define PioneerNativeBfpBandRequestValid PioneerNativeBfp4BandRequestValid
#define PioneerNativeBfpBandFixture PioneerNativeBfp4BandFixture
#define PioneerNativeBfpBandGolden PioneerNativeBfp4BandGolden
#define PIONEER_NATIVE_BFP_BAND_SUPPORT PIONEER_NATIVE_BFP4_BAND_SUPPORT
#define PIONEER_NATIVE_BFP_BAND_RUN PIONEER_NATIVE_BFP4_BAND_RUN
#define PIONEER_NATIVE_BFP_BAND_END PIONEER_NATIVE_BFP4_BAND_END
static constexpr unsigned kPhases=2,kMarker=0x3442464e;
#else
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfpBandProtocol.h"
#include "PioneerNativeBfpBandFixture.h"
#include "PioneerNativeBfpBandGolden.h"
static constexpr unsigned kPhases=4,kMarker=0x3342464e;
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
uint16_t Weight(unsigned tile,int n) {
#if defined(PIONEER_NATIVE_BFP4_BAND_CLIENT) || defined(PIONEER_NATIVE_BFP4_TIMED_CLIENT)
	const unsigned q=PioneerNativeBfp4BandFixture::QuantizedNibble(n),magnitude=q&7;
	if(!magnitude)return 0;
	unsigned top=0;for(unsigned scan=magnitude;scan>>=1;)top++;
	return uint16_t(((q&8)?0x8000:0)|((118+tile%13+top)<<7)|((magnitude<<(7-top))-128));
#else
	(void)tile;
	if(!n)return 0;
	const unsigned magnitude=unsigned(n<0?-n:n);
	unsigned top=0;for(unsigned scan=magnitude;scan>>=1;)top++;
	return uint16_t((n<0?0x8000:0)|((120+top)<<7)|((magnitude<<(7-top))-128));
#endif
}
bool Reference(unsigned width) {
	const auto* golden=PioneerNativeBfpBandGolden(width);if(!golden)return false;
	for(unsigned row=0;row<32;row++) {
		uint32_t total=0;
		for(unsigned tile=0;tile<width/32;tile++)for(unsigned first=0;first<32;first+=16) {
			uint16_t a[16],b[16];for(unsigned k=0;k<16;k++) {
				a[k]=Weight(tile,PioneerNativeBfpBandFixture::Numerator(tile,row,first+k));
				b[k]=PioneerNativeBfpBandFixture::InputBits(tile,first+k);
			}
			for(unsigned term=0;term<kPhases;term++) {
				const unsigned phase=kPhases==2?term*2:term;
				if(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,total,total))return false;
			}
		}
		if(total!=golden[row])return false;
	}
	return true;
}
bool Result(const PioneerNativeBfpBandRequest& r,unsigned width,uint64_t generation,unsigned state) {
	if(r.version!=1 || r.width!=width || r.expected_tag!=PioneerNativeBfpBandTag || r.expected_generation!=generation
		|| r.nonce!=1 || r.reserved || r.accept!=unsigned(state==PIONEER_NATIVE_BFP_CLOSED)
		|| r.result.state!=state || r.result.cold || r.result.started!=1 || r.result.completed!=1
		|| !r.result.polls || r.result.polls>20000 || r.result.phase!=0x7000c
		|| r.result.count!=((kPhases<<24)|width) || r.result.marker!=kMarker)return false;
	for(unsigned pc:r.result.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
#if defined(PIONEER_NATIVE_BFP4_TIMED_CLIENT)
	if(PioneerNativeBfp4BandTiming::State(r.timing,width,0)!=1 || PioneerNativeBfp4BandTiming::State(r.timing+8,width,1)!=1
		|| !PioneerNativeBfp4BandTiming::Later(r.timing+12,r.timing+6))return false;
#endif
	const unsigned restored=unsigned(state==PIONEER_NATIVE_BFP_CLOSED);
	return r.result.held==restored && r.result.l1_restored==restored && r.result.tlb_restored==restored;
}
void Dump(FILE* stream,const PioneerNativeBfpBandRequest& r,unsigned width) {
	const auto* golden=PioneerNativeBfpBandGolden(width);
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)
		fprintf(stream,"NATIVE_BFP_BAND_RETURNED_WORD k%u pass%u row%u got%08x expected%08x\n",width,pass,row,r.result.output[pass*32+row],golden[row]);
}
}
int main(int argc,char** argv) {
	unsigned width=0;
	if(argc==2){if(!strcmp(argv[1],"--fixed-k5120"))width=5120;else if(!strcmp(argv[1],"--fixed-k6144"))width=6144;else if(!strcmp(argv[1],"--fixed-k17408"))width=17408;}
	if(!width){fputs("Use ONE --fixed-k5120, --fixed-k6144 or --fixed-k17408 synthetic diagnostic. NO model/Work inference.\n",stderr);return 2;}
	if(geteuid()!=0)return 1;
	if(!Reference(width)){fputs("NATIVE_BFP_BAND_REFERENCE_CHANGED;NO_DEVICE_OPEN\n",stderr);return 1;}
	int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
	tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeBfpBandTag;
	if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1
		|| metadata.expected_tag || metadata.expected_generation || metadata.reserved[0] || metadata.reserved[1] || metadata.reserved[2]
		|| metadata.driver!=55 || metadata.build_tag!=PioneerNativeBfpBandTag || !metadata.generation
		|| metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1) {
		fputs("NATIVE_BFP_BAND_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;
	}
	PioneerNativeBfpBandRequest r={};r.version=1;r.expected_tag=PioneerNativeBfpBandTag;r.expected_generation=metadata.generation;
	if(ioctl(fd,PIONEER_NATIVE_BFP_BAND_SUPPORT,&r,sizeof(r)) || !PioneerNativeBfpBandRequestValid(r)
		|| r.width || r.nonce || r.accept || r.expected_generation!=metadata.generation) {
		fputs("NATIVE_BFP_BAND_SUPPORT_REFUSED;NO_RUN\n",stderr);close(fd);return 1;
	}
	r={};r.version=1;r.width=width;r.expected_tag=PioneerNativeBfpBandTag;r.expected_generation=metadata.generation;
	if(ioctl(fd,PIONEER_NATIVE_BFP_BAND_RUN,&r,sizeof(r)) || !Result(r,width,metadata.generation,PIONEER_NATIVE_BFP_RESULT)) {
		fprintf(stderr,"NATIVE_BFP_BAND_RUN_UNPROVEN k%u state%u cold%u started%u completed%u polls%u pc%x/%x phase%x count%x marker%x;CLOSE_ONLY_NO_END_OR_RETRY\n",width,
			r.result.state,r.result.cold,r.result.started,r.result.completed,r.result.polls,r.result.pc[0],r.result.pc[1],r.result.phase,r.result.count,r.result.marker);
		close(fd);return 1;
	}
	const auto* golden=PioneerNativeBfpBandGolden(width);
	for(unsigned word=0;word<64;word++)if(r.result.output[word]!=golden[word%32]) {
		fprintf(stderr,"NATIVE_BFP_BAND_NUMERICAL_MISMATCH k%u pass%u row%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",width,word/32,word%32,r.result.output[word],golden[word%32]);
		Dump(stderr,r,width);close(fd);return 1;
	}
	PioneerNativeBfpBandRequest end={};end.version=1;end.width=width;end.expected_tag=PioneerNativeBfpBandTag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
	if(ioctl(fd,PIONEER_NATIVE_BFP_BAND_END,&end,sizeof(end)) || !Result(end,width,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
		|| memcmp(end.result.output,r.result.output,sizeof(r.result.output))
#if defined(PIONEER_NATIVE_BFP4_TIMED_CLIENT)
		|| memcmp(end.timing,r.timing,sizeof(r.timing))
#endif
		) {
		fputs("NATIVE_BFP_BAND_END_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
	}
	if(close(fd))return 1;
	Dump(stdout,r,width);
#if defined(PIONEER_NATIVE_BFP4_TIMED_CLIENT)
	for(unsigned pass=0;pass<2;pass++) {
		const uint32_t* t=r.timing+pass*8;
		uint64_t start=uint64_t(t[4])|(uint64_t(t[5])<<32),finish=uint64_t(t[6])|(uint64_t(t[7])<<32);
		printf("NATIVE_BFP4_BAND_TIMING width%u pass%u start%llu end%llu cycles%llu;CARD_CYCLES_ONLY_L1_INPUTS\n",width,pass,
			(unsigned long long)start,(unsigned long long)finish,(unsigned long long)(finish-start));
	}
#endif
	printf("PASS_NATIVE_BFP_FULL_K bits%u width%u rows32 dual_jobs2 phases%u exact_words64 polls%u generation%llu;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM\n",kPhases==2?4U:8U,width,kPhases,r.result.polls,(unsigned long long)metadata.generation);
	if(fflush(stdout) || ferror(stdout))return 1;
	return 0;
}
