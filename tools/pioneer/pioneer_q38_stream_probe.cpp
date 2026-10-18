/* SPDX-License-Identifier: MIT */
// Explicit CPU-checked fullK PRIMITIVE qualification,NOT inference/backend.
// No arbitrary model/file/code/physical address/coordinate/command arguments.
#include <Drivers.h>
#include <OS.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38StreamProtocol.h"
#include "PioneerQwen38Panel.h"
#include "PioneerQwen38Gguf.h"
#include "PioneerQ38RowFixtures.h"
static bool Call(int fd,uint32_t op,void* data,size_t bytes)
{
	if (!ioctl(fd,op,data,bytes)) return true;
	fprintf(stderr,"STOP:op=%08x errno=%d %s;NO_RETRY\n",op,errno,strerror(errno)); return false;
}
static bool BulkVerified(const tt_q38_bank_bulk& b)
{
	const auto& t=b.result.transfer;
	bool okay=b.result.pages==b.pages && b.result.host_words==b.pages*1024 && b.result.host_ready && b.result.l1_prepared
		&& t.version==1 && t.state==TT_STORE_PASSED && t.requests==6 && t.completed==6 && t.wrote_device
		&& t.host_write_issued && t.host_visible && t.readback_complete && !t.mismatches && !t.recovery_required;
	for (unsigned step=0;step<6;step++) for (unsigned i=0;i<4;i++) okay &= uint32_t(t.after[step][i]-t.before[step][i])==(i==1?b.pages*64:1);
	return okay;
}
static bool Upload(int fd,unsigned bank,uint64_t offset,const uint8_t* data,unsigned bytes)
{
	unsigned pages=(bytes+4095)/4096;
	for (unsigned p=0;p<pages;) {
		tt_q38_bank_bulk r={}; r.version=1; r.bank=bank; r.pages=pages-p<4?pages-p:4; r.offset=offset+uint64_t(p)*4096;
		unsigned copy=bytes-p*4096; if (copy>r.pages*4096) copy=r.pages*4096;
		memcpy(r.data,data+p*4096,copy);
		if (!Call(fd,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&r,sizeof(r)) || !BulkVerified(r)) return false;
		p+=r.pages;
	}
	return true;
}
static bool Execute(int fd,unsigned type,unsigned inputs,const std::vector<uint8_t>& packed,unsigned trial,unsigned ordinal)
{
	tt_q38_stream_probe r={}; r.version=1; r.type=type; r.inputs=inputs; r.bank=ordinal%7;
	unsigned bytes=PioneerQ38StreamBytes(r),columns=PioneerQ38StreamBlockInputs(type),blocks=inputs/columns,tileBytes=PioneerQ38PanelTileBytes(type);
	if (packed.size()!=bytes) return false;
	r.offset=(ordinal&1)?PioneerResidentBanks::Capacity-uint64_t((bytes+4095)/4096)*4096:0;
	for (unsigned k=0;k<inputs;k++) r.activation[k/2]|=uint32_t(PioneerInt8Operand(int((k*29+trial*17)%256)-128))<<((k%2)*16);
	for (unsigned b=0;b<blocks;b++) r.scales[b]=PioneerAttentionBits(b%3?-.03125f:.012345f);
	float expected[16]={};
	for (unsigned b=0;b<blocks;b++) for (unsigned p=0;p<columns/16;p++) {
		PioneerQ38Panel panel;
		if (!PioneerQ38UnpackPanel(type,packed.data()+b*tileBytes,tileBytes,p,&panel)) return false;
		int inputSum=0;
		for (unsigned k=0;k<16;k++) { unsigned at=b*columns+p*16+k; uint16_t a=uint16_t(r.activation[at/2]>>((at%2)*16)); inputSum+=int(a&1023)*((a&0x8000)?-1:1); }
		for (unsigned row=0;row<16;row++) {
			int dot=0;
			for (unsigned k=0;k<16;k++) {
				unsigned e=k*16+row,at=b*columns+p*16+k; uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16)),a=uint16_t(r.activation[at/2]>>((at%2)*16));
				dot+=int(w&1023)*((w&0x8000)?-1:1)*int(a&1023)*((a&0x8000)?-1:1);
			}
			float scale=PioneerAttentionFloat(r.scales[b]);
			float product=(PioneerAttentionFloat(panel.coefficient[row])*scale)*float(dot),correction=(PioneerAttentionFloat(panel.minimum[row])*scale)*float(inputSum);
			expected[row]+=product+correction;
		}
	}
	if (!Upload(fd,r.bank,r.offset,packed.data(),bytes) || !Call(fd,TT_PCI_PROBE_Q38_STREAM_RUN,&r,sizeof(r))) return false;
	const auto& v=r.result;
	bool okay=v.version==1 && v.state==TT_RUNTIME_PASSED && v.prepared && v.started && v.completed && v.stopped && !v.recovery_required
		&& v.context_reset && v.phase==0x7200c && v.panels==inputs/16 && v.marker==0x31533851 && v.pc[0]==0x72038 && v.pc[1]==v.pc[0];
	for (unsigned i=0;i<8;i++) {
		unsigned count=i==1?bytes/64:(i==0 || i==2 || i==3 || i==6)?blocks:0;
		okay &= uint32_t(v.after[i]-v.before[i])==count;
	}
	unsigned mismatches=0; for (unsigned row=0;row<16;row++) mismatches+=r.output[row]!=PioneerAttentionBits(expected[row]);
