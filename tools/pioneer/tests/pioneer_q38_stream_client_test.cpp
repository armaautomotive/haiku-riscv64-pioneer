/* SPDX-License-Identifier: MIT */
// REAL client + REAL memory-auth pipeline;synthetic device syscalls ONLY.
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <array>
#include <map>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StreamProtocol.h"
#include "../PioneerQwen38Panel.h"
#include "../PioneerQwen38Gguf.h"
static unsigned opens,closes,begins,ends,runs,uploads,fault,field,authFault;
static bool failed;
static const char* fixture;
static std::map<uint64_t,std::array<uint32_t,1024>> pages;
static FILE* MockFopen(const char* path,const char* mode)
{
	assert(!strcmp(path,"/boot/home/pioneer-qwen38-stream-20261017-a/public-model-rowgroups-a0611645.bin") && !strcmp(mode,"rb"));
	FILE* file=fopen(fixture,"rb"); assert(file); if (!authFault) return file;
	std::vector<uint8_t> data(725504); assert(fread(data.data(),1,data.size(),file)==data.size()); fclose(file);
	FILE* bad=tmpfile(); assert(bad); data[0]^=1; assert(fwrite(data.data(),1,data.size()-(authFault==2),bad)==data.size()-(authFault==2));
	fflush(bad); rewind(bad); return bad;
}
static int MockExecv(const char* path,char* const*)
{
	assert(!strcmp(path,"/bin/sha256sum")); char n[]="shasum",a[]="-a",bits[]="256"; char* args[]={n,a,bits,NULL}; return execv("/usr/bin/shasum",args);
}
static int MockOpen(const char* path,int flags)
{
	assert(!strcmp(path,"/dev/" TT_PCI_PROBE_DEVICE) && flags==(O_RDWR|O_CLOEXEC)); opens++; return 73;
}
static int MockClose(int fd) { if (fd==73) { closes++; return 0; } return close(fd); }
static int MockIoctl(int fd,unsigned long op,void* data,size_t bytes)
{
	assert(fd==73 && !failed);
	if (op==TT_PCI_PROBE_Q38_STREAM_SUPPORT) {
		assert(bytes==sizeof(tt_q38_stream_support)); *static_cast<tt_q38_stream_support*>(data)={fault==4?48U:49U,1,16,17408,7,0}; return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
		assert(bytes==sizeof(tt_q38_bank_support)); *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,fault==5?3U:4U,1,PioneerResidentBanks::Capacity}; return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_BANK_END) {
		assert(bytes==sizeof(tt_q38_bank_session)); auto& r=static_cast<tt_q38_bank_session*>(data)->result;
		r={}; r.version=1;
		if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) { begins++; r.state=TT_RESIDENT_SESSION_READY; r.active=r.host_ready=1; r.bytes=PioneerResidentBanks::Total; }
		else { ends++; r.state=TT_RESIDENT_SESSION_CLOSED; r.pci_restored=r.host_released=1; }
		return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD) {
		assert(bytes==sizeof(tt_q38_bank_bulk)); auto& r=*static_cast<tt_q38_bank_bulk*>(data); uploads++;
		assert(r.version==1 && !r.reserved && PioneerQ38BankBulkRange(r.bank,r.offset,r.pages));
		for (unsigned p=0;p<r.pages;p++) memcpy(pages[(uint64_t(r.bank)<<32)|(r.offset+p*4096)].data(),r.data+p*1024,4096);
		r.result={}; r.result.pages=r.pages; r.result.host_words=r.pages*1024; r.result.host_ready=r.result.l1_prepared=1;
		auto& t=r.result.transfer; t.version=1; t.state=TT_STORE_PASSED; t.requests=t.completed=6;
		t.wrote_device=t.host_write_issued=t.host_visible=t.readback_complete=1;
		for (unsigned s=0;s<6;s++) for (unsigned i=0;i<4;i++) { t.before[s][i]=UINT32_MAX-17; t.after[s][i]=t.before[s][i]+(i==1?r.pages*64:1); }
		if (fault==6) {
			failed=true;
			switch (field) {
				case 0:r.result.pages++;break; case 1:r.result.host_words++;break; case 2:r.result.host_ready=0;break;
				case 3:r.result.l1_prepared=0;break; case 4:t.version++;break; case 5:t.state=TT_STORE_MISMATCH;break;
				case 6:t.requests--;break; case 7:t.completed--;break; case 8:t.wrote_device=0;break;
				case 9:t.host_write_issued=0;break; case 10:t.host_visible=0;break; case 11:t.readback_complete=0;break;
				case 12:t.mismatches=1;break; case 13:t.recovery_required=1;break;
				default:{unsigned at=field-14; t.after[at/4][at%4]++;break;}
			}
		}
		return 0;
	}
	assert(op==TT_PCI_PROBE_Q38_STREAM_RUN && bytes==sizeof(tt_q38_stream_probe));
	auto& r=*static_cast<tt_q38_stream_probe*>(data); assert(PioneerQ38StreamValid(r)); runs++;
	std::vector<uint8_t> packed(PioneerQ38StreamBytes(r));
	for (unsigned p=0;p<(packed.size()+4095)/4096;p++) {
		auto found=pages.find((uint64_t(r.bank)<<32)|(r.offset+p*4096)); assert(found!=pages.end());
		unsigned copy=unsigned(packed.size())-p*4096; if (copy>4096) copy=4096; memcpy(packed.data()+p*4096,found->second.data(),copy);
	}
	float expected[16]={}; unsigned columns=PioneerQ38StreamBlockInputs(r.type),blocks=r.inputs/columns,tileBytes=PioneerQ38PanelTileBytes(r.type);
	for (unsigned b=0;b<blocks;b++) for (unsigned p=0;p<columns/16;p++) {
		PioneerQ38Panel panel; assert(PioneerQ38UnpackPanel(r.type,packed.data()+b*tileBytes,tileBytes,p,&panel));
		int inputSum=0;
		for (unsigned k=0;k<16;k++) { unsigned at=b*columns+p*16+k; auto a=uint16_t(r.activation[at/2]>>((at%2)*16)); inputSum+=int(a&1023)*((a&0x8000)?-1:1); }
		for (unsigned row=0;row<16;row++) {
			int dot=0;
			for (unsigned k=0;k<16;k++) {
				unsigned e=k*16+row,at=b*columns+p*16+k; uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16)),a=uint16_t(r.activation[at/2]>>((at%2)*16));
				dot+=int(w&1023)*((w&0x8000)?-1:1)*int(a&1023)*((a&0x8000)?-1:1);
			}
			float scale=PioneerAttentionFloat(r.scales[b]),product=(PioneerAttentionFloat(panel.coefficient[row])*scale)*float(dot),correction=(PioneerAttentionFloat(panel.minimum[row])*scale)*float(inputSum);
			expected[row]+=product+correction;
		}
	}
	for (unsigned row=0;row<16;row++) r.output[row]=PioneerAttentionBits(expected[row]);
	auto& v=r.result; v={}; v.version=1; v.state=TT_RUNTIME_PASSED; v.prepared=v.started=v.completed=v.stopped=v.context_reset=1;
	v.phase=0x7200c; v.panels=r.inputs/16; v.marker=0x31533851; v.pc[0]=v.pc[1]=0x72038;
	for (unsigned i=0;i<8;i++) {
		unsigned n=i==1?unsigned(packed.size())/64:(i==0 || i==2 || i==3 || i==6)?blocks:0;
		v.before[i]=UINT32_MAX-10; v.after[i]=v.before[i]+n;
	}
	if (fault==1) { failed=true; errno=EIO; return -1; }
	if (fault==2) {
		failed=true;
		switch (field) {
			case 0:v.version++;break; case 1:v.state=TT_RUNTIME_NONFINITE;break; case 2:v.prepared=0;break;
			case 3:v.started=0;break; case 4:v.completed=0;break; case 5:v.stopped=0;break; case 6:v.recovery_required=1;break;
			case 7:v.context_reset=0;break; case 8:v.phase++;break; case 9:v.panels++;break; case 10:v.marker++;break;
			case 11:v.pc[0]++;break; case 12:v.pc[1]++;break; default:v.after[field-13]++;break;
		}
	}
	if (fault==3) { failed=true; r.output[15]^=1; }
	return 0;
}
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#define fopen MockFopen
#define execv MockExecv
#define main StreamMain
#include "../pioneer_q38_stream_probe.cpp"
#undef main
#undef execv
#undef fopen
#undef ioctl
#undef close
#undef open
static int Run(const char* argument) { char name[]="stream"; char* argv[]={name,const_cast<char*>(argument)}; return StreamMain(2,argv); }
static void Reset(unsigned f=0,unsigned w=0) { opens=closes=begins=ends=runs=uploads=0; fault=f; field=w; failed=false; pages.clear(); }
int main(int argc,char** argv)
{
	assert(argc==2); fixture=argv[1];
	Reset(); assert(!Run("--support") && opens==1 && closes==1 && !begins && !uploads && !runs);
	for (unsigned f:{4U,5U}) { Reset(f); assert(Run("--synthetic")==1 && opens==1 && closes==1 && !begins && !uploads && !runs); }
	Reset(); assert(!Run("--synthetic") && begins==1 && ends==1 && runs==9 && closes==1);
	Reset(); assert(!Run("--public-fixtures") && begins==1 && ends==1 && runs==8 && closes==1);
	for (unsigned f:{1U,2U,3U}) for (unsigned w=0;w<(f==2?21U:1U);w++) {
		Reset(f,w); assert(Run("--synthetic")==1 && failed && runs==1 && begins==1 && !ends && closes==1);
	}
	for (unsigned w=0;w<38;w++) {
		Reset(6,w); assert(Run("--synthetic")==1 && failed && uploads==1 && !runs && begins==1 && !ends && closes==1);
	}
	for (unsigned f:{1U,2U}) {
		Reset(); authFault=f; assert(Run("--public-fixtures")==1 && !opens && !begins && !uploads && !runs); authFault=0;
	}
	Reset(); assert(Run("--bad-argument")==2 && !opens);
	puts("PASS REAL fullK client+memory-auth pipeline:9 synthetic/8 authenticated full-row groups,old-driver/storage refusal,ALL21 completion proofs/38 bulk proofs/wrapping counters/numeric corruption/ioctl failure CLOSE-ONLY,corrupt/truncated data refuses BEFORE card open;NO hardware");
}
