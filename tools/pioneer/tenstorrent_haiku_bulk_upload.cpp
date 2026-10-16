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

static double Seconds() { timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static bool sCloseOnly=false;
static uint32_t Pattern(unsigned trial,unsigned word) { return 0x63971425U^(trial*0x9e3779b9U)^(word*0x31415927U); }
static bool End(int fd)
{
	tt_resident_session_result r={};
	bool okay=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_END,&r,sizeof(r))==0;
	printf("END state %u PCI/host %u/%u cold %u\n",r.state,r.pci_restored,r.host_released,r.cold);
	return okay && r.state==TT_RESIDENT_SESSION_CLOSED && r.pci_restored && r.host_released && !r.cold;
}
static bool Verified(const tt_resident_stream_result& r,unsigned pages)
{
	bool okay=r.version==1 && r.state==TT_STORE_PASSED && r.requests==6 && r.completed==6
		&& r.wrote_device && r.host_write_issued && r.host_visible && r.readback_complete && !r.mismatches && !r.recovery_required;
	for (unsigned step=0;step<6;step++) for (unsigned w=0;w<4;w++)
		okay &= uint32_t(r.after[step][w]-r.before[step][w])==(w==1?64*pages:1);
	return okay;
}
static bool Read(int fd,uint64_t offset,const uint32_t* words,unsigned pages)
{
	for (unsigned page=0;page<pages;page++) {
		tt_pioneer_resident_page r={}; r.version=1; r.offset=offset+page*4096;
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_STORE_READ,&r,sizeof(r)) || r.result.state!=TT_STORE_PASSED
			|| r.result.completed!=1 || r.result.mismatches || r.result.recovery_required || memcmp(r.data,words+page*1024,4096)) {
			sCloseOnly=true; fprintf(stderr,"STOP: independent legacy PIO readback failed at page %u; close only\n",page); return false;
		}
	}
	return true;
}
static bool Denied(int fd,int expected)
{
	tt_pioneer_bulk_upload r={}; r.version=1; r.pages=1; errno=0;
	return ioctl(fd,TT_PCI_PROBE_PIONEER_BULK_UPLOAD,&r,sizeof(r))<0 && errno==expected;
}
static bool Invalid(int fd)
{
	for (unsigned kind=0;kind<8;kind++) {
		tt_pioneer_bulk_upload r={}; r.version=1; r.pages=1;
		size_t bytes=sizeof(r);
		if (kind==0) r.version=2;
		if (kind==1) r.reserved[0]=1;
		if (kind==2) r.reserved[1]=1;
		if (kind==3) r.pages=0;
		if (kind==4) r.pages=5;
		if (kind==5) r.offset=1;
		if (kind==6) { r.pages=4; r.offset=24576; }
		if (kind==7) bytes--;
		errno=0;
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_BULK_UPLOAD,&r,bytes)>=0 || errno!=EINVAL) return false;
	}
	return true;
}
int main(int argc,char** argv)
{
	bool exercise=argc==2 && !strcmp(argv[1],"--compare-owned-upload");
	if (!exercise && (argc!=2 || strcmp(argv[1],"--check-support"))) {
		fprintf(stderr,"Usage: %s --check-support | --compare-owned-upload\nSynthetic fixed-pattern uploads in a private 32KiB bank-0 arena, independent full PIO readback, matched timings; no model/core start.\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IOLBF,0);
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
	if (fd<0) { perror("open"); return 1; }
	tt_resident_session_result session={};
	bool okay=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&session,sizeof(session))==0
		&& session.version==1 && (session.capabilities&TT_PIONEER_CAP_BULK_UPLOAD) && !session.cold;
	printf("Bulk-upload support v%u capabilities 0x%x cold %u\n",session.version,session.capabilities,session.cold);
	if (!okay || !exercise) { close(fd); return okay?0:1; }
	session={}; session.version=1; session.bytes=32768;
	okay=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&session,sizeof(session))==0
		&& session.state==TT_RESIDENT_SESSION_READY && session.active && session.host_ready && !session.cold;
	if (!okay) { perror("BEGIN"); close(fd); return 1; }
	int foreign=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
	okay=foreign>=0 && Denied(foreign,EBUSY) && Invalid(fd);
	fflush(stdout); pid_t child=okay?fork():-1;
	if (child==0) _exit(Denied(fd,EPERM)?0:1);
	int status=0;
	okay=okay && child>0 && waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0;
	if (foreign>=0) close(foreign);
	printf("Request bounds, foreign-FD and inherited-team denial %s\n",okay?"PASS":"FAIL");
	uint32_t data[4096],expected[8192]; double legacyTotal=0,bulkTotal=0; unsigned bytes=0;
	for (unsigned w=0;w<8192;w++) expected[w]=Pattern(97,w);
	for (unsigned p=0;okay && p<8;p++) {
		tt_pioneer_resident_stream r={}; r.version=1; r.offset=p*4096; memcpy(r.data,expected+p*1024,4096);
		okay=ioctl(fd,TT_PCI_PROBE_PIONEER_STREAM_UPLOAD,&r,sizeof(r))==0 && Verified(r.result,1);
		sCloseOnly |= !okay;
	}
	for (unsigned trial=0;okay && trial<14;trial++) {
		unsigned pages=trial<4?trial+1:4; uint64_t offset=(trial&1)?4096:0;
		for (unsigned w=0;w<4096;w++) data[w]=Pattern(trial,w);
		// Alternate order after the four shape/setup trials. Each timed
		// operation includes all six transfer proofs and CPU comparisons.
		for (unsigned which=0;okay && which<2;which++) {
			bool bulk=(which^(trial&1))!=0;
			// Distinct data for each path prevents an omitted second upload
			// from passing merely because the previous path wrote the same bytes.
			for (unsigned w=0;w<4096;w++) data[w]=Pattern(trial*2+which,w);
			double start=Seconds();
			if (bulk) {
				tt_pioneer_bulk_upload r={}; r.version=1; r.pages=pages; r.offset=offset; memcpy(r.data,data,pages*4096);
				okay=ioctl(fd,TT_PCI_PROBE_PIONEER_BULK_UPLOAD,&r,sizeof(r))==0 && Verified(r.result.transfer,pages)
					&& r.result.pages==pages && r.result.host_words==pages*1024 && r.result.host_ready && r.result.l1_prepared;
			} else for (unsigned p=0;okay && p<pages;p++) {
				tt_pioneer_resident_stream r={}; r.version=1; r.offset=offset+p*4096; memcpy(r.data,data+p*1024,4096);
				okay=ioctl(fd,TT_PCI_PROBE_PIONEER_STREAM_UPLOAD,&r,sizeof(r))==0 && Verified(r.result,1);
			}
			double elapsed=Seconds()-start;
			sCloseOnly |= !okay;
			printf("trial %u %s pages %u offset %llu upload %.6fs %s\n",trial,bulk?"BULK":"4KiB",pages,(unsigned long long)offset,elapsed,okay?"PASS":"FAIL");
			if (trial>=4) { if (bulk) bulkTotal+=elapsed; else legacyTotal+=elapsed; }
			if (okay) {
				memcpy(expected+offset/4,data,pages*4096);
				okay=Read(fd,0,expected,8); // includes every untouched neighbour
			}
		}
		if (trial>=4) bytes+=pages*4096;
	}
	if (okay) printf("MATCHED bytes %u legacy %.6fs bulk %.6fs speedup %.3fx; setup excluded, every uploaded word independently PIO-checked\n",bytes,legacyTotal,bulkTotal,bulkTotal>0?legacyTotal/bulkTotal:0);
	bool clean=!sCloseOnly && End(fd); close(fd);
	if (sCloseOnly) fprintf(stderr,"STOP: uncertain transfer; final close only, no END/retry/reset/reload\n");
	printf("%s: owned bulk upload and clean release; not full-model qualification\n",okay&&clean?"PASS":"FAIL");
	return okay&&clean?0:1;
}
