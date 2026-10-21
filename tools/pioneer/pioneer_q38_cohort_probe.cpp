/* SPDX-License-Identifier: MIT */
// Explicit DEBUG primitive qualification,not model inference/Assistant mode.
// Fixed synthetic/authenticated samples ONLY. No arbitrary address/code args.
#include <Drivers.h>
#include <OS.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <memory>
#include <vector>
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CohortProtocol.h"
#include "PioneerQwen38Panel.h"
#include "PioneerQ38RowFixtures.h"
#ifdef PIONEER_Q38_SENTINEL_REFERENCE_CLIENT
#if !defined(PIONEER_Q38_ROOT_CYCLE_CLIENT) || defined(PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT) || defined(PIONEER_Q38_GROUP32_REFERENCE_CLIENT) || defined(PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT) || defined(PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT)
#error Sentinel reference requires separate exact-tag original-TC Work diagnostic
#endif
#include "PioneerQ38SentinelDebugTag.h"
#endif
#ifdef PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT
#if !defined(PIONEER_Q38_ROOT_CYCLE_CLIENT) || defined(PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT) || defined(PIONEER_Q38_GROUP32_REFERENCE_CLIENT) || defined(PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT)
#error Direct SrcB reference requires separate exact-tag original-TC Work diagnostic
#endif
#include "PioneerQ38DirectSrcBDebugTag.h"
#endif
#ifdef PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT
#if !defined(PIONEER_Q38_ROOT_CYCLE_CLIENT) || defined(PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT) || defined(PIONEER_Q38_GROUP32_REFERENCE_CLIENT)
#error Affine/raw-dot reference requires separate exact-tag original-TC Work diagnostic
#endif
#include "PioneerQ38AffineRawDotDebugTag.h"
#endif
#ifdef PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT
#if !defined(PIONEER_Q38_ROOT_CYCLE_CLIENT) || defined(PIONEER_Q38_GROUP32_REFERENCE_CLIENT)
#error Native Q8 reference requires its separate tagged Work diagnostic
#endif
#include "PioneerQ38NativeQ8Data.h"
#include "PioneerQ38NativeQ8DebugTag.h"
#endif
#ifdef PIONEER_Q38_GROUP32_REFERENCE_CLIENT
#ifndef PIONEER_Q38_ROOT_CYCLE_CLIENT
#error Group32 reference is only the separately tagged Work diagnostic
#endif
#include "PioneerQ38Group32Reference.h"
#include "PioneerQ38Group32DebugTag.h"
#endif
#ifdef PIONEER_Q38_DISPATCH_CLIENT
#include "PioneerQ38DispatchHost.h"
using Q38Probe=tt_q38_dispatch_probe;
#else
using Q38Probe=tt_q38_cohort_probe;
static uint32_t PioneerAttentionBits(float value) { uint32_t bits; memcpy(&bits,&value,4); return bits; }
static float PioneerAttentionFloat(uint32_t bits) { float value; memcpy(&value,&bits,4); return value; }
#endif
static bool Call(int fd,uint32_t op,void* data,size_t bytes)
{
	if (!ioctl(fd,op,data,bytes)) return true;
	fprintf(stderr,"STOP:op=%08x errno=%d %s;NO_RETRY\n",op,errno,strerror(errno)); return false;
}
static double Seconds()
{ timespec now={}; if (clock_gettime(CLOCK_MONOTONIC,&now)) return -1; return double(now.tv_sec)+double(now.tv_nsec)*1e-9; }
static bool BulkVerified(const tt_q38_bank_bulk& b)
{
	const auto& t=b.result.transfer;
	bool okay=b.result.pages==b.pages && b.result.host_words==b.pages*1024 && b.result.host_ready && b.result.l1_prepared
		&& t.version==1 && t.state==TT_STORE_PASSED && t.requests==6 && t.completed==6 && t.wrote_device
		&& t.host_write_issued && t.host_visible && t.readback_complete && !t.mismatches && !t.recovery_required;
	for (unsigned step=0;step<6;step++) for (unsigned i=0;i<4;i++) okay &= uint32_t(t.after[step][i]-t.before[step][i])==(i==1?b.pages*64:1);
	return okay;
}
static bool Upload(int fd,unsigned bank,uint64_t offset,const std::vector<uint8_t>& data)
{
	unsigned pages=(unsigned(data.size())+4095)/4096;
	for (unsigned p=0;p<pages;) {
		tt_q38_bank_bulk r={}; r.version=1; r.bank=bank; r.pages=pages-p<4?pages-p:4; r.offset=offset+uint64_t(p)*4096;
		unsigned copy=unsigned(data.size())-p*4096; if (copy>r.pages*4096) copy=r.pages*4096;
		memcpy(r.data,data.data()+p*4096,copy);
		if (!Call(fd,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&r,sizeof(r)) || !BulkVerified(r)) return false;
		p+=r.pages;
	}
	return true;
}
static bool Reference(const Q38Probe& r,const std::vector<uint8_t>& packed,unsigned worker,float* output)
{
#ifdef PIONEER_Q38_GROUP32_REFERENCE_CLIENT
	if(r.type==12)return PioneerQ38Group32Reference(r,packed,worker,output);
#endif
	unsigned count=PioneerQ38CohortBlockInputs(r.type),blocks=r.inputs/count,tileBytes=PioneerQ38PanelTileBytes(r.type);
	for (unsigned g=0;g<r.part[worker].groups;g++) for (unsigned b=0;b<blocks;b++) for (unsigned p=0;p<count/16;p++) {
		PioneerQ38Panel panel; if (!PioneerQ38UnpackPanel(r.type,packed.data()+(g*blocks+b)*tileBytes,tileBytes,p,&panel)) return false;
		int sum=0;
		for (unsigned k=0;k<16;k++) { unsigned at=b*count+p*16+k; uint16_t a=uint16_t(r.activation[at/2]>>((at%2)*16)); sum+=int(a&1023)*((a&0x8000)?-1:1); }
		for (unsigned row=0;row<16;row++) {
			int dot=0;
			for (unsigned k=0;k<16;k++) {
				unsigned e=k*16+row,at=b*count+p*16+k; uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16)),a=uint16_t(r.activation[at/2]>>((at%2)*16));
				dot+=int(w&1023)*((w&0x8000)?-1:1)*int(a&1023)*((a&0x8000)?-1:1);
			}
			float scale=PioneerAttentionFloat(r.scales[b]);
			float product=(PioneerAttentionFloat(panel.coefficient[row])*scale)*float(dot),correction=(PioneerAttentionFloat(panel.minimum[row])*scale)*float(sum);
			output[g*16+row]+=product+correction;
		}
	}
	return true;
}
#ifdef PIONEER_Q38_DISPATCH_CLIENT
// DEBUG ONLY:inspect the ALREADY returned public-fixture host copy. This
// must never issue an ioctl/read a card or substitute a numeric result.
static unsigned ReturnedNumericErrors(const Q38Probe& r,const std::vector<float>& expected,FILE* diagnostics)
{
	if (!r.peers || r.peers>119 || expected.size()!=size_t(r.peers)*256) return ~0U;
	unsigned errors=0;
	for (unsigned w=0;w<r.peers;w++) for (unsigned row=0;row<256;row++) {
		uint32_t actual=r.output[w][row],wanted=PioneerAttentionBits(expected[w*256+row]);
		if (actual==wanted) continue;
		if (diagnostics && errors<8) fprintf(diagnostics,
			"STOP:DISPATCH_NUMERIC worker%u row%u actual%08x expected%08x;HOST_COPY_ONLY_NO_ADDITIONAL_CARD_IO\n",
			w,row,actual,wanted);
		errors++;
	}
	return errors;
}
#endif
static bool Execute(int fd,unsigned type,unsigned inputs,unsigned workers,unsigned groups,unsigned ordinal,const std::vector<uint8_t>& publicRows,const unsigned* rowOffsets
#ifdef PIONEER_Q38_DISPATCH_CLIENT
	,PioneerQ38DispatchHistory& history
#endif
)
{
	auto r=std::make_unique<Q38Probe>(); *r={}; r->type=type; r->inputs=inputs;
#ifdef PIONEER_Q38_DISPATCH_CLIENT
	r->version=1; r->peers=workers;
#else
	r->version=TT_Q38_COHORT_ABI; r->workers=workers;
#endif
	unsigned count=PioneerQ38CohortBlockInputs(type),blocks=inputs/count,tileBytes=PioneerQ38PanelTileBytes(type),groupBytes=blocks*tileBytes;
	for (unsigned k=0;k<inputs;k++) r->activation[k/2]|=uint32_t(PioneerInt8Operand(int((k*29+17)%256)-128))<<((k%2)*16);
	for (unsigned b=0;b<blocks;b++) r->scales[b]=PioneerAttentionBits(b%3?-.03125f:.012345f);
	std::vector<float> expected(workers*256,0); double start=Seconds(); if (start<0) return false;
	for (unsigned w=0;w<workers;w++) {
		unsigned head=64+(w%4)*64; uint64_t offset=uint64_t(w/7)*2097152+head;
		r->part[w]={w%7,groups,offset};
		std::vector<uint8_t> packed(groupBytes*groups,0),upload(head+groupBytes*groups,0);
		if (!publicRows.empty()) {
			for (unsigned g=0;g<groups;g++) {
				unsigned at=rowOffsets[(ordinal&~1U)+((ordinal+g+w)%2)];
				if (at>publicRows.size() || groupBytes>publicRows.size()-at) return false;
				memcpy(packed.data()+g*groupBytes,publicRows.data()+at,groupBytes);
			}
		} else for (unsigned b=0;b<blocks*groups;b++) for (unsigned row=0;row<16;row++) {
			unsigned raw=type==8?34:type==12?144:210; uint8_t* tile=packed.data()+b*tileBytes;
			for (unsigned byte=0;byte<raw;byte++) tile[byte*16+row]=uint8_t(byte*37+row*59+b*13+w*23);
			unsigned d=type==14?208:0; uint16_t half=row%3?uint16_t(0x3c00+row*13):uint16_t(1+row);
			tile[d*16+row]=uint8_t(half); tile[(d+1)*16+row]=uint8_t(half>>8);
			if (type==12) { tile[32+row]=0; tile[48+row]=0x38; }
			if (type==8) {
#ifdef PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT
				// NEW diagnostic domain,not clipping any model/public fixture.
				for(unsigned byte=2;byte<34;byte++)tile[byte*16+row]=uint8_t(int((byte*37+row*59+b*13+w*23)%255)-127);
#else
				tile[32+row]=128; // ordinary checked/TC path still tests -128
#endif
			}
		}
		if (!Reference(*r,packed,w,expected.data()+w*256)) return false;
		memcpy(upload.data()+head,packed.data(),packed.size());
#ifdef PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT
		if(type==8) {
			// CPU oracle above uses ORIGINAL TC bytes. Transform ONLY the
			// private not-yet-uploaded wire copy;never substitute a result.
			for(uint64_t byte=0;byte<packed.size();byte+=4096) {
				uint8_t input[4096]={},encoded[4096];unsigned count=unsigned(packed.size()-byte<4096?packed.size()-byte:4096);
				memcpy(input,packed.data()+byte,count);
				if(!PioneerQ38NativeQ8Data::PreparePage(input,byte,count,encoded,sizeof(encoded)))return false;
				memcpy(upload.data()+head+byte,encoded,count);
			}
		}
#endif
		if (!Upload(fd,w%7,offset-head,upload)) return false;
	}
	double launched=Seconds(); if (launched<start) return false;
#ifdef PIONEER_Q38_DISPATCH_CLIENT
	double previous=launched;
	for (unsigned repeat=0;repeat<2;repeat++) {
		bool executed=Call(fd,TT_PCI_PROBE_Q38_DISPATCH_RUN,r.get(),sizeof(*r));
		if (!executed || !history.Accept(*r)) {
			// ALREADY returned ordinary host memory ONLY. Never issue a
			// second ioctl/register read,END,reset or retry after uncertainty.
			const auto& v=r->result;
			fprintf(stderr,"STOP:DISPATCH_RETURNED type%u K%u peers%u repeat%u ioctl%u state%u cold%u root_phase%08x count%u marker%08x pc%08x/%08x nonce%u started%u completed%u held%u;HOST_COPY_ONLY_NO_ADDITIONAL_CARD_IO\n",
				type,inputs,workers,repeat,unsigned(executed),v.state,v.recovery_required,v.root_phase,v.root_count,v.root_marker,
				v.root_pc[0],v.root_pc[1],v.nonce,v.root_started,v.root_completed,v.root_held);
			return false;
		}
		double finish=Seconds(); if (finish<previous) return false;
#ifdef PIONEER_Q38_ROOT_CYCLE_CLIENT
#ifdef PIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT
		if (!PioneerQ38PeerPhaseEnvelopeValid(r->result.stats)) return false;
		printf("PEER_DEVICE_PHASE_CYCLES type%u K%u peers%u repeat%u valid%u fetch_scale%llu unpack_metadata%llu matrix_affine%llu;SELECTED_MAX_GROUP_PEER_BLOCK_REGIONS_INCLUDE_INSTRUMENTATION_AND_OVERLAP_NOT_PURE_UNITS_OR_TOKEN_RATE\n",
			type,inputs,workers,repeat,unsigned(r->result.stats[57]==PioneerQ38PeerPhaseValid),
			(unsigned long long)PioneerQ38PeerPhaseValue(r->result.stats,0),(unsigned long long)PioneerQ38PeerPhaseValue(r->result.stats,1),
			(unsigned long long)PioneerQ38PeerPhaseValue(r->result.stats,2));
#else
		if (!PioneerQ38RootCycleDataValid(r->result.stats)) return false;
		printf("ROOT_DEVICE_CYCLES type%u K%u peers%u repeat%u prepare%llu wait_and_audit%llu gather_dma_proof%llu;ACTUAL_CARD_COUNTER_NOT_PURE_MATRIX_OR_TOKEN_RATE\n",
			type,inputs,workers,repeat,(unsigned long long)PioneerQ38RootCyclePhase(r->result.stats,0),
			(unsigned long long)PioneerQ38RootCyclePhase(r->result.stats,1),(unsigned long long)PioneerQ38RootCyclePhase(r->result.stats,2));
#endif
#ifdef PIONEER_Q38_SENTINEL_REFERENCE_CLIENT
		printf("ROOT_RETURNED_COUNTS type%u K%u peers%u repeat%u read_packets%u write_packets%u read_flits%u write_flits%u peer_polls%u terminal_audits%u;ALREADY_RETURNED_HOST_COPY_ONLY_NO_ADDITIONAL_CARD_IO_NOT_SPEED\n",
			type,inputs,workers,repeat,r->result.stats[6],r->result.stats[7],r->result.stats[8],r->result.stats[9],r->result.stats[10],r->result.stats[11]);
#endif
#endif
		unsigned errors=ReturnedNumericErrors(*r,expected,stderr);
		printf("DISPATCH_DEBUG type%u K%u peers%u rows_per_peer%u repeat%u nonce%u waiting%u cold%u exact_errors%u checked_call_s%.6f;CPU_REFERENCE_DEBUG_ONLY_NOT_MODEL_OR_TOKEN_RATE\n",
			type,inputs,workers,groups*16,repeat,r->result.nonce,r->result.waiting,r->result.recovery_required,errors,finish-previous);
		if (errors) return false;
		previous=finish;
	}
	return true;
#else
	printf("COHORT_RUN_BEGIN type=%u K=%u workers=%u N_per_worker=%u;ONE_ROOT_INPUT;NOT_INFERENCE\n",type,inputs,workers,groups*16);
	if (!Call(fd,TT_PCI_PROBE_Q38_COHORT_RUN,r.get(),sizeof(*r))) return false;
	double finish=Seconds(); if (finish<launched) return false;
	const auto& result=r->result;
	bool okay=result.version==1 && result.state==TT_RUNTIME_PASSED && result.workers==workers && result.started==workers
		&& result.completed==workers && result.stopped==workers && !result.recovery_required;
	unsigned errors=0;
	for (unsigned w=0;w<workers;w++) {
		const auto& v=result.core[w]; unsigned reads=groups*blocks,flits=groupBytes*groups/64;
		if (w) { reads+=(inputs*2+16383)/16384+1; flits+=(inputs*2+((blocks*4+63)&~63U))/64; }
		bool proof=v.version==1 && v.state==TT_RUNTIME_PASSED && v.prepared && v.context_reset && v.started && v.completed && v.stopped
			&& !v.recovery_required && v.phase==0x7200c && v.panels==groups*(inputs/16) && v.marker==0x31483851 && v.pc[0]==0x72038 && v.pc[1]==v.pc[0];
		for (unsigned c=0;c<8;c++) proof &= uint32_t(v.after[c]-v.before[c])==(c==1?flits:(c==0 || c==2 || c==3 || c==6)?reads:0);
		for (unsigned row=0;row<256;row++) errors+=r->output[w][row]!=PioneerAttentionBits(expected[w*256+row]);
		if (!proof) printf("COHORT_BAD_WORKER%u state=%u prep=%u arm=%u start/complete/stop=%u/%u/%u cold=%u phase=%x panels=%u marker=%x pc=%x/%x\n",
			w,v.state,v.prepared,v.context_reset,v.started,v.completed,v.stopped,v.recovery_required,v.phase,v.panels,v.marker,v.pc[0],v.pc[1]);
		okay &= proof;
	}
	printf("COHORT_PRIMITIVE type=%u K=%u workers=%u N_per_worker=%u start/complete/stop=%u/%u/%u cold=%u exact_errors=%u checked_call_s=%.6f prep_reference_upload_s=%.6f;NOT_TOKEN_RATE\n",
		type,inputs,workers,groups*16,result.started,result.completed,result.stopped,result.recovery_required,errors,finish-launched,launched-start);
	return okay && !errors;
#endif
}
int main(int argc,char** argv)
{
	if (argc!=2) return 2;
	bool supportOnly=!strcmp(argv[1],"--support"),real=false; unsigned workers=0;
	if (!strcmp(argv[1],"--synthetic")) workers=2;
	else if (!strcmp(argv[1],"--synthetic-seven")) workers=7;
#ifdef PIONEER_Q38_DISPATCH_CLIENT
	else if (!strcmp(argv[1],"--synthetic-119")) workers=119;
#else
	else if (!strcmp(argv[1],"--synthetic-120")) workers=120;
#endif
	else if (!strcmp(argv[1],"--public-fixtures")) { real=true; workers=2; }
	else if (!strcmp(argv[1],"--public-fixtures-seven")) { real=true; workers=7; }
#ifdef PIONEER_Q38_DISPATCH_CLIENT
	else if (!strcmp(argv[1],"--public-fixtures-119")) { real=true; workers=119; }
#else
	else if (!strcmp(argv[1],"--public-fixtures-120")) { real=true; workers=120; }
#endif
	else if (!supportOnly) return 2;
	setvbuf(stdout,NULL,_IOLBF,0); std::vector<uint8_t> publicRows;
	if (real) {
		publicRows.resize(PIONEER_Q38_ROW_FIXTURE_BYTES);
		if (!PioneerQ38LoadRowFixtures(publicRows.data(),unsigned(publicRows.size()))) { fprintf(stderr,"STOP:PUBLIC_ROW_MEMORY_SHA_REQUIRED_BEFORE_OPEN\n"); return 1; }
#ifdef PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT
		// First TWO fixtures are fixed K5120 Q8 tiles. Prove their source
		// domain BEFORE accelerator open/BEGIN,not midway through uploads.
		const unsigned q8Bytes=2*(5120/32)*576;
		if(publicRows.size()<q8Bytes)return 1;
		for(unsigned byte=0;byte<q8Bytes;byte++)if(byte%576>=32 && byte%576<544 && publicRows[byte]==128) {
			puts("STOP_NATIVE_Q8_FIXTURE_MINUS128_BEFORE_OPEN;NO_ACCELERATOR_FD");return 1;
		}
#endif
		puts("PUBLIC_ROW_MEMORY_SHA_PASS_725504_BYTES;INTERLEAVED_GROUPS_NOT_FULL_MODEL");
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC); if (fd<0) return 1;
#ifdef PIONEER_Q38_SENTINEL_REFERENCE_CLIENT
	tt_pioneer_driver_reload sentinelTag={};sentinelTag.version=1;
	if(!Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&sentinelTag,sizeof(sentinelTag)) || !PioneerQ38SentinelDebugTag(sentinelTag)) {
		close(fd);
#ifdef PIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT
		puts("STOP_PEER_PHASE_REQUIRES_EXACT_IDLE_TAG25_ABI4;NO_BEGIN_OR_CARD_READ");
#elif defined(PIONEER_Q38_COMMAND_REFERENCE_CLIENT)
		puts("STOP_COMMAND_REUSE_REQUIRES_EXACT_IDLE_TAG24;NO_BEGIN_OR_CARD_READ");
#elif defined(PIONEER_Q38_PREFETCH_REFERENCE_CLIENT)
		puts("STOP_PREFETCH_REQUIRES_EXACT_IDLE_TAG23;NO_BEGIN_OR_CARD_READ");
#elif defined(PIONEER_Q38_RAW_CACHE_REFERENCE_CLIENT)
		puts("STOP_RAW_CACHE_REQUIRES_EXACT_IDLE_TAG22;NO_BEGIN_OR_CARD_READ");
#elif defined(PIONEER_Q38_FRONTEND_REFERENCE_CLIENT)
		puts("STOP_FRONTEND_REQUIRES_EXACT_IDLE_TAG21;NO_BEGIN_OR_CARD_READ");
#elif defined(PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT)
		puts("STOP_BATCH_RAW_REQUIRES_EXACT_IDLE_TAG20;NO_BEGIN_OR_CARD_READ");
#elif defined(PIONEER_Q38_BATCH4_REFERENCE_CLIENT)
		puts("STOP_BATCH4_REQUIRES_EXACT_IDLE_TAG19;NO_BEGIN_OR_CARD_READ");
#elif defined(PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT)
		puts("STOP_HOST_PHASE_REQUIRES_EXACT_IDLE_TAG18;NO_BEGIN_OR_CARD_READ");
#else
		puts("STOP_SENTINEL_REQUIRES_EXACT_IDLE_TAG17;NO_BEGIN_OR_CARD_READ");
#endif
		return 1;
	}
