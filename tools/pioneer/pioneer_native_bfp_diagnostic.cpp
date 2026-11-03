/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfpProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfpFixture.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
namespace {
uint32_t Bits(float value) { uint32_t b;memcpy(&b,&value,4);return b; }
float Float(uint32_t bits) { float v;memcpy(&v,&bits,4);return v; }
uint32_t Expected(unsigned bits,unsigned row) {
	long double sum=0;
	for(unsigned k=0;k<32;k++) {
		const unsigned magnitude=PioneerNativeBfpFixture::Magnitude(bits,row,k);
		long double weight=std::ldexp(static_cast<long double>(magnitude),int(PioneerNativeBfpFixture::Exponent(row))-127-int(bits-2));
		if(PioneerNativeBfpFixture::Negative(row,k))weight=-weight;
		sum+=weight*Float(uint32_t(PioneerNativeBfpFixture::InputBits(k))<<16);
	}
	return Bits(float(sum)); // exact finite dyadic fixture, not a model/Work CPU product
}
bool Result(const PioneerNativeBfpRequest& r,unsigned bits,uint64_t generation,unsigned state) {
	if(r.version!=1 || r.bits!=bits || r.expected_tag!=PioneerNativeBfpTag || r.expected_generation!=generation
		|| r.nonce!=1 || r.result.state!=state || r.result.cold || !r.result.started || !r.result.completed
		|| !r.result.polls || r.result.polls>20000 || r.result.phase!=0x7000c || r.result.count!=2 || r.result.marker!=0x3142464e)return false;
	for(unsigned pc:r.result.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
	return state==PIONEER_NATIVE_BFP_RESULT?(!r.result.held && !r.result.l1_restored && !r.result.tlb_restored)
		:(r.result.held && r.result.l1_restored && r.result.tlb_restored);
}
void DumpReturnedWords(const PioneerNativeBfpRequest& r,unsigned bits) {
	// CPU copy already returned by the ONE RUN, not another device query.
	// Preserve every word for diagnosis, without accepting the mismatch or
	// issuing END/reset/retry/readback against the still-owned parked tile.
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)
		fprintf(stderr,"NATIVE_BFP_RETURNED_WORD bits%u pass%u row%u got%08x expected%08x\n",
			bits,pass,row,r.result.output[pass*32+row],Expected(bits,row));
}
}
int main(int argc,char** argv) {
	if(argc!=2 || (strcmp(argv[1],"--fixed-bfp4") && strcmp(argv[1],"--fixed-bfp8"))) {
		fputs("Use --fixed-bfp4 or --fixed-bfp8 for ONE isolated synthetic native tile diagnostic. NO model inference.\n",stderr);return 2;
	}
	const unsigned bits=strcmp(argv[1],"--fixed-bfp4")?8:4;
	if(geteuid()!=0)return 1;
	int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
	tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=PioneerNativeBfpTag;
	if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.driver!=55 || metadata.build_tag!=PioneerNativeBfpTag
		|| !metadata.generation || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1) {close(fd);return 1;}
	PioneerNativeBfpRequest r={};r.version=1;r.expected_tag=PioneerNativeBfpTag;r.expected_generation=metadata.generation;
	if(ioctl(fd,PIONEER_NATIVE_BFP_SUPPORT,&r,sizeof(r)) || !PioneerNativeBfpEmptyResult(r.result)) {close(fd);return 1;}
	r={};r.version=1;r.bits=bits;r.expected_tag=PioneerNativeBfpTag;r.expected_generation=metadata.generation;
	if(ioctl(fd,PIONEER_NATIVE_BFP_RUN,&r,sizeof(r)) || !Result(r,bits,metadata.generation,PIONEER_NATIVE_BFP_RESULT)) {
		fprintf(stderr,"NATIVE_BFP_RUN_UNPROVEN state%u cold%u started%u completed%u polls%u pc%x/%x phase%x count%u marker%x;CLOSE_ONLY_NO_END_OR_RETRY\n",
			r.result.state,r.result.cold,r.result.started,r.result.completed,r.result.polls,r.result.pc[0],r.result.pc[1],r.result.phase,r.result.count,r.result.marker);
		close(fd);return 1;
	}
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)if(r.result.output[pass*32+row]!=Expected(bits,row)) {
		fprintf(stderr,"NATIVE_BFP_NUMERICAL_MISMATCH bits%u pass%u row%u got%08x expected%08x;CLOSE_ONLY_NO_END_OR_RETRY\n",bits,pass,row,r.result.output[pass*32+row],Expected(bits,row));
		DumpReturnedWords(r,bits);
		close(fd);return 1;
	}
	PioneerNativeBfpRequest end={};end.version=1;end.bits=bits;end.expected_tag=PioneerNativeBfpTag;end.expected_generation=metadata.generation;end.nonce=1;end.accept=1;
	if(ioctl(fd,PIONEER_NATIVE_BFP_END,&end,sizeof(end)) || !Result(end,bits,metadata.generation,PIONEER_NATIVE_BFP_CLOSED)
		|| memcmp(end.result.output,r.result.output,sizeof(r.result.output))) {
		fputs("NATIVE_BFP_END_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
	}
	if(close(fd))return 1;
	printf("PASS_NATIVE_BFP_FIXED_KERNEL bits%u rows32 dual_jobs2 exact_words64 polls%u generation%llu;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM\n",bits,r.result.polls,(unsigned long long)metadata.generation);return 0;
}
