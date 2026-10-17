/* SPDX-License-Identifier: MIT */
#include "TenstorrentHaikuTile.h"
#include "TenstorrentGgufQuant.h"
#include "TenstorrentGgufTensor.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ8NativeF32Layout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Unpack.h"
#include <stdlib.h>
#include <time.h>
static double Seconds() { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static uint32_t Bits(float f) { uint32_t b; memcpy(&b,&f,4); return b; }
static int Input(unsigned k,unsigned trial) { return int((k*17+trial*29)%256)-128; }
static float Scale(unsigned b,unsigned trial) { return float(int((b*7+trial*3)%31)-15)/1024.0f; }
static bool Proof(const tt_runtime_fleet_result& r,unsigned k,unsigned n,unsigned w) { return PioneerFleetProof(r,k,n,w); }
static bool Proof(const tt_runtime_persistent_fleet_result& r,unsigned k,unsigned n,unsigned w) { return PioneerPersistentProof(r,k,n,w); }
static const tt_runtime_matvec_result& Core(const tt_runtime_fleet_result& r,unsigned w) { return r.core[w]; }
static const tt_runtime_matvec_result& Core(const tt_runtime_persistent_fleet_result& r,unsigned w) { return r.core[w].math; }
static unsigned Stopped(const tt_runtime_fleet_result& r) { return r.stopped; }
static unsigned Stopped(const tt_runtime_persistent_fleet_result&) { return 0; }
static bool ReuseProof(const tt_runtime_fleet_result&,unsigned) { return true; }
static bool ReuseProof(const tt_runtime_persistent_fleet_result& r,unsigned trial) {
	unsigned launches=0;
	for (unsigned w=0;w<r.workers;w++) { launches+=r.core[w].launched; if (r.core[w].sequence!=trial+1 || r.core[w].launched!=(trial==0)) return false; }
	printf(" Persistent idle%u actual launches%u sequence%u; RISCs remain alive until explicit END/STOP\n",r.idle,launches,r.core[0].sequence);
	return true;
}
template<class Request> int Run(char** argv,bool persistent,bool inlineMath=false) {
	unsigned numbers[3];
	for (unsigned i=0;i<3;i++) { char* end=NULL; unsigned long v=strtoul(argv[4+i],&end,10); if (!end || end==argv[4+i] || *end || v>262144) return 2; numbers[i]=unsigned(v); }
	unsigned outputs=numbers[0],workers=numbers[1],trials=numbers[2];
	if (!trials || trials>16) return 2;
	FILE* model=fopen(argv[2],"rb"); if (!model) { perror("model"); return 1; }
	PioneerGgufTensor tensor={}; PioneerFleetPlan plan;
	if (!PioneerFindGgufTensor(model,argv[3],tensor) || tensor.inputs>6144 || tensor.outputs<outputs
		|| !PioneerFleetPartition(unsigned(tensor.inputs),outputs,workers,plan) || fseeko(model,off_t(tensor.offset),SEEK_SET)) { fclose(model); return 2; }
	unsigned inputs=unsigned(tensor.inputs),blocks=inputs/32;
	uint64_t bytes=PioneerQ8CompactWeightBytes(inputs,outputs);
	uint8_t* raw=(uint8_t*)malloc(size_t(outputs)*blocks*34);
	uint32_t* packed=(uint32_t*)calloc(size_t(bytes/4),4);
	auto* request=(Request*)calloc(1,sizeof(Request));
	if (!raw || !packed || !request) { fclose(model); free(raw); free(packed); free(request); return 1; }
	bool passed=fread(raw,34,size_t(outputs)*blocks,model)==size_t(outputs)*blocks;
	fclose(model);
	for (unsigned row=0;row<outputs && passed;row++) for (unsigned b=0;b<blocks;b++) {
		const uint8_t* wb=raw+(size_t(row)*blocks+b)*34;
		uint16_t half=uint16_t(wb[0])|(uint16_t(wb[1])<<8);
		if ((half&0x7c00)==0x7c00) { passed=false; break; }
		packed[PioneerQ8NativeF32ScaleByte(inputs,b,row)/4]=PioneerQ8HalfToFloatBits(half);
		for (unsigned i=0;i<32;i++) {
			int q=wb[2+i]; if (q>=128) q-=256; uint8_t encoded=0;
			if (!PioneerInt8UnpackEncode(q,encoded)) { passed=false; break; }
			uint64_t byte=PioneerQ8NativeF32ValueByte(inputs,b*32+i,row);
			packed[byte/4]|=uint32_t(encoded)<<(8*(byte&3));
		}
	}
	setvbuf(stdout,NULL,_IONBF,0);
	int fd=passed?open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR):-1;
	if (fd<0) { free(raw); free(packed); free(request); return 1; }
	if (persistent) {
		tt_resident_session_result support={};
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&support,sizeof(support)) || support.cold
			|| !(support.capabilities&(inlineMath?TT_PIONEER_CAP_INLINE_PERSISTENT_FLEET:TT_PIONEER_CAP_PERSISTENT_FLEET))) { close(fd); free(raw); free(packed); free(request); return 1; }
	}
	tt_resident_session_result session={}; session.version=1; session.bytes=bytes;
	bool closeOnly=false,cold=false;
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&session,sizeof(session));
	passed=!status && session.active && session.state==TT_RESIDENT_SESSION_READY && !session.cold; closeOnly=status!=0; cold=session.cold;
	printf("BEGIN state%u cold%u K%u N%u workers%u retained%llu bytes\n",session.state,session.cold,inputs,outputs,plan.workers,(unsigned long long)bytes);
	for (uint64_t offset=0;offset<bytes && passed;offset+=16384) {
		tt_pioneer_bulk_upload p={}; p.version=1; p.offset=offset; p.pages=unsigned((bytes-offset)/4096); if (p.pages>4) p.pages=4;
		memcpy(p.data,packed+offset/4,p.pages*4096);
		status=ioctl(fd,TT_PCI_PROBE_PIONEER_BULK_UPLOAD,&p,sizeof(p));
		const auto& r=p.result.transfer; closeOnly=status!=0; cold=r.recovery_required;
		passed=!status && r.state==TT_STORE_PASSED && r.completed==6 && r.host_visible && r.readback_complete && !r.mismatches && !cold;
	}
	for (unsigned trial=0;trial<trials && passed;trial++) {
		size_t requestBytes=persistent?PioneerPersistentRequestBytes(outputs):PioneerFleetRequestBytes(outputs);
		memset(request,0,requestBytes); request->version=inlineMath?2:persistent?1:2; request->inputs=inputs; request->outputs=outputs; request->workers=workers;
		for (unsigned k=0;k<inputs;k++) request->input[k/2]|=uint32_t(PioneerInt8Operand(Input(k,trial)))<<(16*(k&1));
		for (unsigned b=0;b<blocks;b++) request->scale[b]=Bits(Scale(b,trial));
		double start=Seconds(); status=ioctl(fd,persistent?TT_PCI_PROBE_PIONEER_RUNTIME_PERSISTENT_FLEET:TT_PCI_PROBE_PIONEER_RUNTIME_Q8_FLEET,request,requestBytes); double elapsed=Seconds()-start;
		const auto& r=request->result; closeOnly=status!=0; cold=r.recovery_required;
		passed=!status && Proof(r,inputs,outputs,workers);
		if (!passed) closeOnly=true; // a malformed/incomplete proof authorizes no follow-up ioctl
		printf("Trial%u status%d state%u workers%u start/complete/stop%u/%u/%u pages%u cold%u %.6fs\n",trial,status,r.state,r.workers,r.started,r.completed,Stopped(r),r.staged_pages,cold,elapsed);
		for (unsigned w=0;w<r.workers && w<120;w++) if (!passed || w<2 || w+2>=r.workers) {
			const auto& c=Core(r,w); printf(" Core%u rows%u state%u s/c/s%u/%u/%u phase/tiles/marker %x/%u/%x PC%x/%x NoC %u/%u/%u/%u cold%u\n",w,plan.rows[w],c.state,c.started,c.completed,c.stopped,c.phase,c.tiles,c.marker,c.pc[0],c.pc[1],c.after[0]-c.before[0],c.after[1]-c.before[1],c.after[2]-c.before[2],c.after[3]-c.before[3],c.recovery_required);
		}
		passed=passed && ReuseProof(r,trial);
		if (!passed) break;
		unsigned differences=0;
		for (unsigned row=0;row<outputs;row++) {
			float reference=0;
			for (unsigned b=0;b<blocks;b++) {
				const uint8_t* wb=raw+(size_t(row)*blocks+b)*34; int dot=0;
				for (unsigned i=0;i<32;i++) { int q=wb[2+i]; if (q>=128) q-=256; dot+=q*Input(b*32+i,trial); }
				float d=PioneerHalfFloat(uint16_t(wb[0])|(uint16_t(wb[1])<<8)); reference=fmaf(float(dot),d*Scale(b,trial),reference);
			}
			if (Bits(reference)!=request->output[row]) { if (differences<4) printf("Mismatch row%u card%08x CPU%08x\n",row,request->output[row],Bits(reference)); differences++; }
		}
		passed=!differences; printf("Independent IEEE CPU validation: %u/%u different bits; %s\n",differences,outputs,passed?"PASS":"STOP");
		tt_runtime_profile_result profile={};
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&profile,sizeof(profile))) { closeOnly=true; passed=false; break; }
		printf("Profile format%u mask%x",profile.format,profile.mask);
		for (unsigned s=0;s<TT_PROFILE_SLOTS;s++) printf(" %llu",(unsigned long long)profile.microseconds[s]);
		puts("");
	}
	if (!cold && !closeOnly && session.active) {
		status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_END,&session,sizeof(session));
		printf("END state%u PCI/host%u/%u cold%u status%d\n",session.state,session.pci_restored,session.host_released,session.cold,status);
		passed &= !status && session.state==TT_RESIDENT_SESSION_CLOSED && session.pci_restored && session.host_released && !session.cold;
	} else fprintf(stderr,"STOP: close-only; no reset/retry/reload after uncertainty\n");
	close(fd); free(raw); free(packed); free(request); return passed?0:1;
}
int main(int argc,char** argv) {
	bool inlineMath=argc==7 && !strcmp(argv[1],"--run-inline-persistent-and-restore");
	bool persistent=inlineMath || (argc==7 && !strcmp(argv[1],"--run-persistent-and-restore"));
	if (!persistent && (argc!=7 || strcmp(argv[1],"--run-fleet-and-restore"))) {
		fprintf(stderr,"Usage: %s --run-fleet-and-restore|--run-persistent-and-restore|--run-inline-persistent-and-restore MODEL TENSOR OUTPUT_ROWS WORKERS TRIALS\nReal learned Q8_0 rows; CPU validation on changed public vectors, no inference or speedup claim.\n",argv[0]); return 2;
	}
	return persistent?Run<tt_pioneer_runtime_persistent_fleet>(argv,true,inlineMath):Run<tt_pioneer_runtime_fleet>(argv,false);
}