#ifdef PIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT
	puts("PEER_PHASE_DEBUG_EXACT_TAG25_ABI4;OPAQUE_PRIVATE_TIMING_DATA;ORIGINAL_TC_SOURCE_CPU_REFERENCE;NOT_MODEL_OR_WORK_INFERENCE");
#elif defined(PIONEER_Q38_COMMAND_REFERENCE_CLIENT)
	puts("COMMAND_REUSE_DEBUG_EXACT_TAG24;ORIGINAL_TC_SOURCE_CPU_REFERENCE;EXTRA_RETURN_TO_Q8_AFTER_OTHER_TYPES;NOT_MODEL_OR_WORK_INFERENCE");
#elif defined(PIONEER_Q38_FRONTEND_REFERENCE_CLIENT)
	puts("FRONTEND_DEBUG_EXACT_TAG21;ORIGINAL_TC_SOURCE_CPU_REFERENCE;EXTRA_RETURN_TO_Q8_AFTER_OTHER_TYPES;NOT_MODEL_OR_WORK_INFERENCE");
#elif defined(PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT)
	puts("BATCH_RAW_DEBUG_EXACT_TAG20;ORIGINAL_TC_SOURCE_CPU_REFERENCE;EXTRA_RETURN_TO_Q8_AFTER_OTHER_TYPES;NOT_MODEL_OR_WORK_INFERENCE");
