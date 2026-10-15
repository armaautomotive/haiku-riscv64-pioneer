/* SPDX-License-Identifier: MIT */
#ifndef TENSTORRENT_HAIKU_RESIDENT_H
#define TENSTORRENT_HAIKU_RESIDENT_H
#include "TenstorrentHaikuTile.h"
#include "PioneerResidentTensor.h"
#include <stdlib.h>

// Separate from the legacy tile backend. It never retries after an error,
// never substitutes a CPU product, and owns one exclusive session lifetime.
class PioneerResidentDevice {
	int fd=-1;
	bool active=false,failed=false,cold=false,finished=false,stream=false;
	tt_pioneer_runtime_matvec* request=NULL;
public:
	uint64_t uploads=0,calls=0;
	~PioneerResidentDevice() { Finish(); free(request); }
	bool Open(uint64_t bytes) {
		if (fd>=0 || failed || finished || !PioneerResidentCapacity(bytes)) return false;
		fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
		if (fd<0) { perror("resident open"); failed=true; return false; }
		tt_resident_session_result s={};
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&s,sizeof(s)) || s.version!=1 || (s.capabilities&31)!=31) {
			fprintf(stderr,"Resident matvec capability unavailable; no compute submitted\n"); failed=true; return false;
		}
		stream=(s.capabilities&32)!=0;
		fprintf(stderr,"Resident upload path: %s\n",stream?"verified six-step stream":"legacy PIO-checked");
		request=(tt_pioneer_runtime_matvec*)calloc(1,sizeof(*request));
		if (!request) { failed=true; return false; }
		s={}; s.version=1; s.bytes=bytes;
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&s,sizeof(s)); active=s.active!=0; cold=s.cold!=0;
		if (status || !active || s.state!=TT_RESIDENT_SESSION_READY || cold) {
			fprintf(stderr,"Resident BEGIN failed: state %u active %u cold %u\n",s.state,s.active,s.cold); failed=true; return false;
		}
		return true;
	}
	bool Upload(uint64_t offset,const uint32_t* data) {
		if (!active || failed || finished || cold || !data) return false;
		int status=0; uint32_t state=0; bool complete=false;
		if (stream) {
			tt_pioneer_resident_stream p={}; p.version=1; p.offset=offset; memcpy(p.data,data,4096);
			status=ioctl(fd,TT_PCI_PROBE_PIONEER_STREAM_UPLOAD,&p,sizeof(p));
			const auto& r=p.result; cold=r.recovery_required!=0; state=r.state;
			complete=r.version==1 && r.requests==6 && r.completed==6 && r.host_write_issued && r.host_visible && r.readback_complete && !r.mismatches;
		} else {
			tt_pioneer_resident_page p={}; p.version=1; p.offset=offset; memcpy(p.data,data,4096);
			status=ioctl(fd,TT_PCI_PROBE_PIONEER_STORE_UPLOAD,&p,sizeof(p)); cold=p.result.recovery_required!=0; state=p.result.state;
			complete=p.result.version==1 && p.result.completed==3 && !p.result.mismatches;
		}
		if (status || state!=TT_STORE_PASSED || !complete || cold) {
			fprintf(stderr,"Resident upload STOP: status %d state %u offset %llu cold %u\n",status,state,(unsigned long long)offset,cold);
			failed=true; return false;
		}
		uploads++;
		if (!(uploads%4096)) fprintf(stderr,"Resident upload verified: %llu MiB\n",(unsigned long long)(uploads/256));
		return true;
	}
	bool Matvec(uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* vector,uint32_t* result) {
		if (!active || failed || finished || cold || !vector || !result || !PioneerMatvecShape(inputs,outputs)) return false;
		memset(request,0,sizeof(*request)); request->version=1; request->inputs=inputs; request->outputs=outputs; request->weight_offset=offset;
		memcpy(request->input,vector,inputs*2);
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC,request,sizeof(*request));
		const auto& r=request->result; cold=r.recovery_required!=0;
		if (status || r.version!=1 || r.state!=TT_RUNTIME_PASSED || !r.started || !r.completed || !r.stopped || cold) {
			fprintf(stderr,"Resident matvec STOP: status %d state %u start/complete/stop %u/%u/%u phase/tiles/PC %x/%u/%x cold %u; no fallback/retry\n",
				status,r.state,r.started,r.completed,r.stopped,r.phase,r.tiles,r.pc[1],cold); failed=true; return false;
		}
		memcpy(result,request->output,outputs*4); calls++; return true;
	}
	bool Finish() {
		if (finished) return !cold;
		finished=true;
		if (active && !cold) {
			tt_resident_session_result s={};
			int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_END,&s,sizeof(s));
			cold=status || s.state!=TT_RESIDENT_SESSION_CLOSED || !s.pci_restored || !s.host_released || s.cold;
			fprintf(stderr,"Resident END: state %u PCI/host %u/%u cold %u\n",s.state,s.pci_restored,s.host_released,cold);
		}
		if (cold) fprintf(stderr,"STOP: cold recovery required; no retry/reset/module reload\n");
		if (fd>=0) { close(fd); fd=-1; }
		active=false; return !cold;
	}
};
#endif
