/* SPDX-License-Identifier: MIT */
// REAL client,synthetic syscalls;NO device is opened. The first failed RUN
// must log ONLY its already returned host copy,then CLOSE with NO later IO.
#include <Drivers.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchProtocol.h"
#ifdef PIONEER_Q38_WORK_CLIENT_TEST
#include "../PioneerQ38WorkSession.h"
#undef TT_PCI_PROBE_Q38_DISPATCH_SUPPORT
#undef TT_PCI_PROBE_Q38_DISPATCH_RUN
#define TT_PCI_PROBE_Q38_DISPATCH_SUPPORT TT_PCI_PROBE_Q38_WORK_SUPPORT
#define TT_PCI_PROBE_Q38_DISPATCH_RUN TT_PCI_PROBE_Q38_WORK_RUN
#endif
static unsigned opens=0,closes=0,begins=0,runs=0,uploads=0,ends=0,fault=0;
static bool failed=false;
static int MockOpen(const char* path,int flags)
{
	assert(!strcmp(path,"/dev/" TT_PCI_PROBE_DEVICE) && flags==(O_RDWR|O_CLOEXEC)); opens++; return 73;
}
static int MockClose(int fd) { assert(fd==73); closes++; return 0; }
static int MockIoctl(int fd,unsigned long op,void* data,size_t bytes)
{
	assert(fd==73 && !failed);
	if (op==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT) {
		assert(bytes==sizeof(tt_q38_dispatch_support));
		*static_cast<tt_q38_dispatch_support*>(data)={TT_Q38_DISPATCH_DRIVER,
#ifdef PIONEER_Q38_WORK_CLIENT_TEST
			TT_Q38_WORK_SUPPORT_VERSION,
#else
			1,
#endif
			119,256,17408,7,64,1,0}; return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
		*static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerResidentBanks::Capacity}; return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
		assert(bytes==sizeof(tt_q38_bank_session)); begins++;
		auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={}; r.version=1;
		r.state=TT_RESIDENT_SESSION_READY; r.active=r.host_ready=1; r.bytes=PioneerResidentBanks::Total; return 0;
	}
	if (op==TT_PCI_PROBE_Q38_BANK_END) { ends++; assert(false); }
	if (op==TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD) {
		assert(bytes==sizeof(tt_q38_bank_bulk)); uploads++; auto& r=*static_cast<tt_q38_bank_bulk*>(data);
		r.result={}; r.result.pages=r.pages; r.result.host_words=r.pages*1024; r.result.host_ready=r.result.l1_prepared=1;
		auto& t=r.result.transfer; t.version=1; t.state=TT_STORE_PASSED; t.requests=t.completed=6;
		t.wrote_device=t.host_write_issued=t.host_visible=t.readback_complete=1;
		for (unsigned s=0;s<6;s++) for (unsigned c=0;c<4;c++) t.after[s][c]=c==1?r.pages*64:1;
		return 0;
	}
	assert(op==TT_PCI_PROBE_Q38_DISPATCH_RUN && bytes==sizeof(tt_q38_dispatch_probe)); runs++;
	auto& r=*static_cast<tt_q38_dispatch_probe*>(data); assert(PioneerQ38DispatchValid(r));
	r.result={}; r.result.version=1; r.result.state=TT_RUNTIME_COMPLETION_FAILED;
	r.result.recovery_required=1; r.result.root_started=1; r.result.root_phase=0x7200c;
	r.result.root_count=1; r.result.root_marker=0x464d3851; failed=true;
#ifdef PIONEER_Q38_ROOT_CYCLE_CLIENT
	if (fault>=2) {
		// Correct Work history envelope but malformed diagnostic framing.
		// REAL client must CLOSE ONLY,without another RUN or END.
		r.result.state=TT_RUNTIME_PASSED; r.result.recovery_required=0; r.result.nonce=1;
		r.result.peers=r.peers; r.result.root_count=r.peers; r.result.root_completed=r.result.root_held=1;
		r.result.root_marker=0x314d3851;
		r.result.stats[57]=fault==2?0:0x31435951;
		r.result.stats[60]=r.result.stats[62]=1; // phase0 deliberately zero
		return 0;
	}