#elif defined(PIONEER_Q38_BATCH4_REFERENCE_CLIENT)
	puts("BATCH4_DEBUG_EXACT_TAG19;ORIGINAL_TC_SOURCE_CPU_REFERENCE;EXTRA_RETURN_TO_Q8_AFTER_OTHER_TYPES;NOT_MODEL_OR_WORK_INFERENCE");
#elif defined(PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT)
	puts("HOST_PHASE_DEBUG_EXACT_TAG18;ORIGINAL_TC_SOURCE_CPU_REFERENCE;EXTRA_RETURN_TO_Q8_AFTER_OTHER_TYPES;NOT_MODEL_OR_WORK_INFERENCE");
#else
	puts("SENTINEL_DEBUG_EXACT_TAG17;ORIGINAL_TC_SOURCE_CPU_REFERENCE;EXTRA_RETURN_TO_Q8_AFTER_OTHER_TYPES;NOT_MODEL_OR_WORK_INFERENCE");
#endif
#endif
#ifdef PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT
	tt_pioneer_driver_reload directTag={};directTag.version=1;
	if(!Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&directTag,sizeof(directTag)) || !PioneerQ38DirectSrcBDebugTag(directTag)) {
		close(fd);puts("STOP_DIRECT_SRCB_REQUIRES_EXACT_IDLE_TAG16;NO_BEGIN_OR_CARD_READ");return 1;
	}
	puts("DIRECT_SRCB_DEBUG_EXACT_TAG16;ORIGINAL_TC_SOURCE_CPU_REFERENCE;EXTRA_RETURN_TO_Q8_AFTER_OTHER_TYPES;NOT_MODEL_OR_WORK_INFERENCE");
