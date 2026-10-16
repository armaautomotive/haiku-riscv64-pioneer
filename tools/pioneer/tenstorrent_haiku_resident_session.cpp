/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static double Seconds() { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static int Device() { return open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); }
static bool Legacy(int fd,bool busy)
{
	uint32_t version=0; errno=0;
	int result=ioctl(fd,TT_PCI_PROBE_PIONEER_HOST_READ_PAGE_SUPPORT,&version,sizeof(version));
	return busy?result<0 && errno==EBUSY:result==0 && version==1;
}
static bool QuantizedDenied(int fd,uint32_t capabilities,int expected)
{
	tt_pioneer_runtime_q8_matvec request={}; request.matvec.version=1; request.matvec.inputs=1024; request.matvec.outputs=32;
	bool denied=true;
	if (capabilities&1024) {
		errno=0; denied &= ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8,&request,sizeof(request))<0 && errno==expected;
	}
	if (capabilities&2048) {
		errno=0; denied &= ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_COMPACT,&request,sizeof(request))<0 && errno==expected;
	}
	if (capabilities&TT_PIONEER_CAP_NATIVE_Q8_UNPACK) {
		errno=0; denied &= ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE,&request,sizeof(request))<0 && errno==expected;
	}
	if (capabilities&TT_PIONEER_CAP_NATIVE_Q8_F32_SCALES) {
		errno=0; denied &= ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE_F32_SCALES,&request,sizeof(request))<0 && errno==expected;
	}
	if (capabilities&TT_PIONEER_CAP_Q8_BULK) {
		errno=0; denied &= ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_BULK,&request,sizeof(request))<0 && errno==expected;
	}
	return denied;
}
static bool Begin(int fd,tt_resident_session_result& r,uint64_t bytes=8192)
{
	r={}; r.version=1; r.bytes=bytes;
	return ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&r,sizeof(r))==0
		&& r.version==1 && r.state==TT_RESIDENT_SESSION_READY && r.active && r.host_ready
		&& !r.pci_restored && !r.host_released && r.host_address_bits>32 && !r.cold && (r.capabilities&3)==3;
}
static bool Stream(int fd,uint64_t offset,const uint32_t* data,bool& cold)
{
	tt_pioneer_resident_stream p={}; p.version=1; p.offset=offset; memcpy(p.data,data,4096);
	double start=Seconds();
	if (ioctl(fd,TT_PCI_PROBE_PIONEER_STREAM_UPLOAD,&p,sizeof(p))) { perror("resident stream"); return false; }
	double elapsed=Seconds()-start;
	const auto& r=p.result; cold=r.recovery_required!=0;
	printf("STREAM offset %llu state %u requests/completed %u/%u CPU-visible/readback %u/%u mismatches %u cold %u\n",
		(unsigned long long)offset,r.state,r.requests,r.completed,r.host_visible,r.readback_complete,r.mismatches,r.recovery_required);
	printf("  whole 4KiB stream ioctl %.6fs, including full validation; not a bandwidth benchmark\n",elapsed);
	for (unsigned step=0;step<6;step++) printf("  step %u response-or-ACK/flits/accepted/sent %u/%u/%u/%u polls %u\n",step,
		uint32_t(r.after[step][0]-r.before[step][0]),uint32_t(r.after[step][1]-r.before[step][1]),
		uint32_t(r.after[step][2]-r.before[step][2]),uint32_t(r.after[step][3]-r.before[step][3]),r.polls[step]);
	return r.version==1 && r.state==TT_STORE_PASSED && r.completed==6 && r.host_write_issued && r.host_visible && r.readback_complete && !r.mismatches && !cold;
}
static bool Chunk(int fd,uint64_t offset,bool upload,uint32_t* data,bool& cold,bool stream=false)
{
	if (upload && stream) return Stream(fd,offset,data,cold);
	tt_pioneer_resident_page p={}; p.version=1; p.offset=offset;
	if (upload) memcpy(p.data,data,4096);
	double start=Seconds();
	if (ioctl(fd,upload?TT_PCI_PROBE_PIONEER_STORE_UPLOAD:TT_PCI_PROBE_PIONEER_STORE_READ,&p,sizeof(p))) {
		perror("resident chunk"); return false;
	}
	double elapsed=Seconds()-start;
	const auto& r=p.result; cold=r.recovery_required!=0;
	printf("%s offset %llu state %u requests/completed %u/%u mismatches %u cold %u\n",
		upload?"UPLOAD":"READ",(unsigned long long)offset,r.state,r.requests,r.completed,r.mismatches,r.recovery_required);
	printf("  whole 4KiB legacy ioctl %.6fs, including PIO validation; not a bandwidth benchmark\n",elapsed);
	for (unsigned step=0;step<(upload?3U:1U);step++) printf("  step %u response-or-ACK/flits/accepted/sent %u/%u/%u/%u polls %u\n",step,
		uint32_t(r.after[step][0]-r.before[step][0]),uint32_t(r.after[step][1]-r.before[step][1]),
		uint32_t(r.after[step][2]-r.before[step][2]),uint32_t(r.after[step][3]-r.before[step][3]),r.polls[step]);
	bool passed=r.version==1 && r.state==TT_STORE_PASSED && r.completed==(upload?3U:1U) && !r.mismatches && !cold;
	if (passed && !upload) passed=memcmp(data,p.data,4096)==0;
	return passed;
}
static bool StoreFile(int fd,const char* path,bool automaticClose,bool stream)
{
	uint32_t first[1024],second[1024];
	FILE* file=fopen(path,"rb");
	if (!file) { perror("model file"); return false; }
	bool read=fseek(file,32*1024*1024,SEEK_SET)==0 && fread(first,1,4096,file)==4096 && fread(second,1,4096,file)==4096;
	fclose(file);
	if (!read || !memcmp(first,second,4096)) { fprintf(stderr,"Need two distinct complete model chunks at file offset 32MiB\n"); return false; }
	printf("Public model chunks at 32MiB: hashes %08x/%08x\n",PioneerResidentPageHash(first),PioneerResidentPageHash(second));
	tt_resident_session_result info={};
	if (!Begin(fd,info,12288)) { perror("store begin"); return false; }
	bool cold=false,passed=Legacy(fd,true);
	tt_pioneer_resident_page invalid={}; invalid.version=1; invalid.offset=4096; errno=0;
	passed &= ioctl(fd,TT_PCI_PROBE_PIONEER_STORE_READ,&invalid,sizeof(invalid))<0 && errno==ENOENT;
	invalid.offset=12288; errno=0;
	passed &= ioctl(fd,TT_PCI_PROBE_PIONEER_STORE_UPLOAD,&invalid,sizeof(invalid))<0 && errno==EINVAL;
	if (stream) {
		tt_pioneer_resident_stream bad={}; bad.version=1; bad.offset=12288; errno=0;
		passed &= ioctl(fd,TT_PCI_PROBE_PIONEER_STREAM_UPLOAD,&bad,sizeof(bad))<0 && errno==EINVAL;
		bad.offset=0; bad.reserved=1; errno=0;
		passed &= ioctl(fd,TT_PCI_PROBE_PIONEER_STREAM_UPLOAD,&bad,sizeof(bad))<0 && errno==EINVAL;
	}
	passed=passed && Chunk(fd,0,true,first,cold,stream) && Chunk(fd,8192,true,second,cold,stream)
		&& Chunk(fd,0,false,first,cold) && Chunk(fd,8192,false,second,cold)
		&& Chunk(fd,0,true,second,cold,stream) && Chunk(fd,0,false,second,cold) && Chunk(fd,8192,false,second,cold);
	if (cold) { fprintf(stderr,"STOP: cold recovery required; close only, no retry or reload\n"); return false; }
	if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_STATUS,&info,sizeof(info))) passed=false;
	printf("Persistent session uploaded pages %u; active %u cold %u\n",info.uploaded_pages,info.active,info.cold);
	passed &= info.active && info.uploaded_pages==2 && !info.cold;
	if (automaticClose) {
		printf("Leaving initialized storage to final-FD-close cleanup\n");
		return passed;
	}
	if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_END,&info,sizeof(info))) { perror("store end"); return false; }
	printf("Store close state %u PCI/host restored/released %u/%u cold %u\n",info.state,info.pci_restored,info.host_released,info.cold);
	passed &= info.state==TT_RESIDENT_SESSION_CLOSED && info.pci_restored && info.host_released && !info.cold;
	return passed && Legacy(fd,false);
}
int main(int argc,char** argv)
{
	bool support=argc==2 && !strcmp(argv[1],"--check-support");
	bool automaticClose=argc==3 && !strcmp(argv[1],"--store-file-chunks-close");
	bool stream=argc==3 && !strcmp(argv[1],"--store-file-chunks-stream");
	bool store=argc==3 && (!strcmp(argv[1],"--store-file-chunks") || automaticClose || stream);
	if (!support && !store && (argc!=2 || strcmp(argv[1],"--exercise-owned-lifecycle"))) {
		fprintf(stderr,"Usage: %s --check-support | --exercise-owned-lifecycle | --store-file-chunks MODEL_FILE | --store-file-chunks-close MODEL_FILE | --store-file-chunks-stream MODEL_FILE\nExclusive FD/team-owned mappings and host-buffer lifecycle; optional two model-data chunks retained/read back in bank 0. No inference/core start.\n",argv[0]); return 2;
	}
	int fd=Device(); if (fd<0) { perror("open"); return 1; }
	tt_resident_session_result r={};
	if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&r,sizeof(r))) { perror("session support"); close(fd); return 1; }
	printf("Resident session support v%u capabilities %u (bits: mappings 1, host buffer 2, storage 4, inter-step guards 8)\n",r.version,r.capabilities);
	if (r.version!=1 || (r.capabilities&3)!=3) { close(fd); return 1; }
	if (support) { close(fd); return 0; }
	if (store) {
		bool passed=(r.capabilities&12)==12 && (!stream || (r.capabilities&32)) && StoreFile(fd,argv[2],automaticClose,stream); close(fd);
		if (passed && automaticClose) {
			int check=Device(); passed=check>=0 && Legacy(check,false); if (check>=0) close(check);
		}
		printf("%s: persistent model-data storage across ioctls, NOT model inference or speedup\n",passed?"PASS":"FAIL");
		return passed?0:1;
	}
	int foreign=Device(); if (foreign<0) { perror("foreign open"); close(fd); return 1; }
	bool passed=Begin(fd,r);
	printf("Begin state %u active/host-ready %u/%u high-address bits %u; PCI held %u cold %u\n",
		r.state,r.active,r.host_ready,r.host_address_bits,!r.pci_restored,r.cold);
	if (passed) {
		passed=Legacy(foreign,true) && Legacy(fd,true);
		passed &= QuantizedDenied(foreign,r.capabilities,EBUSY);
		if (r.capabilities&256) {
			tt_runtime_profile_result profile={};
			passed &= ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&profile,sizeof(profile))==0
				&& profile.version==1 && !profile.available && !profile.sequence && !profile.mask;
			errno=0; passed &= ioctl(foreign,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&profile,sizeof(profile))<0 && errno==EBUSY;
		}
		if (r.capabilities&32) {
			tt_pioneer_resident_stream otherStream={}; otherStream.version=1; errno=0;
			passed &= ioctl(foreign,TT_PCI_PROBE_PIONEER_STREAM_UPLOAD,&otherStream,sizeof(otherStream))<0 && errno==EBUSY;
		}
		tt_resident_session_result other={}; other.version=1; other.bytes=8192;
		errno=0; passed &= ioctl(foreign,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&other,sizeof(other))<0 && errno==EBUSY;
		errno=0; passed &= ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&other,sizeof(other))<0 && errno==EBUSY;
		fflush(stdout); // Do not duplicate buffered diagnostic lines across fork.
		pid_t child=fork();
		if (child==0) {
			tt_resident_session_result info={}; errno=0;
			bool denied=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_STATUS,&info,sizeof(info))<0 && errno==EPERM;
			denied &= QuantizedDenied(fd,r.capabilities,EPERM);
			if (r.capabilities&256) {
				tt_runtime_profile_result profile={}; errno=0;
				denied &= ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&profile,sizeof(profile))<0 && errno==EPERM;
			}
			if (r.capabilities&32) {
				tt_pioneer_resident_stream inherited={}; inherited.version=1; errno=0;
				denied &= ioctl(fd,TT_PCI_PROBE_PIONEER_STREAM_UPLOAD,&inherited,sizeof(inherited))<0 && errno==EPERM;
			}
			_exit(denied?0:1);
		}
		int status=0;
		passed &= child>0 && waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0;
		int duplicate=dup(fd);
		if (duplicate>=0) {
			close(fd); fd=duplicate;
			tt_resident_session_result info={};
			passed &= ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_STATUS,&info,sizeof(info))==0 && info.active && info.host_ready;
			if (r.capabilities&256) {
				tt_runtime_profile_result profile={};
				passed &= ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&profile,sizeof(profile))==0 && profile.version==1 && !profile.available;
			}
		} else passed=false;
		printf("Foreign FD/legacy conflict gates, inherited-team denial and duplicate-FD retention %s\n",passed?"PASS":"FAIL");
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_END,&r,sizeof(r))) { perror("session end"); passed=false; }
		printf("End state %u active %u PCI-restored/host-released %u/%u cold %u\n",r.state,r.active,r.pci_restored,r.host_released,r.cold);
		passed &= r.state==TT_RESIDENT_SESSION_CLOSED && !r.active && r.pci_restored && r.host_released && !r.cold;
		if (r.capabilities&256) {
			tt_runtime_profile_result profile={};
			passed &= ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&profile,sizeof(profile))<0;
		}
	}
	close(fd);
	if (passed) passed=Legacy(foreign,false);
	if (passed) {
		fd=Device();
		passed=fd>=0 && Begin(fd,r);
		if (fd>=0) close(fd); // deliberate no-END path: final close must clean up
		passed &= Legacy(foreign,false);
		printf("Final-FD-close automatic cleanup %s\n",passed?"PASS":"FAIL");
	}
	close(foreign);
	printf("%s: exclusive lifecycle only, no model/DMA/core execution\n",passed?"PASS":"FAIL");
	return passed?0:1;
}
