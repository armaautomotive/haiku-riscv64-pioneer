/* SPDX-License-Identifier: MIT */
// REAL debug client,synthetic syscalls;never opens card/network/model data.
#include <cassert>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38PanelProbe.h"
#include "../PioneerQwen38Panel.h"
static unsigned opens=0,closes=0,begins=0,ends=0,runs=0,fault=0,field=0;
static bool failed=false;
static int MockOpen(const char* path,int flags)
{
	assert(!strcmp(path,"/dev/" TT_PCI_PROBE_DEVICE) && flags==(O_RDWR|O_CLOEXEC)); opens++; return 73;
}
static int MockClose(int fd) { assert(fd==73); closes++; return 0; }
static int MockIoctl(int fd,unsigned long op,void* data,size_t bytes)
{
	assert(fd==73 && !failed);
	if (op==TT_PCI_PROBE_Q38_PANEL_SUPPORT) {
		assert(bytes==sizeof(tt_q38_panel_support));
		*static_cast<tt_q38_panel_support*>(data)={fault==4?45U:46U,1,1,16,16,0x5100}; return 0;
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
	PioneerQ38Panel reference={}; assert(PioneerQ38UnpackPanel(p.type,reinterpret_cast<const uint8_t*>(p.tile),p.bytes,p.panel,&reference));
	for (unsigned row=0;row<16;row++) {
		int sum=0;
		for (unsigned k=0;k<16;k++) {
			unsigned element=k*16+row; uint16_t w=uint16_t(reference.operand[element/2]>>((element%2)*16));
			uint16_t a=uint16_t(p.activation[k/2]>>((k%2)*16));
			sum+=int(w&1023)*((w&0x8000)?-1:1)*int(a&1023)*((a&0x8000)?-1:1);
		}
		p.dot[row]=uint32_t(sum); p.coefficient[row]=reference.coefficient[row]; p.minimum[row]=reference.minimum[row];
	}
	auto& r=p.result; r={}; r.version=1; r.state=TT_RUNTIME_PASSED; r.prepared=r.started=r.completed=r.stopped=r.context_reset=1;
	r.phase=TT_Q38_PANEL_DONE; r.panels=1; r.marker=0x4d415431; r.pc[0]=r.pc[1]=TT_Q38_PANEL_PARK;
	for (unsigned i=0;i<8;i++) r.before[i]=r.after[i]=UINT32_MAX-i;
	if (fault==1) { failed=true; errno=EIO; return -1; }
	if (fault==2) {
		failed=true;
		switch (field) {
			case 0:r.version=2;break; case 1:r.state=TT_RUNTIME_NONFINITE;break;
			case 2:r.prepared=0;break; case 3:r.started=0;break; case 4:r.completed=0;break; case 5:r.stopped=0;break;
			case 6:r.recovery_required=1;break; case 7:r.context_reset=0;break; case 8:r.phase++;break;
			case 9:r.panels=2;break; case 10:r.marker=0;break; case 11:r.pc[0]++;break; case 12:r.pc[1]++;break;
			default:r.after[field-13]++;break;
		}
	}
	if (fault==3) { failed=true; if (field==0) p.dot[15]^=1; else if (field==1) p.coefficient[15]^=1; else p.minimum[15]^=1; }
	return 0;
}
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#define main Q38PanelMain
#include "../pioneer_q38_panel_probe.cpp"
#undef main
#undef ioctl
#undef close
#undef open
static int Run(const char* argument)
{
	char name[]="panel"; char* argv[]={name,const_cast<char*>(argument)}; return Q38PanelMain(2,argv);
}
static void Reset(unsigned f=0,unsigned w=0) { opens=closes=begins=ends=runs=0; failed=false; fault=f; field=w; }
int main()
{
	Reset(); assert(Run("--support")==0 && opens==1 && closes==1 && !begins && !runs);
	Reset(4); assert(Run("--synthetic")==1 && opens==1 && closes==1 && !begins && !runs);
	Reset(); assert(Run("--synthetic")==0 && begins==1 && ends==1 && runs==12 && closes==1);
	for (unsigned f=1;f<=3;f++) for (unsigned w=0;w<(f==1?1U:f==2?21U:3U);w++) {
		Reset(f,w); assert(Run("--synthetic")==1 && failed && begins==1 && runs==1 && !ends && closes==1);
	}
	Reset(); assert(Run("--bad-argument")==2 && !opens);
	puts("PASS: REAL local-panel debug client,metadata-only/old-driver refusal,all12 mixed shapes,ALL21 completion proofs+3 value corruptions/ioctl error close-only;NO card/network/model access");
}