#ifdef PIONEER_Q38_STREAM_DIAGNOSTICS
	if (!okay || mismatches) {
		printf("FAIL_PROOF phase=%x panels=%u expected=%u marker=%x polls=%u prepared=%u context_reset=%u\n",
			v.phase,v.panels,inputs/16,v.marker,v.polls,v.prepared,v.context_reset);
		for (unsigned i=0;i<8;i++) printf("FAIL_COUNTER%u before=%u after=%u delta=%u expected=%u\n",i,v.before[i],v.after[i],uint32_t(v.after[i]-v.before[i]),
			i==1?bytes/64:(i==0 || i==2 || i==3 || i==6)?blocks:0);
	}
#endif
	printf("FULL_K_PRIMITIVE type=%u K=%u bank=%u last=%u state=%u start/complete/stop=%u/%u/%u cold=%u pc=%x/%x exact_errors=%u\n",
		type,inputs,r.bank,ordinal&1,v.state,v.started,v.completed,v.stopped,v.recovery_required,v.pc[0],v.pc[1],mismatches);
	return okay && !mismatches;
}
int main(int argc,char** argv)
{
	if (argc!=2 || (strcmp(argv[1],"--support") && strcmp(argv[1],"--synthetic") && strcmp(argv[1],"--public-fixtures"))) return 2;
	setvbuf(stdout,NULL,_IOLBF,0); bool real=!strcmp(argv[1],"--public-fixtures");
	std::vector<uint8_t> publicRows;
	if (real) {
		publicRows.resize(PIONEER_Q38_ROW_FIXTURE_BYTES);
		if (!PioneerQ38LoadRowFixtures(publicRows.data(),unsigned(publicRows.size()))) { fprintf(stderr,"STOP:PUBLIC_FULL_ROW_MEMORY_SHA_REQUIRED_BEFORE_OPEN\n"); return 1; }
		puts("PUBLIC_FULL_ROW_MEMORY_SHA_PASS_725504_BYTES;NOT_FULL_MODEL_OR_INFERENCE");
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC); if (fd<0) return 1;
	tt_q38_stream_support support={}; tt_q38_bank_support storage={};
	bool okay=Call(fd,TT_PCI_PROBE_Q38_STREAM_SUPPORT,&support,sizeof(support)) && support.driver==49 && support.version==1
		&& support.local_rows==16 && support.max_inputs==17408 && support.banks==7 && !support.model_inference;
	if (!okay || !strcmp(argv[1],"--support")) { close(fd); return okay?0:1; }
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&storage,sizeof(storage)) && storage.driver==45 && storage.version==1 && storage.banks==7
		&& storage.page_bytes==4096 && storage.max_pages==4 && storage.storage_only==1 && storage.bytes_per_bank==PioneerResidentBanks::Capacity;
	if (!okay) { close(fd); return 1; }
	tt_q38_bank_session session={}; session.version=1;
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) && session.result.version==1 && session.result.state==TT_RESIDENT_SESSION_READY
		&& session.result.active && session.result.host_ready && !session.result.cold && !session.result.capabilities && session.result.bytes==PioneerResidentBanks::Total;
	unsigned cases=0,at=0;
	const unsigned kinds[]={8,8,12,12,14,14,12,12},widths[]={5120,5120,5120,5120,5120,5120,17408,17408};
	for (unsigned ordinal=0;okay && ordinal<(real?8U:9U);ordinal++) {
		unsigned type=real?kinds[ordinal]:(ordinal/3==0?8:ordinal/3==1?12:14),inputs=real?widths[ordinal]:(ordinal%3==0?5120:ordinal%3==1?6144:17408);
		unsigned columns=PioneerQ38StreamBlockInputs(type),tileBytes=PioneerQ38PanelTileBytes(type),blockBytes=type==8?34:type==12?144:210;
		std::vector<uint8_t> packed((inputs/columns)*tileBytes,0);
		if (real) {
			if (at>publicRows.size() || packed.size()>publicRows.size()-at) { okay=false; break; }
			memcpy(packed.data(),publicRows.data()+at,packed.size()); at+=unsigned(packed.size());
		} else for (unsigned b=0;b<inputs/columns;b++) for (unsigned row=0;row<16;row++) {
			uint8_t* tile=packed.data()+b*tileBytes;
			for (unsigned byte=0;byte<blockBytes;byte++) tile[byte*16+row]=uint8_t(byte*37+row*59+b*13);
			unsigned d=type==14?208:0; uint16_t half=row%3?uint16_t(0x3c00+row*13):uint16_t(1+row);
			tile[d*16+row]=uint8_t(half); tile[(d+1)*16+row]=uint8_t(half>>8);
			if (type==12) { tile[32+row]=0; tile[48+row]=0x38; }
			if (type==8) tile[32+row]=128;
		}
		okay=Execute(fd,type,inputs,packed,1,ordinal); if (okay) cases++;
	}
	if (real) okay &= at==publicRows.size();
	if (!okay) { fprintf(stderr,"STOP:FULL_K_PROBE_FAILURE;CLOSE_ONLY_NO_END_RETRY_RESET\n"); close(fd); return 1; }
	session={}; session.version=1;
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session)) && session.result.state==TT_RESIDENT_SESSION_CLOSED
		&& !session.result.active && session.result.pci_restored && session.result.host_released && !session.result.cold;
	if (close(fd)) okay=false;
	printf("%s_LOCAL_FULL_K_%s_PRIMITIVES cases=%u;KNOWN_CLEAN_END=%u;NOT_MULTIWORKER_OR_GRAPH_OR_INFERENCE_OR_SPEED\n",okay?"PASS":"FAIL",real?"PUBLIC_ROWS":"SYNTHETIC",cases,unsigned(okay));
	return okay && cases==(real?8U:9U)?0:1;
}
