/* SPDX-License-Identifier: MIT */
// Execute the REAL metadata-only CLI branch. Any compute ioctl is a failure.
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
static tt_coordinated_support fixture={};
static unsigned opened=0,closed=0,queries=0;
static int queryStatus=0;
static int MockOpen(const char* name,int flags) {
	assert(!strcmp(name,"/dev/" TT_PCI_PROBE_DEVICE) && flags==O_RDWR); opened++; return 91;
}
static int MockClose(int fd) { assert(fd==91); closed++; return 0; }
static int MockIoctl(int fd,unsigned long op,void* data,size_t bytes) {
	assert(fd==91 && op==TT_PCI_PROBE_PIONEER_COORDINATED_SUPPORT && bytes==sizeof(fixture));
	*static_cast<tt_coordinated_support*>(data)=fixture; queries++; return queryStatus;
}
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#define main PioneerSupportCliMain
#include "../tenstorrent_haiku_resident_attention.cpp"
#undef main
#undef ioctl
#undef close
#undef open
static bool Run() {
	opened=closed=queries=0;
	char program[]="support-test",option[]="--check-coordinated-support";
	char* args[]={program,option,nullptr};
	bool okay=PioneerSupportCliMain(2,args)==0;
	assert(opened==1 && closed==1 && queries==1);
	return okay;
}
int main() {
	for (unsigned version=0;version<65;version++) {
		fixture={version,119,3,1,8192}; queryStatus=0;
		assert(Run()==(version>=7 && version<=43));
	}
	for (unsigned version:{41U,42U,43U}) for (unsigned word=1;word<5;word++) for (unsigned bit=0;bit<32;bit++) {
		fixture={version,119,3,1,8192}; reinterpret_cast<uint32_t*>(&fixture)[word]^=uint32_t(1)<<bit;
		assert(!Run());
	}
	fixture={42,119,3,1,8192}; queryStatus=-1; assert(!Run());
	puts("PASS REAL support CLI versions7..43/unknown refusal/ALL worker-KV-immutable-gather bit faults/status error,exactly ONE metadata query/close and NO BEGIN or compute;NO hardware");
}