#endif
#ifdef PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT
	tt_pioneer_driver_reload affineTag={};affineTag.version=1;
	if(!Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&affineTag,sizeof(affineTag)) || !PioneerQ38AffineRawDotDebugTag(affineTag)) {
		close(fd);puts("STOP_AFFINE_RAWDOT_REQUIRES_EXACT_IDLE_TAG15;NO_BEGIN_OR_CARD_READ");return 1;
	}
	puts("AFFINE_RAWDOT_DEBUG_EXACT_TAG15;ORIGINAL_TC_SOURCE_CPU_REFERENCE;EXTRA_RETURN_TO_Q8_AFTER_OTHER_TYPES;NOT_MODEL_OR_WORK_INFERENCE");
#endif
#ifdef PIONEER_Q38_NATIVE_Q8_REFERENCE_CLIENT
	tt_pioneer_driver_reload nativeTag={};nativeTag.version=1;
	if(!Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&nativeTag,sizeof(nativeTag)) || !PioneerQ38NativeQ8DebugTag(nativeTag)) {
		close(fd);puts("STOP_NATIVE_Q8_REQUIRES_EXACT_IDLE_TAG14;NO_BEGIN_OR_CARD_READ");return 1;
	}
	puts("NATIVE_Q8_DEBUG_EXACT_TAG14_BOUND;ORIGINAL_TC_CPU_REFERENCE_SEPARATE_FROM_SM7_WIRE;NOT_MODEL_OR_WORK_INFERENCE");
