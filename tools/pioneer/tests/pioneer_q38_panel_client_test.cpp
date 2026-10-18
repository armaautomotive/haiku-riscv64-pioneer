/* SPDX-License-Identifier: MIT */
// REAL debug client,synthetic syscalls;never opens card/network/model data.
#include <cassert>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38PanelProbe.h"
#include "../PioneerQwen38Panel.h"
static unsigned opens=0,closes=0,begins=0,ends=0,runs=0,fault=0,field=0;
static bool failed=false;
#ifdef PIONEER_Q38_PUBLIC_FIXTURES
static const char* hostFixture=nullptr;
static unsigned authFault=0;
static FILE* MockFopen(const char* path,const char* mode)
{
	assert(hostFixture && !strcmp(path,"/boot/home/pioneer-qwen38-affine-20261017-a/public-model-panels-eb62d234.bin") && !strcmp(mode,"rb"));
	FILE* source=fopen(hostFixture,"rb"); assert(source);
	if (!authFault) return source;
	uint8_t data[12544]; assert(fread(data,1,sizeof(data),source)==sizeof(data)); fclose(source);
	FILE* temporary=tmpfile(); assert(temporary); data[0]^=1;
	assert(fwrite(data,1,sizeof(data)-(authFault==2),temporary)==sizeof(data)-(authFault==2)); fflush(temporary); rewind(temporary); return temporary;
}
static int MockExecv(const char* path,char* const*)
{
	assert(!strcmp(path,"/bin/sha256sum"));
	char name[]="shasum",option[]="-a",bits[]="256"; char* args[]={name,option,bits,NULL};
	return execv("/usr/bin/shasum",args); // same stdin digest format,host-only test
}
#endif
static int MockOpen(const char* path,int flags)
{
	assert(!strcmp(path,"/dev/" TT_PCI_PROBE_DEVICE) && flags==(O_RDWR|O_CLOEXEC)); opens++; return 73;
}
static int MockClose(int fd) { if (fd==73) { closes++; return 0; } return close(fd); }
static int MockIoctl(int fd,unsigned long op,void* data,size_t bytes)
{
	assert(fd==73 && !failed);
	if (op==TT_PCI_PROBE_Q38_PANEL_SUPPORT) {
		assert(bytes==sizeof(tt_q38_panel_support));
		*static_cast<tt_q38_panel_support*>(data)={fault==4?45U:unsigned(TT_Q38_PANEL_CURRENT_DRIVER),TT_Q38_PANEL_ABI_VERSION,1,16,TT_Q38_PANEL_COLUMNS,0x5100}; return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_BANK_END) {
		assert(bytes==sizeof(tt_q38_bank_session)); auto& r=static_cast<tt_q38_bank_session*>(data)->result;
		r={}; r.version=1;
		if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) { begins++; r.state=TT_RESIDENT_SESSION_READY; r.active=r.host_ready=1; r.bytes=PioneerResidentBanks::Total; }
		else { ends++; r.state=TT_RESIDENT_SESSION_CLOSED; r.pci_restored=r.host_released=1; }
		return 0;
	}
	assert(op==TT_PCI_PROBE_Q38_PANEL_RUN && bytes==sizeof(tt_q38_panel_probe));
	auto& p=*static_cast<tt_q38_panel_probe*>(data); assert(PioneerQ38PanelProbeValid(p)); runs++;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
	PioneerQ38Panel reference={}; float accumulated[16]={};
	for (unsigned panel=0;panel<PioneerQ38PanelExpectedCount(p.type);panel++) {
		assert(PioneerQ38UnpackPanel(p.type,reinterpret_cast<const uint8_t*>(p.tile),p.bytes,panel,&reference));
		int inputSum=0;
		for (unsigned k=0;k<16;k++) {
			unsigned at=panel*16+k; uint16_t a=uint16_t(p.activation[at/2]>>((at%2)*16));
			inputSum+=int(a&1023)*((a&0x8000)?-1:1);
		}
		for (unsigned row=0;row<16;row++) {
			int dot=0;
			for (unsigned k=0;k<16;k++) {
				unsigned e=k*16+row,at=panel*16+k; uint16_t w=uint16_t(reference.operand[e/2]>>((e%2)*16)),a=uint16_t(p.activation[at/2]>>((at%2)*16));
				dot+=int(w&1023)*((w&0x8000)?-1:1)*int(a&1023)*((a&0x8000)?-1:1);
			}
			float scale,coefficient,minimum; memcpy(&scale,&p.activation_scale,4); memcpy(&coefficient,&reference.coefficient[row],4); memcpy(&minimum,&reference.minimum[row],4);
			float product=(coefficient*scale)*float(dot),correction=(minimum*scale)*float(inputSum); accumulated[row]+=product+correction;
		}
	}
	for (unsigned row=0;row<16;row++) {
		memcpy(&p.dot[row],&accumulated[row],4); p.coefficient[row]=reference.coefficient[row]; p.minimum[row]=reference.minimum[row];
	}