#endif
	if (fault) { errno=EIO; return -1; }
	return 0;
}
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#define PIONEER_Q38_DISPATCH_CLIENT
#ifdef PIONEER_Q38_REUSE_CLIENT_TEST
#include "../PioneerQ38RowFixtures.h"
static bool fixtureOkay=true;
static bool MockFixtures(uint8_t* data,unsigned bytes) { assert(bytes==PIONEER_Q38_ROW_FIXTURE_BYTES); memset(data,0,bytes); return fixtureOkay; }
#define PioneerQ38LoadRowFixtures MockFixtures
#define PIONEER_Q38_REUSE_MAIN DispatchClientMain
#include "../pioneer_q38_reuse_probe.cpp"
#undef PioneerQ38LoadRowFixtures
#elif defined(PIONEER_Q38_WORK_CLIENT_TEST)
#define PIONEER_Q38_WORK_PROBE_MAIN DispatchClientMain
#include "../pioneer_q38_work_probe.cpp"
#else
#define main DispatchClientMain
#include "../pioneer_q38_cohort_probe.cpp"
#undef main
#endif
#undef ioctl
#undef close
#undef open
int main()
{
	for (unsigned kind=0;kind<
#ifdef PIONEER_Q38_ROOT_CYCLE_CLIENT
		4
#else
		2
#endif
		;kind++) {
		opens=closes=begins=runs=uploads=ends=0; failed=false; fault=kind;
		char name[]="dispatch";
#ifdef PIONEER_Q38_REUSE_CLIENT_TEST
		char arg[]="--public-mixed-inputs";
#else
		char arg[]="--synthetic";
#endif
		char* argv[]={name,arg};
		assert(DispatchClientMain(2,argv)==1 && failed && opens==1 && closes==1 && begins==1 && uploads && runs==1 && !ends);
	}
	auto numeric=std::make_unique<Q38Probe>(); *numeric={}; numeric->peers=2;
	std::vector<float> expected(512,0); FILE* diagnostics=tmpfile(); assert(diagnostics);
	assert(!ReturnedNumericErrors(*numeric,expected,diagnostics) && !ftell(diagnostics));
	for (unsigned w=0;w<2;w++) for (unsigned row=0;row<16;row++) {
		numeric->output[w][row]=0x40000000; expected[w*256+row]=1;
	}
	assert(ReturnedNumericErrors(*numeric,expected,diagnostics)==32);
	rewind(diagnostics); char line[256]; unsigned lines=0;
	while (fgets(line,sizeof(line),diagnostics)) {
		assert(strstr(line,"actual40000000 expected3f800000;HOST_COPY_ONLY_NO_ADDITIONAL_CARD_IO"));
		lines++;
	}
	assert(lines==8 && !ferror(diagnostics)); fclose(diagnostics);
	numeric->peers=0; assert(ReturnedNumericErrors(*numeric,expected,nullptr)==~0U);
	numeric->peers=120; assert(ReturnedNumericErrors(*numeric,expected,nullptr)==~0U);
	numeric->peers=2; expected.pop_back(); assert(ReturnedNumericErrors(*numeric,expected,nullptr)==~0U);
	assert(opens==1 && closes==1 && begins==1 && uploads && runs==1 && !ends);
#ifdef PIONEER_Q38_REUSE_CLIENT_TEST
	fixtureOkay=false; opens=closes=begins=runs=uploads=ends=0; failed=false;
	char name[]="mixed",arg[]="--public-mixed-inputs",bad[]="--unknown";
	char* argv[]={name,arg}; assert(DispatchClientMain(2,argv)==1 && !opens && !begins && !uploads && !runs && !ends && !closes);
	argv[1]=bad; assert(DispatchClientMain(2,argv)==2 && !opens);
	puts("PASS mixed debug client:fixture authentication and fixed CLI rejected BEFORE accelerator open;ALL case uploads precede the first RUN,first RUN failure CLOSE ONLY");
#endif
	puts("PASS numeric failure details:32 mismatches counted,ONLY first8 public HOST-copy words logged,bad shape refused,NO syscall/card IO");
	puts("PASS REAL dispatch debug client:failed ioctl/returned UNKNOWN diagnostics from HOST COPY ONLY;exactly one RUN,one CLOSE,NO subsequent ioctl/END/reset/retry;NO native/device IO");
}