#endif
#ifdef PIONEER_Q38_GROUP32_REFERENCE_CLIENT
	tt_pioneer_driver_reload tag={};tag.version=1;
	if(!Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&tag,sizeof(tag)) || !PioneerQ38Group32DebugTag(tag)) {
		close(fd);puts("STOP GROUP32_DEBUG_REQUIRES_EXACT_IDLE_DRIVER_TAG;NO_BEGIN_OR_CARD_READ");return 1;
	}
	puts("GROUP32_DEBUG_EXACT_TAG_BOUND;CHANGED_FP32_ROUNDING_ORACLE_ONLY;NOT_MODEL_QUALITY_OR_WORK_INFERENCE");
#endif
#ifdef PIONEER_Q38_DISPATCH_CLIENT
	tt_q38_dispatch_support support={};
#ifndef PIONEER_Q38_DISPATCH_SUPPORT_VERSION
#define PIONEER_Q38_DISPATCH_SUPPORT_VERSION 1
#endif
	bool okay=Call(fd,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,&support,sizeof(support)) && support.driver==TT_Q38_DISPATCH_DRIVER && support.version==PIONEER_Q38_DISPATCH_SUPPORT_VERSION && support.peers==119
		&& support.rows==256 && support.max_inputs==17408 && support.banks==7 && support.offset_alignment==64 && support.persistent==1 && !support.model_inference;
	printf("DISPATCH_METADATA driver%u ABI%u peers%u persistent%u inference%u accepted%u;NO_BEGIN_YET\n",
		support.driver,support.version,support.peers,support.persistent,support.model_inference,unsigned(okay));
	PioneerQ38DispatchHistory history;
