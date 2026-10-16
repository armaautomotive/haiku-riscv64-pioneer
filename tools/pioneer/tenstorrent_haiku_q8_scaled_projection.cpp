/* SPDX-License-Identifier: MIT */
#include "TenstorrentHaikuTile.h"
#include "TenstorrentGgufQuant.h"
#include "TenstorrentGgufTensor.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Unpack.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ8NativeF32Layout.h"
#include <stdlib.h>
#include <time.h>
static double Seconds() { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static uint32_t Bits(float f) { uint32_t v; memcpy(&v,&f,4); return v; }
static int Input(unsigned k,unsigned trial) { return int((k*17+trial*29)%256)-128; }
static float Scale(unsigned block,unsigned trial) { return float(int((block*7+trial*3)%31)-15)/1024.0f; }
int main(int argc,char** argv)
{
	bool f32Scales=argc>=2 && (!strcmp(argv[1],"--run-qwen-q8-native-f32-scales") || !strcmp(argv[1],"--check-native-f32-scale-support"));
	bool native=f32Scales || (argc>=2 && (!strcmp(argv[1],"--run-qwen-q8-native") || !strcmp(argv[1],"--check-native-support")));
	bool compact=native || (argc>=2 && (!strcmp(argv[1],"--run-qwen-q8-compact") || !strcmp(argv[1],"--check-compact-support")));
	bool support=argc==2 && (!strcmp(argv[1],"--check-native-f32-scale-support") || !strcmp(argv[1],"--check-support") || !strcmp(argv[1],"--check-compact-support") || !strcmp(argv[1],"--check-native-support"));
	unsigned outputs=32;
	if (argc==5) {
		char* end=NULL; unsigned long n=strtoul(argv[4],&end,10);
		if (!end || end==argv[4] || *end || n<16 || n>4096 || (n&15)) return 2;
		outputs=unsigned(n);
	}
	if (!support && ((argc!=3 && argc!=5) || (!compact && strcmp(argv[1],"--run-qwen-q8-scaled")))) {
		fprintf(stderr,"Usage: %s --check-support|--check-compact-support|--check-native-support|--check-native-f32-scale-support | --run-qwen-q8-scaled|--run-qwen-q8-compact|--run-qwen-q8-native|--run-qwen-q8-native-f32-scales MODEL [TENSOR OUTPUT_ROWS]\nReal retained Q8_0 weights, two independent signed-byte vectors and per-block scales. Native signed unpack refuses weight -128; never clamps. NOT full inference or a CPU speedup.\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0);
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	tt_resident_session_result session={};
	if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&session,sizeof(session))) { perror("support"); close(fd); return 1; }
	unsigned capability=f32Scales?TT_PIONEER_CAP_NATIVE_Q8_F32_SCALES:native?TT_PIONEER_CAP_NATIVE_Q8_UNPACK:compact?2048:1024;
	printf("Session capabilities %u; %s Q8 bit%u required\n",session.capabilities,native?"native INT8 unpack":compact?"compact":"scaled",capability);
	if (session.version!=1 || !(session.capabilities&capability)) { close(fd); return 1; }
	if (support) { close(fd); return 0; }
	FILE* model=fopen(argv[2],"rb"); if (!model) { perror("model"); close(fd); return 1; }
	PioneerGgufTensor tensor={};
	if (!PioneerFindGgufTensor(model,argc==5?argv[3]:"blk.0.attn_q.weight",tensor)
		|| tensor.inputs>TT_RUNTIME_MATVEC_INPUTS_MAX || tensor.outputs<outputs || !(compact?PioneerQ8CompactShape(unsigned(tensor.inputs),outputs):PioneerMatvecShape(unsigned(tensor.inputs),outputs))
		|| fseeko(model,off_t(tensor.offset),SEEK_SET)) { fprintf(stderr,"Unsupported Q8_0 tensor\n"); fclose(model); close(fd); return 1; }
	unsigned inputs=unsigned(tensor.inputs),blocks=inputs/32;
	unsigned bytes=unsigned(compact?PioneerQ8CompactWeightBytes(inputs,outputs):PioneerMatvecWeightBytes(inputs,outputs));
	uint32_t operation=f32Scales?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE_F32_SCALES:native?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE:compact?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_COMPACT:TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8;
	uint8_t* raw=(uint8_t*)malloc(size_t(outputs)*blocks*34);
	uint32_t* packed=(uint32_t*)calloc(bytes/4,4);
	tt_pioneer_runtime_q8_matvec* request=(tt_pioneer_runtime_q8_matvec*)calloc(1,sizeof(*request));
	if (!raw || !packed || !request) { free(raw); free(packed); free(request); fclose(model); close(fd); return 1; }
	bool passed=fread(raw,34,size_t(outputs)*blocks,model)==size_t(outputs)*blocks,cold=false,closeOnly=false;
	fclose(model);
	for (unsigned row=0;row<outputs && passed;row++) for (unsigned block=0;block<blocks;block++) {
		const uint8_t* data=raw+(size_t(row)*blocks+block)*34;
		uint16_t d=uint16_t(data[0])|(uint16_t(data[1])<<8);
		if ((d&0x7c00)==0x7c00) { passed=false; break; }
		for (unsigned i=0;i<32;i++) {
			int q=data[2+i]; if (q>=128) q-=256;
			uint8_t encoded=data[2+i];
			if (native && !PioneerInt8UnpackEncode(q,encoded)) { fprintf(stderr,"Native INT8 weight %d refused at row%u block%u i%u before BEGIN\n",q,row,block,i); passed=false; break; }
			uint64_t byte=f32Scales?PioneerQ8NativeF32ValueByte(inputs,block*32+i,row):compact?PioneerQ8CompactValueByte(inputs,block*32+i,row):PioneerMatvecWeightElement(inputs,block*32+i,row);
			packed[byte/4]|=uint32_t(compact?uint16_t(encoded):PioneerInt8Operand(q))<<(8*(byte&3));
		}
		uint64_t byte=f32Scales?PioneerQ8NativeF32ScaleByte(inputs,block,row):compact?PioneerQ8CompactScaleByte(inputs,block,row):(uint64_t(row/16)*(inputs/16)+block*2+1)*576+544+(row%16)*2;
		packed[byte/4]|=f32Scales?PioneerQ8HalfToFloatBits(d):uint32_t(d)<<(8*(byte&3));
	}
	printf("Real Qwen Q8_0 tensor [%u,%llu], %u rows, %u bytes retained; all %u blocks/row, integer dots + device scales + SFPU accumulation\n",
		inputs,(unsigned long long)tensor.outputs,outputs,bytes,blocks);
	printf("Weight format %s; NoC reads per matvec %u (math tiles %u)\n",native?"native sign-magnitude bytes, 1.125 bytes/parameter":compact?"compact signed bytes, 1.125 bytes/parameter":"expanded integer overlay, 2.25 bytes/parameter",(inputs/16)*(outputs/16)/(compact?2:1),(inputs/16)*(outputs/16));
	if (f32Scales) puts("Distinct exact FP32 weight scales; same original half values and footprint, no hot-loop half conversion; NOT cache precision");
	session={}; session.version=1; session.bytes=bytes;
	if (passed) {
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&session,sizeof(session));
		closeOnly=status!=0; passed=!status && session.active && session.state==TT_RESIDENT_SESSION_READY; cold=session.cold!=0;
	}
	for (unsigned offset=0;offset<bytes && passed;offset+=4096) {
		tt_pioneer_resident_page page={}; page.version=1; page.offset=offset; memcpy(page.data,packed+offset/4,4096);
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_STORE_UPLOAD,&page,sizeof(page)); closeOnly=status!=0;
		passed=!status && page.result.state==TT_STORE_PASSED && !page.result.recovery_required; cold=page.result.recovery_required!=0;
	}
	uint32_t previous[4096]={}; uint64_t sequence=0;
	for (unsigned trial=0;trial<2 && passed;trial++) {
		memset(request,0,sizeof(*request)); auto& m=request->matvec; m.version=1; m.inputs=inputs; m.outputs=outputs;
		for (unsigned k=0;k<inputs;k++) m.input[k/2]|=uint32_t(PioneerInt8Operand(Input(k,trial)))<<(16*(k&1));
		for (unsigned b=0;b<blocks;b++) request->scale[b]=Bits(Scale(b,trial));
		double start=Seconds(); int status=ioctl(fd,operation,request,sizeof(*request));
		const auto& r=m.result; cold=r.recovery_required!=0; closeOnly=status!=0;
		printf("Trial %u state %u start/complete/stop %u/%u/%u cold %u phase/PC %x/%x/%x tiles %u NoC %u/%u/%u/%u elapsed %.6fs\n",
			trial,r.state,r.started,r.completed,r.stopped,cold,r.phase,r.pc[0],r.pc[1],r.tiles,
			r.after[0]-r.before[0],r.after[1]-r.before[1],r.after[2]-r.before[2],r.after[3]-r.before[3],Seconds()-start);
		passed=!status && r.state==TT_RUNTIME_PASSED && r.started && r.completed && r.stopped && !cold;
		if (!passed) break;
		double error2=0,reference2=0; unsigned errors=0; float maxError=0;
		for (unsigned row=0;row<outputs;row++) {
			float reference=0,absolute=0;
			for (unsigned b=0;b<blocks;b++) {
				const uint8_t* data=raw+(size_t(row)*blocks+b)*34; int dot=0;
				for (unsigned i=0;i<32;i++) { int q=data[2+i]; if (q>=128) q-=256; dot+=q*Input(b*32+i,trial); }
				float d=PioneerHalfFloat(uint16_t(data[0])|(uint16_t(data[1])<<8));
				float product=float(dot)*(d*Scale(b,trial)); reference+=product; absolute+=fabsf(product);
			}
			float actual=PioneerTileFloat(m.output[row]),difference=actual-reference,error=fabsf(difference);
			if (!isfinite(actual) || error>1e-6f+absolute*1e-6f) errors++;
			if (error>maxError) maxError=error;
			error2+=double(difference)*difference; reference2+=double(reference)*reference;
			if (row<4) printf("  row%u card %.9g CPU-Q8-block %.9g error %.9g\n",row,actual,reference,error);
		}
		double relative=reference2?sqrt(error2/reference2):sqrt(error2);
		printf("Scaled Q8 reference failures %u/%u max-abs %.9g relative-L2 %.9g (required <1e-5)\n",errors,outputs,maxError,relative);
		passed=!errors && isfinite(relative) && relative<1e-5;
		if (passed) {
			tt_runtime_profile_result p={}; status=ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&p,sizeof(p)); closeOnly=status!=0;
			passed=!status && p.version==1 && p.available && p.clock_stable && p.format==(f32Scales?7U:native?6U:compact?5U:4U) && p.mask==0x17f
				&& p.sequence>sequence && p.state==TT_RUNTIME_PASSED && !p.recovery_required && p.started && p.completed && p.stopped;
			uint64_t last=0; for (unsigned i=0;i<TT_PROFILE_SLOTS;i++) if (p.mask&(1U<<i)) { passed &= p.microseconds[i]>=last; last=p.microseconds[i]; }
			sequence=p.sequence;
			if (passed) printf("CPU wall time (guards/polls INCLUDED) total %llu us, compute+proof %llu, PIO %llu; NOT device-only throughput\n",
				(unsigned long long)p.microseconds[8],(unsigned long long)(p.microseconds[4]-p.microseconds[3]),(unsigned long long)(p.microseconds[5]-p.microseconds[4]));
		}
		memcpy(previous,m.output,outputs*4);
	}
	if (passed) {
		auto& m=request->matvec; uint32_t saved=request->scale[0]; request->scale[0]=0x7fc00000;
		int status=ioctl(fd,operation,request,sizeof(*request)); closeOnly=status!=0; cold=m.result.recovery_required!=0;
		passed=!status && m.result.state==TT_RUNTIME_REFUSED && !m.result.started && !cold;
		printf("Nonfinite scale refused before start: %s\n",passed?"PASS":"FAIL"); request->scale[0]=saved;
		if (passed) {
			// Keep the wrong format's weight range valid, so this tests the
			// immutable prepared-code gate rather than an earlier range error.
			if (compact) m.outputs=outputs/2;
			status=compact?ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8,request,sizeof(*request))
				:ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_INT8,&m,sizeof(m)); closeOnly=status!=0; cold=m.result.recovery_required!=0;
			m.outputs=outputs;
			passed=!status && m.result.state==TT_RUNTIME_CONFIG_FAILED && !m.result.started && !cold;
			printf("Prepared-format switch refused: %s\n",passed?"PASS":"FAIL");
		}
		if (passed) {
			if (native) {
				status=ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_COMPACT,request,sizeof(*request)); closeOnly=status!=0; cold=m.result.recovery_required!=0;
				passed=!status && m.result.state==TT_RUNTIME_CONFIG_FAILED && !m.result.started && !cold;
				printf("Native-to-raw-byte format switch refused: %s\n",passed?"PASS":"FAIL");
			}
		}
		if (passed) {
			if (f32Scales) {
				status=ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE,request,sizeof(*request)); closeOnly=status!=0; cold=m.result.recovery_required!=0;
				passed=!status && m.result.state==TT_RUNTIME_CONFIG_FAILED && !m.result.started && !cold;
				printf("FP32-to-FP16 weight scale format switch refused: %s\n",passed?"PASS":"FAIL");
			}
		}
		if (passed) {
			status=ioctl(fd,operation,request,sizeof(*request)); closeOnly=status!=0; cold=m.result.recovery_required!=0;
			passed=!status && m.result.state==TT_RUNTIME_PASSED && m.result.started && m.result.completed && m.result.stopped && !cold && !memcmp(previous,m.output,outputs*4);
			printf("Bit-identical reuse after refusals: %s\n",passed?"PASS":"FAIL");
		}
	}
	if (cold || closeOnly) fprintf(stderr,"STOP: %s; no retries, reset, reload or further card commands\n",cold?"cold recovery required":"request failed / state uncertain");
	else if (session.active) {
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_END,&session,sizeof(session));
		printf("END state %u PCI/host %u/%u cold %u\n",session.state,session.pci_restored,session.host_released,session.cold);
		passed &= !status && session.state==TT_RESIDENT_SESSION_CLOSED && session.pci_restored && session.host_released && !session.cold;
	}
	free(raw); free(packed); free(request); close(fd);
	printf("%s: scaled real Q8_0 projection, retained weights; NOT full-model inference or a CPU speedup\n",passed?"PASS":"FAIL"); return passed?0:1;
}
