/* SPDX-License-Identifier: MIT */
// NEW fixed synthetic diagnostic. Old ideal Expected()/client/trials unchanged.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfpProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfpFixture.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
// PREEXISTING independent predictions,then all64 physical words in record
//66112229d3b318d6e79864ecf6d45ce3007a32ae44b7b6c939ce7b05c6ccdf36.
// Only this immutable fixture is qualified,not arbitrary model/Work arithmetic.
const uint32_t Golden[32]={
	0x4180b500,0x3f171800,0x402aa000,0x3fa3a200,0x42021500,0xc0841200,0x3ef06000,0x3fad6400,
	0x42796900,0xbfcb6a00,0x4001f200,0xc0617400,0x416ab480,0xc07ece00,0x4051c400,0xbf9e4e00,
	0x41eca440,0xc04dac00,0xbed78000,0x40193800,0x42737500,0xbfad9600,0x3fbd1000,0x41046700,
	0x417f2480,0xc0a0cf00,0xbe93c000,0xbe199000,0x41f80300,0xbf3c1000,0x3f822800,0xc03a2200};
uint16_t Weight(unsigned row,unsigned column) {
	unsigned magnitude=PioneerNativeBfpFixture::Magnitude(8,row,column);if(!magnitude)return 0;
	unsigned top=0;for(unsigned scan=magnitude;scan>>=1;)top++;
	const unsigned exponent=PioneerNativeBfpFixture::Exponent(row)-6+top;
	return uint16_t((PioneerNativeBfpFixture::Negative(row,column)?0x8000:0)|(exponent<<7)|((magnitude<<(7-top))-128));
}
bool Reference() {
	// BEFORE any device open. Integer specification must still agree with the
	// complete independently observed fixture,not merely with itself or a tolerance.
	for(unsigned row=0;row<32;row++) {
		uint32_t total=0;
		for(unsigned first=0;first<32;first+=16) {
			uint16_t a[16],b[16];
			for(unsigned k=0;k<16;k++){a[k]=Weight(row,first+k);b[k]=PioneerNativeBfpFixture::InputBits(first+k);}
			for(unsigned phase=0;phase<4;phase++)if(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,total,total))return false;
		}
		if(total!=Golden[row])return false;
	}
	return true;
}
bool Result(const PioneerNativeBfpRequest& r,uint64_t generation,unsigned state) {
	if(r.version!=1 || r.bits!=8 || r.expected_tag!=PioneerNativeBfpTag || r.expected_generation!=generation
		|| r.nonce!=1 || r.reserved || r.accept!=unsigned(state==PIONEER_NATIVE_BFP_CLOSED)
		|| r.result.state!=state || r.result.cold || r.result.started!=1 || r.result.completed!=1
		|| !r.result.polls || r.result.polls>20000 || r.result.phase!=0x7000c || r.result.count!=2 || r.result.marker!=0x3142464e)return false;
	for(unsigned pc:r.result.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
	const unsigned restored=unsigned(state==PIONEER_NATIVE_BFP_CLOSED);
	return r.result.held==restored && r.result.l1_restored==restored && r.result.tlb_restored==restored;
}
void Dump(FILE* stream,const PioneerNativeBfpRequest& r) {
	// The already returned CPU copy only;NO additional ioctl/device readback.
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)
		fprintf(stream,"NATIVE_BFP8_DOCUMENTED_RETURNED_WORD pass%u row%u got%08x expected%08x\n",
			pass,row,r.result.output[pass*32+row],Golden[row]);
}
}
int main(int argc,char** argv) {
	if(argc!=2 || strcmp(argv[1],"--fixed-bfp8-documented-rounding")) {
		fputs("Use --fixed-bfp8-documented-rounding for ONE new fixed synthetic diagnostic. NO model/Work inference.\n",stderr);return 2;
	}
	if(geteuid()!=0)return 1;
	if(!Reference()){fputs("NATIVE_BFP8_DOCUMENTED_REFERENCE_CHANGED;NO_DEVICE_OPEN\n",stderr);return 1;}
	int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
	tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeBfpTag;
	if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1
		|| metadata.expected_tag!=PioneerNativeBfpTag || metadata.driver!=55 || metadata.build_tag!=PioneerNativeBfpTag
		|| !metadata.generation || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1) {close(fd);return 1;}
	PioneerNativeBfpRequest r={};r.version=1;r.expected_tag=PioneerNativeBfpTag;r.expected_generation=metadata.generation;
	if(ioctl(fd,PIONEER_NATIVE_BFP_SUPPORT,&r,sizeof(r)) || !PioneerNativeBfpRequestValid(r)
		|| r.bits || r.nonce || r.accept || r.expected_generation!=metadata.generation) {close(fd);return 1;}
	r={};r.version=1;r.bits=8;r.expected_tag=PioneerNativeBfpTag;r.expected_generation=metadata.generation;
	if(ioctl(fd,PIONEER_NATIVE_BFP_RUN,&r,sizeof(r)) || !Result(r,metadata.generation,PIONEER_NATIVE_BFP_RESULT)) {
		fputs("NATIVE_BFP8_DOCUMENTED_RUN_UNPROVEN;CLOSE_ONLY_NO_END_OR_RETRY\n",stderr);close(fd);return 1;
	}
	for(unsigned word=0;word<64;word++)if(r.result.output[word]!=Golden[word%32]) {
		fprintf(stderr,"NATIVE_BFP8_DOCUMENTED_NUMERICAL_MISMATCH pass%u row%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",
			word/32,word%32,r.result.output[word],Golden[word%32]);Dump(stderr,r);close(fd);return 1;
	}
	PioneerNativeBfpRequest end={};end.version=1;end.bits=8;end.expected_tag=PioneerNativeBfpTag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
	if(ioctl(fd,PIONEER_NATIVE_BFP_END,&end,sizeof(end)) || !Result(end,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
		|| memcmp(end.result.output,r.result.output,sizeof(r.result.output))) {
		fputs("NATIVE_BFP8_DOCUMENTED_END_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
	}
	if(close(fd))return 1;
	Dump(stdout,r);
	printf("PASS_NATIVE_BFP8_DOCUMENTED_FIXED_KERNEL bits8 rows32 dual_jobs2 exact_words64 polls%u generation%llu;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_K_MODEL_QUALITY_OR_SPEED_CLAIM\n",r.result.polls,(unsigned long long)metadata.generation);
	if(fflush(stdout) || ferror(stdout))return 1;
	return 0;
}