#else
	tt_q38_cohort_support support={};
	bool okay=Call(fd,TT_PCI_PROBE_Q38_COHORT_SUPPORT,&support,sizeof(support)) && support.driver==TT_Q38_COHORT_DRIVER && support.version==TT_Q38_COHORT_ABI && support.workers==120
		&& support.rows==256 && support.max_inputs==17408 && support.banks==7 && support.offset_alignment==64 && !support.model_inference;
	printf("COHORT_METADATA observed_driver=%u ABI=%u workers=%u rows=%u max_K=%u banks=%u alignment=%u inference=%u accepted=%u;NO_BEGIN_YET\n",
		support.driver,support.version,support.workers,support.rows,support.max_inputs,support.banks,support.offset_alignment,support.model_inference,unsigned(okay));
#endif
	if (!okay || supportOnly) { close(fd); return okay?0:1; }
	tt_q38_bank_support storage={};
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&storage,sizeof(storage)) && storage.driver==45 && storage.version==1 && storage.banks==7
		&& storage.page_bytes==4096 && storage.max_pages==4 && storage.storage_only==1 && storage.bytes_per_bank==PioneerResidentBanks::Capacity;
	if (!okay) { close(fd); return 1; }
	tt_q38_bank_session session={}; session.version=1;
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) && session.result.version==1 && session.result.state==TT_RESIDENT_SESSION_READY
		&& session.result.active && session.result.host_ready && !session.result.cold && !session.result.capabilities && session.result.bytes==PioneerResidentBanks::Total;
	const unsigned kinds[]={8,8,12,12,14,14,12,12},widths[]={5120,5120,5120,5120,5120,5120,17408,17408};
	unsigned rowOffsets[8]={},at=0,cases=0,limit=real?8:workers==2?9:3;
	for (unsigned i=0;i<8;i++) { rowOffsets[i]=at; at+=(widths[i]/PioneerQ38CohortBlockInputs(kinds[i]))*PioneerQ38PanelTileBytes(kinds[i]); }
	if (real && at!=publicRows.size()) okay=false;
