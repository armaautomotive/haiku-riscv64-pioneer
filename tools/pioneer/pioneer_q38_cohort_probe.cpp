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
			if (type==8) tile[32+row]=128;
		}
		if (!Reference(*r,packed,w,expected.data()+w*256)) return false;
		memcpy(upload.data()+head,packed.data(),packed.size());
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
		double finish=Seconds(); if (finish<previous) return false; unsigned errors=0;
		for (unsigned w=0;w<workers;w++) for (unsigned row=0;row<256;row++) errors+=r->output[w][row]!=PioneerAttentionBits(expected[w*256+row]);
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
		puts("PUBLIC_ROW_MEMORY_SHA_PASS_725504_BYTES;INTERLEAVED_GROUPS_NOT_FULL_MODEL");
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC); if (fd<0) return 1;
#ifdef PIONEER_Q38_DISPATCH_CLIENT
	tt_q38_dispatch_support support={};
	bool okay=Call(fd,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,&support,sizeof(support)) && support.driver==TT_Q38_DISPATCH_DRIVER && support.version==1 && support.peers==119
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
	for (unsigned ordinal=0;okay && ordinal<limit;ordinal++) {
		unsigned type=real?kinds[ordinal]:workers==2?(ordinal/3==0?8:ordinal/3==1?12:14):(ordinal==0?8:ordinal==1?12:14);
		unsigned inputs=real?widths[ordinal]:workers==2?(ordinal%3==0?5120:ordinal%3==1?6144:17408):(ordinal==1?17408:5120);
		okay=Execute(fd,type,inputs,workers,real && workers==2?3:1,ordinal,publicRows,rowOffsets
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
