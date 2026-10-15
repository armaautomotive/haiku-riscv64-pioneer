/* SPDX-License-Identifier: MIT */
#include "TenstorrentHaikuTile.h"
#include "TenstorrentGgufQuant.h"
#include "TenstorrentGgufTensor.h"
#include "PioneerFloat16.h"
#include <stdlib.h>
#include <time.h>
static double Seconds() { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static bool Profile(int fd,bool fp16,bool dma,bool int8,uint64_t& previous,bool& closeOnly)
{
	tt_runtime_profile_result p={};
	if (ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&p,sizeof(p))) {
		perror("runtime phase metadata"); closeOnly=true; return false;
	}
	bool valid=p.version==1 && p.available && p.clock_stable && p.sequence>previous
		&& p.format==(int8?3U:fp16?2U:dma?1U:0U) && p.mask==(dma?0x1dfU:0x17fU)
		&& p.state==TT_RUNTIME_PASSED && p.started && p.completed && p.stopped && !p.recovery_required;
	uint64_t last=0;
	for (unsigned slot=0;slot<TT_PROFILE_SLOTS;slot++) if (p.mask&(1U<<slot)) {
		valid &= p.microseconds[slot]>=last; last=p.microseconds[slot];
	}
	if (!valid) { fprintf(stderr,"Invalid/stale phase metadata; no timing claim\n"); return false; }
	previous=p.sequence; const uint64_t* t=p.microseconds;
	printf("  CPU wall-time profile #%llu us (guards/polling INCLUDED): prepare %llu stage %llu configure %llu compute-proof %llu PIO %llu stop %llu DMA %llu finish %llu total %llu; NOT device-only throughput\n",
		(unsigned long long)p.sequence,(unsigned long long)t[1],(unsigned long long)(t[2]-t[1]),(unsigned long long)(t[3]-t[2]),
		(unsigned long long)(t[4]-t[3]),(unsigned long long)(dma?0:t[5]-t[4]),(unsigned long long)(t[6]-t[dma?4:5]),
		(unsigned long long)(dma?t[7]-t[6]:0),(unsigned long long)(t[8]-t[dma?7:6]),(unsigned long long)t[8]);
	return true;
}
int main(int argc,char** argv)
{
	bool support=argc==2 && !strcmp(argv[1],"--check-support");
	bool compareDma=argc>=3 && !strcmp(argv[1],"--compare-qwen-resident-output-dma");
	bool fp16=argc>=3 && !strcmp(argv[1],"--run-qwen-resident-fp16");
	bool int8=argc>=3 && !strcmp(argv[1],"--probe-qwen-q8-int8-engine");
	bool integerProbe=(argc>=3 && !strcmp(argv[1],"--probe-qwen-q8-integer-block")) || int8;
	unsigned outputs=32;
	if (argc==5) {
		char* end=NULL; unsigned long n=strtoul(argv[4],&end,10);
		if (!end || end==argv[4] || *end || n<16 || n>TT_RUNTIME_MATVEC_OUTPUTS_MAX || (n&15)) return 2;
		outputs=unsigned(n);
	}
	if (!support && ((argc!=3 && argc!=5) || (!compareDma && !fp16 && !integerProbe && strcmp(argv[1],"--run-qwen-resident-projection")))) {
		fprintf(stderr,"Usage: %s --check-support | {--run-qwen-resident-projection | --compare-qwen-resident-output-dma | --run-qwen-resident-fp16 | --probe-qwen-q8-integer-block | --probe-qwen-q8-int8-engine} MODEL_GGUF [TENSOR_NAME OUTPUT_ROWS]\nReal Q8_0 rows (default: 32 blk.0.attn_q), retained weights, two separate vectors; DMA comparison requires bit-identical PIO results; integer-block probes require exact dots, not a scaled Q8 implementation. NOT full-model inference or tokens/second.\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0);
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) { perror("open"); return 1; }
	tt_resident_session_result session={};
	if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&session,sizeof(session))) { perror("runtime support"); close(fd); return 1; }
	printf("Session support v%u capabilities %u; resident matvec bit16 required\n",session.version,session.capabilities);
	if (session.version!=1 || (session.capabilities&31)!=31) { close(fd); return 1; }
	if (compareDma && !(session.capabilities&64)) { fprintf(stderr,"Output DMA capability unavailable\n"); close(fd); return 1; }
	if (fp16 && !(session.capabilities&128)) { fprintf(stderr,"Fixed FP16 matvec capability unavailable\n"); close(fd); return 1; }
	if (int8 && !(session.capabilities&512)) { fprintf(stderr,"Fixed integer-8 matvec capability unavailable\n"); close(fd); return 1; }
	if (support) { close(fd); return 0; }
	bool profiling=(session.capabilities&256)!=0;
	FILE* model=fopen(argv[2],"rb"); if (!model) { close(fd); return 1; }
	PioneerGgufTensor tensor={};
	if (!PioneerFindGgufTensor(model,argc==5?argv[3]:"blk.0.attn_q.weight",tensor)
		|| tensor.inputs>TT_RUNTIME_MATVEC_INPUTS_MAX || tensor.outputs<outputs || !PioneerMatvecShape(unsigned(tensor.inputs),outputs)
		|| fseeko(model,off_t(tensor.offset),SEEK_SET)) { fprintf(stderr,"Need a bounded Q8_0 tensor with a supported runtime shape\n"); fclose(model); close(fd); return 1; }
	const unsigned inputs=unsigned(tensor.inputs);
	float* weights=(float*)malloc(inputs*outputs*sizeof(float));
	unsigned bytes=unsigned(PioneerMatvecWeightBytes(inputs,outputs));
	uint32_t* packed=(uint32_t*)calloc(bytes/4,sizeof(uint32_t));
	tt_pioneer_runtime_matvec* request=(tt_pioneer_runtime_matvec*)calloc(1,sizeof(tt_pioneer_runtime_matvec));
	if (!weights || !packed || !request) { free(weights); free(packed); free(request); fclose(model); close(fd); return 1; }
	bool passed=true,cold=false,closeOnly=false;
	for (unsigned row=0;row<outputs && passed;row++) for (unsigned block=0;block<inputs/32;block++) {
		uint8_t data[34];
		if (fread(data,1,34,model)!=34 || !PioneerDecodeQ8_0(data,weights+row*inputs+block*32)) { passed=false; break; }
		if (integerProbe) for (unsigned k=0;k<32;k++) {
			int q=data[2+k]; if (q>=128) q-=256;
			weights[row*inputs+block*32+k]=block==0?float(q):0;
		}
	}
	fclose(model);
	for (unsigned row=0;row<outputs && passed;row++) for (unsigned k=0;k<inputs;k++) {
		uint64_t byte=PioneerMatvecWeightElement(inputs,k,row);
		uint16_t value=int8?PioneerInt8Operand(int(weights[row*inputs+k])):fp16?PioneerFloat16(weights[row*inputs+k]):PioneerTileBf16(weights[row*inputs+k]);
		if (int8 && !PioneerInt8OperandValid(value)) { passed=false; break; }
		if (fp16 && (value&0x7c00)==0x7c00) { passed=false; break; }
		packed[byte/4]|=uint32_t(value)<<(8*(byte&3));
	}
	printf("Source tensor Q8_0 [%llu,%llu] file offset %llu; retained sample %ux%u packed %u bytes\n",
		(unsigned long long)tensor.inputs,(unsigned long long)tensor.outputs,(unsigned long long)tensor.offset,outputs,inputs,bytes);
	if (integerProbe) printf("INTEGER BLOCK PROBE: first 32 raw Q8 integers per real row, zero padding, %s, exact dot gate. Scales are NOT applied; not a quantized inference backend.\n",int8?"integer-8 encoded operands":"exactly represented BF16 operands");
	if (int8) puts("INTEGER ENGINE: forced integer-8 matrix arithmetic with int32 destination/readback; FP16 unpack is ONLY an operand bit overlay, not FP16 arithmetic.");
	session={}; session.version=1; session.bytes=bytes;
	if (passed) passed=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&session,sizeof(session))==0 && session.active && session.state==TT_RESIDENT_SESSION_READY;
	for (unsigned offset=0;offset<bytes && passed;offset+=4096) {
		tt_pioneer_resident_page page={}; page.version=1; page.offset=offset; memcpy(page.data,packed+offset/4,4096);
		passed=ioctl(fd,TT_PCI_PROBE_PIONEER_STORE_UPLOAD,&page,sizeof(page))==0 && page.result.state==TT_STORE_PASSED && !page.result.recovery_required;
		cold=page.result.recovery_required!=0;
	}
	if (passed) printf("Weights uploaded once: %u pages; running two independent vectors\n",bytes/4096);
	uint32_t pioReference[TT_RUNTIME_MATVEC_OUTPUTS_MAX]={};
	uint64_t previousProfile=0;
	for (unsigned trial=0;trial<(compareDma?4U:2U) && passed;trial++) {
		bool dma=compareDma && (trial&1);
		unsigned vectorTrial=compareDma?trial/2:trial;
		memset(request,0,sizeof(*request)); request->version=1; request->inputs=inputs; request->outputs=outputs;
		float vector[TT_RUNTIME_MATVEC_INPUTS_MAX];
		for (unsigned k=0;k<inputs;k++) {
			vector[k]=float(int((k*17+vectorTrial*29)%257)-128)/256.0f;
			if (integerProbe) vector[k]=k<32?float(int((k*17+vectorTrial*29)%255)-127):0;
			request->input[k/2]|=uint32_t(int8?PioneerInt8Operand(int(vector[k])):fp16?PioneerFloat16(vector[k]):PioneerTileBf16(vector[k]))<<(16*(k&1));
		}
		double start=Seconds();
		int status=ioctl(fd,int8?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_INT8:fp16?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_FP16:dma?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_DMA:TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC,request,sizeof(*request));
		double elapsed=Seconds()-start;
		const auto& r=request->result; cold=r.recovery_required!=0;
		printf("Trial %u state %u start/complete/stop %u/%u/%u staged %u polls %u phase/tiles/marker %x/%u/%x PC %x/%x cold %u %.6fs\n",
			trial,r.state,r.started,r.completed,r.stopped,r.staged_pages,r.polls,r.phase,r.tiles,r.marker,r.pc[0],r.pc[1],r.recovery_required,elapsed);
		printf("  device weight reads response/flits/accepted/sent %u/%u/%u/%u\n",uint32_t(r.after[0]-r.before[0]),uint32_t(r.after[1]-r.before[1]),uint32_t(r.after[2]-r.before[2]),uint32_t(r.after[3]-r.before[3]));
		if (status || r.state!=TT_RUNTIME_PASSED || !r.started || !r.completed || !r.stopped || cold) { if (status) perror("runtime matvec"); passed=false; break; }
		if (profiling && !Profile(fd,fp16,dma,int8,previousProfile,closeOnly)) { passed=false; break; }
		if (compareDma) {
			if (!dma) memcpy(pioReference,request->output,outputs*4);
			else {
				unsigned different=0; for (unsigned row=0;row<outputs;row++) different+=pioReference[row]!=request->output[row];
				printf("  DMA vs independent PIO execution: %u bit mismatches of %u outputs\n",different,outputs);
				if (different) { passed=false; break; }
			}
			printf("  result path %s, independent vector %u\n",dma?"owned output DMA":"legacy PIO",vectorTrial);
		}
		double dot=0,a2=0,ref2=0,delta2=0; unsigned mismatches=0,strictMismatches=0,integerMismatches=0;
		float maxError=0,maxNormalized=0;
		for (unsigned row=0;row<outputs;row++) {
			float reference=0,bf16=0,absolute=0;
			for (unsigned k=0;k<inputs;k++) {
				float w=weights[row*inputs+k],x=vector[k]; reference+=w*x;
				// All four documented FP16 fidelity phases consume nine
				// explicit SrcA bits and all ten SrcB mantissa bits.
				float product=fp16?PioneerFloat16Value(PioneerFloat16(w)&~1U)*PioneerFloat16Value(PioneerFloat16(x))
					:PioneerTileFloat(uint32_t(PioneerTileBf16(w))<<16)*PioneerTileFloat(uint32_t(PioneerTileBf16(x))<<16);
				bf16+=product; absolute+=fabsf(product);
			}
			int32_t integerActual=0; memcpy(&integerActual,request->output+row,4);
			float actual=int8?float(integerActual):PioneerTileFloat(request->output[row]);
			integerMismatches+=integerProbe && actual!=reference;
			float error=fabsf(actual-bf16),normalized=absolute>0?error/absolute:error;
			bool strict=!isfinite(actual) || error>1e-5f+absolute*1e-5f;
			strictMismatches+=strict;
			// Match the existing data-tile/real-Qwen acceptance target,
			// not a new bound fitted to this trial. MVMUL's mixed-exponent
			// reduction is not IEEE exact. Keep reporting the stricter
			// diagnostic and also check full-vector Q8 FP32 agreement.
			mismatches+=!isfinite(actual) || error>1e-6f+absolute/1024.0f;
			if (error>maxError) maxError=error;
			if (normalized>maxNormalized) maxNormalized=normalized;
			dot+=double(actual)*reference; a2+=double(actual)*actual; ref2+=double(reference)*reference;
			double difference=actual-reference; delta2+=difference*difference;
			if (row<4 || (strict && strictMismatches<=8)) printf("  row%u card %.9g CPU-%s %.9g CPU-Q8%s %.9g error/sum-abs %.9g\n",row,actual,int8?"INT32":fp16?"FP16-phases":"BF16",bf16,integerProbe?"-integer-block":"",reference,normalized);
		}
		double cosine=dot/sqrt(a2*ref2),relative=sqrt(delta2/ref2);
		printf("  %s-level target failures %u; stricter 1e-5 diagnostic failures %u; max absolute/normalized error %.9g/%.9g\n",fp16?"FP16":"BF16",mismatches,strictMismatches,maxError,maxNormalized);
		printf("  independent Q8%s FP32 reference cosine %.9f relative-L2 %.9f; %s\n",integerProbe?"-integer-block":"",cosine,relative,integerProbe?"exact dot gate required":"NOT bit-exact");
		if (integerProbe) printf("  Exact integer-block mismatches %u of %u\n",integerMismatches,outputs);
		passed=!integerMismatches && !mismatches && isfinite(cosine) && cosine>.9999 && relative<.01;
		if (passed && int8) memcpy(pioReference,request->output,outputs*4);
		if (passed) passed=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_STATUS,&session,sizeof(session))==0 && session.uploaded_pages==bytes/4096;
	}
	if (passed && int8) {
		uint32_t saved=request->input[0]; request->input[0]=0x40004080; // +128 is outside signed-byte input range
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_INT8,request,sizeof(*request));
		cold=request->result.recovery_required!=0;
		if (status) { perror("integer invalid-input refusal"); closeOnly=true; }
		passed=!status && request->result.state==TT_RUNTIME_REFUSED && !request->result.started && !cold;
		printf("  Native integer encoding refusal: state %u started %u cold %u\n",request->result.state,request->result.started,cold);
		request->input[0]=saved;
		if (passed) {
			status=ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC,request,sizeof(*request));
			cold=request->result.recovery_required!=0;
			if (status) { perror("integer prepared-format refusal"); closeOnly=true; }
			passed=!status && request->result.state==TT_RUNTIME_CONFIG_FAILED && !request->result.started && !cold;
			printf("  Native prepared-format refusal: state %u started %u cold %u\n",request->result.state,request->result.started,cold);
		}
		if (passed) {
			status=ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_INT8,request,sizeof(*request));
			const auto& r=request->result; cold=r.recovery_required!=0;
			if (status) { perror("integer reuse after refusal"); closeOnly=true; }
			passed=!status && r.state==TT_RUNTIME_PASSED && r.started && r.completed && r.stopped && !cold
				&& !memcmp(pioReference,request->output,outputs*4);
			if (passed && profiling) passed=Profile(fd,false,false,true,previousProfile,closeOnly);
			printf("  Exact integer reuse after both refusals: %s\n",passed?"PASS":"FAIL");
		}
	}
	if (cold) fprintf(stderr,"STOP: cold recovery required, no retry/reset/reload\n");
	else if (closeOnly) fprintf(stderr,"STOP: metadata request failed; close-only, no further card commands or timing claim\n");
	else if (session.active) {
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_END,&session,sizeof(session))) passed=false;
		printf("Close state %u PCI/host %u/%u cold %u\n",session.state,session.pci_restored,session.host_released,session.cold);
		passed &= session.state==TT_RESIDENT_SESSION_CLOSED && session.pci_restored && session.host_released && !session.cold;
	}
	free(weights); free(packed); free(request); close(fd);
	printf("%s: %s, retained weights and two vectors; NOT full inference or a CPU speedup\n",passed?"PASS":"FAIL",integerProbe?"real Qwen integer-block probe, no scales applied":"real Qwen projection");
	return passed?0:1;
}