#else
	PioneerQ38Panel reference={}; assert(PioneerQ38UnpackPanel(p.type,reinterpret_cast<const uint8_t*>(p.tile),p.bytes,p.panel,&reference));
	for (unsigned row=0;row<16;row++) {
		int sum=0;
		for (unsigned k=0;k<16;k++) {
			unsigned element=k*16+row; uint16_t w=uint16_t(reference.operand[element/2]>>((element%2)*16));
			uint16_t a=uint16_t(p.activation[k/2]>>((k%2)*16));
			sum+=int(w&1023)*((w&0x8000)?-1:1)*int(a&1023)*((a&0x8000)?-1:1);
		}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
		int inputSum=0; for (unsigned k=0;k<16;k++) {
			uint16_t a=uint16_t(p.activation[k/2]>>((k%2)*16)); inputSum+=int(a&1023)*((a&0x8000)?-1:1);
		}
		float scale; memcpy(&scale,&p.activation_scale,4);
		float coefficient,minimum; memcpy(&coefficient,&reference.coefficient[row],4); memcpy(&minimum,&reference.minimum[row],4);
		float product=(coefficient*scale)*float(sum),correction=(minimum*scale)*float(inputSum),value=product+correction;
		memcpy(&p.dot[row],&value,4);
#else
		p.dot[row]=uint32_t(sum);
#endif
		p.coefficient[row]=reference.coefficient[row]; p.minimum[row]=reference.minimum[row];
	}
#endif
	auto& r=p.result; r={}; r.version=1; r.state=TT_RUNTIME_PASSED; r.prepared=r.started=r.completed=r.stopped=r.context_reset=1;
	r.phase=TT_Q38_LOCAL_DONE; r.panels=PioneerQ38PanelExpectedCount(p.type); r.marker=TT_Q38_PANEL_SUCCESS_MARKER; r.pc[0]=r.pc[1]=TT_Q38_LOCAL_PARK;
	for (unsigned i=0;i<8;i++) r.before[i]=r.after[i]=UINT32_MAX-i;
	if (fault==1) { failed=true; errno=EIO; return -1; }
	if (fault==2) {
		failed=true;
		switch (field) {
			case 0:r.version=2;break; case 1:r.state=TT_RUNTIME_NONFINITE;break;
			case 2:r.prepared=0;break; case 3:r.started=0;break; case 4:r.completed=0;break; case 5:r.stopped=0;break;
			case 6:r.recovery_required=1;break; case 7:r.context_reset=0;break; case 8:r.phase++;break;
			case 9:r.panels++;break; case 10:r.marker=0;break; case 11:r.pc[0]++;break; case 12:r.pc[1]++;break;
			default:r.after[field-13]++;break;
		}
	}
	if (fault==3) { failed=true; if (field==0) p.dot[15]^=1; else if (field==1) p.coefficient[15]^=1; else p.minimum[15]^=1; }
	return 0;
}
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#ifdef PIONEER_Q38_PUBLIC_FIXTURES
#define fopen MockFopen
#define execv MockExecv
#endif
#define main Q38PanelMain
#include "../pioneer_q38_panel_probe.cpp"
#undef main
#undef ioctl
#undef close
#undef open
#ifdef PIONEER_Q38_PUBLIC_FIXTURES
#undef fopen
#undef execv
#endif
static int Run(const char* argument)
{
	char name[]="panel"; char* argv[]={name,const_cast<char*>(argument)}; return Q38PanelMain(2,argv);
}
static void Reset(unsigned f=0,unsigned w=0) { opens=closes=begins=ends=runs=0; failed=false; fault=f; field=w; }
int main(int argc,char** argv)
{
#ifdef PIONEER_Q38_PUBLIC_FIXTURES
	assert(argc==2); hostFixture=argv[1];
#else
	(void)argv; assert(argc==1);
#endif
	Reset(); assert(Run("--support")==0 && opens==1 && closes==1 && !begins && !runs);
	Reset(4); assert(Run("--synthetic")==1 && opens==1 && closes==1 && !begins && !runs);
	Reset(); assert(Run("--synthetic")==0 && begins==1 && ends==1 && runs==12 && closes==1);
	for (unsigned f=1;f<=3;f++) for (unsigned w=0;w<(f==1?1U:f==2?21U:3U);w++) {
		Reset(f,w); assert(Run("--synthetic")==1 && failed && begins==1 && runs==1 && !ends && closes==1);
	}
	Reset(); assert(Run("--bad-argument")==2 && !opens);
#ifdef PIONEER_Q38_PUBLIC_FIXTURES
	Reset(); assert(Run("--public-fixtures")==0 && begins==1 && ends==1 && runs==12 && closes==1);
	for (unsigned bad=1;bad<=2;bad++) {
		Reset(); authFault=bad; assert(Run("--public-fixtures")==1 && !opens && !begins && !runs && !ends); authFault=0;
	}
	puts("PASS: REAL public-fragment client+MEMORY SHA pipeline,all12 authentic scaled panels,corrupt+truncated fixture refuses BEFORE device open;NO card/network access");
#else
	puts("PASS: REAL local-panel debug client,metadata-only/old-driver refusal,all12 mixed shapes,ALL21 completion proofs+3 value corruptions/ioctl error close-only;NO card/network/model access");
#endif
}
