/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <initializer_list>
#include "../TenstorrentHaikuTile.h"
#include "../TenstorrentGgufQuant.h"
#include "../TenstorrentGgufTensor.h"
#include "../PioneerFloat16.h"
// Compile the REAL diagnostic main against synthetic syscalls/data. Never
// open a card, model download, socket or Assistant. This is not math evidence.
static FILE* sModel=NULL;
static unsigned sCalls=0,sFail=0,sUploads=0,sClose=0,sEnd=0,sUnsafeOperation=0;
static unsigned sProfileSequence=0,sProfileFormat=0;
static bool sProfiling=false;
static bool sFailed=false;
static int MockOpen(const char* name,int flags) { assert(!strcmp(name,"/dev/" TT_PCI_PROBE_DEVICE) && flags==O_RDWR); return 73; }
static int MockClose(int fd) { assert(fd==73); sClose++; return 0; }
static FILE* MockFopen(const char* name,const char* mode) {
	assert(!strcmp(name,"SYNTHETIC_MODEL") && !strcmp(mode,"rb") && sModel);
	FILE* file=sModel; sModel=NULL; return file;
}
static bool MockTensor(FILE*,const char*,PioneerGgufTensor& tensor) {
	tensor={}; tensor.inputs=1024; tensor.outputs=32; tensor.type=8; tensor.bytes=32*32*34; return true;
}
static int MockIoctl(int fd,unsigned long operation,void* data,size_t) {
	assert(fd==73 && !sFailed); // any subsequent card command is a test failure
	if (++sCalls==sFail) { sFailed=true; errno=EIO; return -1; }
	if (operation==TT_PCI_PROBE_PIONEER_SESSION_SUPPORT) {
		auto& r=*static_cast<tt_resident_session_result*>(data); r.version=1; r.capabilities=31|64|(sProfiling?256:0); return 0;
	}
	if (operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN || operation==TT_PCI_PROBE_PIONEER_SESSION_STATUS) {
		auto& r=*static_cast<tt_resident_session_result*>(data); r.version=1;
		r.active=r.host_ready=1; r.state=TT_RESIDENT_SESSION_READY; r.uploaded_pages=sUploads;
		if ((sUnsafeOperation==4 && operation==TT_PCI_PROBE_PIONEER_SESSION_STATUS)
			|| (sUnsafeOperation==5 && operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN)) { r.cold=1; sFailed=true; }
		return 0;
	}
	if (operation==TT_PCI_PROBE_PIONEER_STORE_UPLOAD) {
		auto& r=static_cast<tt_pioneer_resident_page*>(data)->result;
		r.version=1; r.state=TT_STORE_PASSED; r.requests=r.completed=3; sUploads++;
		if (sUnsafeOperation==1) { r.completed=2; sFailed=true; } return 0;
	}
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_DMA) {
		sProfileFormat=operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_DMA?1:0;
		auto& request=*static_cast<tt_pioneer_runtime_matvec*>(data); auto& r=request.result;
		r.version=1; r.state=TT_RUNTIME_PASSED; r.started=r.completed=r.stopped=1;
		float sum=0;
		for (unsigned k=0;k<request.inputs;k++) sum+=PioneerTileFloat((request.input[k/2]>>(16*(k&1)))<<16);
		uint32_t bits=0; memcpy(&bits,&sum,4);
		for (unsigned row=0;row<request.outputs;row++) request.output[row]=bits;
		if (sUnsafeOperation==2) { r.completed=0; sFailed=true; }
		if (sUnsafeOperation==3) { r.stopped=0; sFailed=true; }
		return 0;
	}
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE) {
		auto& p=*static_cast<tt_runtime_profile_result*>(data); p.version=1; p.available=p.clock_stable=1;
		p.sequence=++sProfileSequence; p.format=sProfileFormat; p.mask=p.format?0x1df:0x17f;
		p.state=TT_RUNTIME_PASSED; p.started=p.completed=p.stopped=1;
		for (unsigned slot=0;slot<TT_PROFILE_SLOTS;slot++) p.microseconds[slot]=slot*100+100;
		if (sUnsafeOperation==6) { p.recovery_required=1; sFailed=true; }
		if (sUnsafeOperation==7) { p.completed=0; sFailed=true; }
		if (sUnsafeOperation==8) { p.stopped=0; sFailed=true; }
		return 0;
	}
	assert(operation==TT_PCI_PROBE_PIONEER_SESSION_END);
	auto& r=*static_cast<tt_resident_session_result*>(data); r.active=0; r.state=TT_RESIDENT_SESSION_CLOSED;
	r.pci_restored=r.host_released=1; sEnd++; return 0;
}
#define main PioneerRuntimeCliMain
#define open MockOpen
#define close MockClose
#define fopen MockFopen
#define ioctl MockIoctl
#define PioneerFindGgufTensor MockTensor
#include "../tenstorrent_haiku_runtime_matvec.cpp"
#undef PioneerFindGgufTensor
#undef ioctl
#undef fopen
#undef close
#undef open
#undef main
static int RunCli(unsigned fail,unsigned unsafe=0,bool profiling=false) {
	sCalls=sUploads=sClose=sEnd=0; sFail=fail; sUnsafeOperation=unsafe; sFailed=false;
	sProfiling=profiling; sProfileSequence=sProfileFormat=0;
	sModel=tmpfile(); assert(sModel);
	uint8_t block[34]; memset(block,1,sizeof(block)); block[0]=0; block[1]=0x3c; // d=1, all quants=1
	for (unsigned row=0;row<32;row++) for (unsigned b=0;b<32;b++) assert(fwrite(block,1,34,sModel)==34);
	assert(!fseek(sModel,0,SEEK_SET));
	char name[]="synthetic-runtime-cli",mode[]="--compare-qwen-resident-output-dma",model[]="SYNTHETIC_MODEL";
	char* argv[]={name,mode,model}; int status=PioneerRuntimeCliMain(3,argv);
	if (sModel) { fclose(sModel); sModel=NULL; }
	assert(sClose==1); return status;
}
int main() {
	unsigned cases=0;
	for (bool profiling:{false,true}) {
		assert(!RunCli(0,0,profiling) && sEnd==1); unsigned calls=sCalls;
		for (unsigned call=1;call<=calls;call++) {
			assert(RunCli(call,0,profiling)==1 && sFailed && sCalls==call && !sEnd); cases++;
		}
	}
	for (unsigned unsafe=1;unsafe<=8;unsafe++) assert(RunCli(0,unsafe,true)==1 && sFailed && !sEnd);
	printf("PASS: SYNTHETIC real CLI, all%u ioctl failures and incomplete transfer/compute/stop, cold session and unsafe profile cause close-only with NO follow-up card command; NO device/model/performance evidence\n",cases);
}