#if defined(PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT) || defined(PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT) || defined(PIONEER_Q38_SENTINEL_REFERENCE_CLIENT)
	const unsigned originalLimit=limit;limit++; // explicit mixed-type freshness regression in SAME live cohort
#endif
	for (unsigned ordinal=0;okay && ordinal<limit;ordinal++) {
#if defined(PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT) || defined(PIONEER_Q38_DIRECT_SRCB_REFERENCE_CLIENT) || defined(PIONEER_Q38_SENTINEL_REFERENCE_CLIENT)
		unsigned fixture=ordinal==originalLimit?0:ordinal;
#else
		unsigned fixture=ordinal;
#endif
		unsigned type=real?kinds[fixture]:workers==2?(fixture/3==0?8:fixture/3==1?12:14):(fixture==0?8:fixture==1?12:14);
		unsigned inputs=real?widths[fixture]:workers==2?(fixture%3==0?5120:fixture%3==1?6144:17408):(fixture==1?17408:5120);
		okay=Execute(fd,type,inputs,workers,real && workers==2?3:1,fixture,publicRows,rowOffsets
#ifdef PIONEER_Q38_DISPATCH_CLIENT
			,history
#endif
		); if (okay) cases++;
	}
	if (!okay) { fprintf(stderr,"STOP:COHORT_PRIMITIVE_FAILURE;CLOSE_ONLY_NO_END_RETRY_RESET\n"); close(fd); return 1; }
	session={}; session.version=1;
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session)) && session.result.state==TT_RESIDENT_SESSION_CLOSED
		&& !session.result.active && session.result.pci_restored && session.result.host_released && !session.result.cold;
	printf("BANK_END state%u active%u pci_restored%u host_released%u cold%u accepted%u;NO_RETRY_OR_ADDITIONAL_CARD_READ\n",
		session.result.state,session.result.active,session.result.pci_restored,session.result.host_released,session.result.cold,unsigned(okay));
	if (close(fd)) okay=false;
	printf("%s_COHORT_%s_PRIMITIVES cases=%u workers=%u KNOWN_CLEAN_END=%u;NOT_CONTIGUOUS_MATRIX_OR_GRAPH_OR_MODEL_INFERENCE_OR_SPEED\n",
		okay?"PASS":"FAIL",real?"AUTHENTIC_ROW_MIXES":"SYNTHETIC",cases,workers,unsigned(okay));
	return okay && cases==limit?0:1;
}
