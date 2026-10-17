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
	bool active=false,failed=false,cold=false,finished=false,stream=false,fp16=false,q8=false,compact=false,native=false,f32Scales=false,closeOnly=false;
	tt_pioneer_runtime_matvec* request=NULL;
	tt_pioneer_attention_tile_test* attentionRequest=NULL;
	bool dual=false,reuse=false;
	bool attention=false;
	bool bulkUpload=false;
	unsigned fleet=0;
	tt_pioneer_runtime_fleet* fleetRequest=NULL;
	tt_pioneer_fleet_attention* fleetAttention=NULL;
	tt_pioneer_resident_attention* residentAttention=NULL;
	bool residentKV=false;
	bool payloadBlocks=false;
	unsigned coordinatorNonce=0;
	unsigned coordinatedVersion=0;
	bool coordinated=false;
	bool coordinatedDebug=false;
	bool coordinatedWide=false;
	bool coordinatedSpan=false;
	bool coordinatedNarrow=false;
	bool decoderRequested=false,decoderRegistered=false,decoderParallel=false,decoderStageRequested=false,decoderGlu=false,decoderGreedy=false,decoderRms=false,decoderCounters=false,decoderPaired=false,decoderInline=false,decoderPartial=false,decoderCachedQuery=false,decoderInputPull=false,decoderPhasePipeline=false;
	unsigned decoderTokens=0;
	tt_pioneer_decoder_run* decoderRequest=NULL;
	tt_pioneer_coordinated_fleet* coordinatedRequest=NULL;
	size_t requestBytes=0;
public:
	uint64_t uploads=0,calls=0,attentionCalls=0;
	unsigned peakWorkers=0;
	uint64_t activationReuses=0;
	uint64_t wideCalls=0,spanCalls=0,narrowCalls=0,greedyCalls=0;
	unsigned UploadPageLimit() const { return decoderRegistered?0:coordinatedSpan?256:4; }
	bool NarrowEnabled() const { return coordinatedNarrow; }
	bool DecoderProfile(tt_runtime_profile_result& result) {
		if (!decoderRegistered || !active || failed || finished || cold) return false;
		return !ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&result,sizeof(result))
			&& result.version==1 && result.available && result.clock_stable && result.format==21 && result.mask==0x13f
			&& result.state==TT_RUNTIME_PASSED && result.started && result.completed && result.stopped && !result.recovery_required;
	}
	unsigned lastActivationPages=0;
	bool DecoderStages(tt_decoder_stage_profile& p) {
		return decoderStageRequested && decoderRegistered && decoderTokens && active && !failed && !finished && !cold
			&& !ioctl(fd,TT_PCI_PROBE_PIONEER_DECODER_STAGES,&p,sizeof(p)) && PioneerDecoderStageValid(p,coordinatorNonce,decoderTokens,decoderPaired?3:decoderGlu?2:1);
	}
	// Diagnostic cohort tuning only; no new driver authority or active
	// job mutation. The kernel independently partitions every request.
	bool CoordinatedLimit(unsigned workers) {
		if (!coordinated || !active || failed || finished || cold || !workers || workers>119) return false;
		fleet=workers; return true;
	}
	bool CoordinatedProfile(tt_runtime_profile_result& result,bool wide=false) {
		if (!coordinated || !active || failed || finished || cold) return false;
		return !ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE,&result,sizeof(result))
			&& result.version==1 && result.available && result.clock_stable
			&& result.format==(wide?18U:coordinatedNarrow?20U:15U) && result.mask==0x13f && result.state==TT_RUNTIME_PASSED
			&& result.started && result.completed && result.stopped && !result.recovery_required;
	}
	~PioneerResidentDevice() { Finish(); free(request); free(attentionRequest); free(fleetRequest); free(fleetAttention); free(residentAttention); free(coordinatedRequest); free(decoderRequest); }
	bool Open(uint64_t bytes,bool half=false,bool quant=false,bool rawBytes=false,bool hardwareUnpack=false,bool exactScales=false,bool twoCores=false,bool reuseInput=false,bool deviceAttention=false,unsigned groupedUpload=0,unsigned workerLimit=0,bool deviceKV=false,bool blockPayload=false,bool coordinatorMode=false,bool debugOutput=false,unsigned requireLayer=0) {
		if (requireLayer>16 || (requireLayer && (!coordinatorMode || debugOutput)) || (requireLayer>=4 && workerLimit!=119)) return false;
		if (debugOutput && !coordinatorMode) return false;
		if (coordinatorMode && (!deviceKV || !blockPayload || !workerLimit || workerLimit>119 || bytes!=UINT64_C(0x80000000))) return false;
		if (blockPayload && !deviceKV) return false;
		if (deviceKV && (!workerLimit || !deviceAttention || !PioneerResidentAttentionCapacity(bytes))) return false;
		if (fd>=0 || failed || finished || (half && quant) || (rawBytes && !quant) || (hardwareUnpack && !rawBytes) || (exactScales && !hardwareUnpack) || (twoCores && !exactScales) || (reuseInput && !twoCores) || (deviceAttention && !reuseInput) || groupedUpload>2 || (groupedUpload && !reuseInput) || workerLimit>120 || (workerLimit && (!reuseInput || !groupedUpload)) || !PioneerResidentCapacity(bytes)) return false;
		fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
		if (fd<0) { perror("resident open"); failed=true; return false; }
		tt_resident_session_result s={};
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&s,sizeof(s)) || s.version!=1 || (s.capabilities&31)!=31) {
			fprintf(stderr,"Resident matvec capability unavailable; no compute submitted\n"); failed=true; return false;
		}
		stream=(s.capabilities&32)!=0;
		fp16=half;
		q8=quant;
		compact=rawBytes;
		native=hardwareUnpack;
		f32Scales=exactScales;
		dual=twoCores;
		reuse=reuseInput;
		attention=deviceAttention;
		fleet=workerLimit;
		residentKV=deviceKV;
		payloadBlocks=blockPayload;
		coordinated=coordinatorMode;
		coordinatedDebug=debugOutput;
		coordinatedWide=requireLayer>=3;
		decoderRequested=requireLayer>=4; decoderParallel=requireLayer>=5; decoderStageRequested=requireLayer>=6; decoderGlu=requireLayer>=7; decoderGreedy=requireLayer>=8; decoderRms=requireLayer>=9; decoderCounters=requireLayer==10; decoderPaired=requireLayer>=11; decoderInline=requireLayer>=12; decoderPartial=requireLayer==13; decoderCachedQuery=requireLayer>=14; decoderInputPull=requireLayer==15; decoderPhasePipeline=requireLayer==16;
		if (coordinated && !(s.capabilities&TT_PIONEER_CAP_COORDINATED_FLEET)) {
			fprintf(stderr,"Explicit on-card coordinator capability unavailable; no BEGIN/fallback\n"); failed=true; return false;
		}
		if (coordinated) {
			tt_coordinated_support support={};
			if (ioctl(fd,TT_PCI_PROBE_PIONEER_COORDINATED_SUPPORT,&support,sizeof(support)) || support.version<7 || support.version>25
				|| (requireLayer && support.version<8) || (requireLayer>=2 && support.version<9) || (requireLayer>=3 && support.version<10)
				|| (requireLayer==4 && support.version!=13)
				|| (requireLayer==5 && support.version!=14)
				|| (requireLayer==6 && support.version!=15)
				|| (requireLayer==7 && support.version!=16)
				|| (requireLayer==8 && support.version!=17)
				|| (requireLayer==9 && support.version!=18)
				|| (requireLayer==10 && support.version!=19)
				|| (requireLayer==11 && support.version!=20)
				|| (requireLayer==12 && support.version!=21)
				|| (requireLayer==13 && support.version!=22)
				|| (requireLayer==14 && support.version!=23)
				|| (requireLayer==15 && support.version!=24)
				|| (requireLayer==16 && support.version!=25)
				|| support.max_workers!=119 || support.attention_version!=3 || support.immutable_weights!=1 || support.gather_max!=8192) {
				fprintf(stderr,"Required coordinated-matrix/KV-v3 contract unavailable; no BEGIN/fallback\n"); failed=true; return false;
			}
			coordinatedVersion=support.version;
			coordinatedSpan=coordinatedWide && support.version>=11;
			coordinatedNarrow=coordinatedWide && support.version==12;
		}
		if (payloadBlocks && (s.capabilities&(TT_PIONEER_CAP_PAYLOAD_BLOCKS|TT_PIONEER_CAP_WIDE_PAYLOAD))
			!=(TT_PIONEER_CAP_PAYLOAD_BLOCKS|TT_PIONEER_CAP_WIDE_PAYLOAD)) {
			fprintf(stderr,"Explicit bounded payload block capability unavailable; no BEGIN/fallback\n"); failed=true; return false;
		}
		if (residentKV && !(s.capabilities&TT_PIONEER_CAP_RESIDENT_ATTENTION)) {
			fprintf(stderr,"Explicit device-resident KV attention unavailable; no BEGIN/fallback\n"); failed=true; return false;
		}
		if (fleet && (!(s.capabilities&TT_PIONEER_CAP_Q8_FLEET) || (attention && !(s.capabilities&TT_PIONEER_CAP_FLEET_ATTENTION)))) {
			fprintf(stderr,"Explicit multicore/whole-attention capability unavailable; no BEGIN/compute/fallback\n"); failed=true; return false;
		}
		bulkUpload=groupedUpload && (s.capabilities&TT_PIONEER_CAP_BULK_UPLOAD);
		if (residentKV && !bulkUpload) { fprintf(stderr,"Resident KV requires owned bulk scratch capability; no BEGIN/fallback\n"); failed=true; return false; }
		if (groupedUpload==1 && !bulkUpload) { fprintf(stderr,"Explicit owned bulk upload unavailable; no BEGIN/compute submitted\n"); failed=true; return false; }
		if (fp16 && !(s.capabilities&128)) { fprintf(stderr,"Fixed FP16 matvec unavailable; no compute submitted\n"); failed=true; return false; }
		if (q8 && !(s.capabilities&1024)) { fprintf(stderr,"Scaled Q8 matvec unavailable; no compute submitted\n"); failed=true; return false; }
		if (compact && !(s.capabilities&2048)) { fprintf(stderr,"Compact Q8 matvec unavailable; no compute submitted\n"); failed=true; return false; }
		if (native && !(s.capabilities&TT_PIONEER_CAP_NATIVE_Q8_UNPACK)) { fprintf(stderr,"Aligned native INT8-unpack Q8 matvec unavailable; no compute submitted\n"); failed=true; return false; }
		if (f32Scales && !(s.capabilities&TT_PIONEER_CAP_NATIVE_Q8_F32_SCALES)) { fprintf(stderr,"Exact-scale/compensated-SFPU Q8 matvec unavailable; no compute submitted\n"); failed=true; return false; }
		if (dual && !(s.capabilities&TT_PIONEER_CAP_Q8_DUAL)) { fprintf(stderr,"Fixed two-core bulk Q8 unavailable; no BEGIN/compute submitted\n"); failed=true; return false; }
		if (reuse && !(s.capabilities&TT_PIONEER_CAP_Q8_DUAL_REUSE)) { fprintf(stderr,"Guarded exact activation reuse unavailable; no BEGIN/compute submitted\n"); failed=true; return false; }
		if (attention && !(s.capabilities&TT_PIONEER_CAP_ATTENTION_TILE_SHARED)) { fprintf(stderr,"Shared attention capability unavailable; no BEGIN/compute submitted\n"); failed=true; return false; }
		fprintf(stderr,"Resident upload path: %s\n",stream?"verified six-step stream":"legacy PIO-checked");
		if (bulkUpload) fprintf(stderr,"Explicit owned bulk upload: synchronous 1..4 pages, six exact proofs and full CPU comparisons\n");
		if (coordinatedSpan) fprintf(stderr,"Explicit contract11 NEW-weight upload:1..256 pages,card-side full GDDR/echo proof and frozen-CPU transport comparison\n");
		if (coordinatedNarrow) fprintf(stderr,"Explicit contract12 narrow matrices:published worker source/idle/math proof,one owned output write/read/all-word compare; strict diagnostic path unchanged\n");
		if (decoderRequested) fprintf(stderr,"EXPERIMENTAL contract13 fixed whole-position decoder requested;must register all immutable weights/norms before running;NOT a qualified speedup\n");
		fprintf(stderr,"Resident operand format: %s\n",q8?"original Q8_0 signed bytes/scales; integer dots + SFPU accumulation":fp16?"FP16, four fidelity phases, explicit subnormal flush":"BF16");
		if (q8) fprintf(stderr,"Resident Q8 weight layout: %s\n",native?"native signed INT8 unpack, 1.125 bytes/parameter; refuse -128 weights":compact?"compact signed bytes, 1.125 bytes/parameter":"expanded integer overlays, 2.25 bytes/parameter");
		if (f32Scales) fprintf(stderr,"Resident native scale mode: exact FP32 scale bits, compensated SFPU product + round-to-odd; no CPU learned sums\n");
		if (dual && !coordinated) fprintf(stderr,"Two fixed bulk workers: disjoint rows, both complete/stop proofs required; NOT a CPU speedup qualification\n");
		if (reuse && !coordinated) fprintf(stderr,"Explicit per-core exact-byte activation reuse; output poison/full completion proofs retained\n");
		if (coordinated) fprintf(stderr,"Explicit on-card dispatcher + up to%u persistent math peers; one shared activation upload, on-card completion/source/counter proof/gather; no CPU reference validation implied\n",fleet);
		if (coordinated) fprintf(stderr,"Coordinated result policy: %s; no CPU learned sums\n",coordinatedDebug?"debug separate-launch exact ordered readback":coordinatedWide?"fused narrow output plus explicit private1MiB wide DMA/all-word verification":"fast fused owned-host output for <=4096 rows; larger outputs retain PIO");
		else if (fleet) fprintf(stderr,"Throughput candidate: up to%u row workers, one shared activation upload/whole projection, on-card input distribution/output poison; no CPU reference validation implied\n",fleet);
		requestBytes=dual?sizeof(tt_pioneer_runtime_dual_matvec):sizeof(tt_pioneer_runtime_q8_matvec);
		request=(tt_pioneer_runtime_matvec*)calloc(1,requestBytes);
		if (!request) { failed=true; return false; }
		s={}; s.version=1; s.bytes=bytes;
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&s,sizeof(s)); active=s.active!=0; cold=s.cold!=0;
		closeOnly=status!=0;
		if (status || !active || s.state!=TT_RESIDENT_SESSION_READY || cold) {
			fprintf(stderr,"Resident BEGIN failed: state %u active %u cold %u\n",s.state,s.active,s.cold); failed=true; return false;
		}
		return true;
	}
	bool Upload(uint64_t offset,const uint32_t* data) {
		if (!active || failed || finished || cold || decoderRegistered || !data) return false;
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
			closeOnly=status!=0;
			fprintf(stderr,"Resident upload STOP: status %d state %u offset %llu cold %u\n",status,state,(unsigned long long)offset,cold);
			failed=true; return false;
		}
		uploads++;
		if (!(uploads%4096)) fprintf(stderr,"Resident upload verified: %llu MiB\n",(unsigned long long)(uploads/256));
		return true;
	}
	bool UploadPages(uint64_t offset,const uint32_t* data,unsigned pages) {
		if (!active || failed || finished || cold || !data || !pages || pages>UploadPageLimit()) return false;
		if (coordinatedSpan) {
			unsigned bytes=pages*4096; size_t size=PioneerSpanRequestBytes(bytes);
			auto* p=static_cast<tt_pioneer_layer_span*>(calloc(1,size)); if (!p) { failed=true; return false; }
			p->version=1; p->bytes=bytes; p->offset=offset; memcpy(p->data,data,bytes);
			int status=ioctl(fd,TT_PCI_PROBE_PIONEER_LAYER_SPAN,p,size); cold=p->result.master.recovery_required!=0;
			bool proved=!status && PioneerSpanProof(p->result,bytes,coordinatorNonce+1);
			if (!proved) {
				const auto& m=p->result.master;
				fprintf(stderr,"Span upload STOP status%d state%u bytes%u nonce%u s/c/s%u/%u/%u PC%x marker%x cold%u; close only,no fallback/retry\n",
					status,m.state,bytes,m.nonce,m.started,m.completed,m.stopped,m.pc[1],m.marker,cold);
				failed=true; closeOnly=true; free(p); return false;
			}
			coordinatorNonce=p->result.master.nonce; uploads+=pages; spanCalls++; free(p);
			if (uploads%4096<pages) fprintf(stderr,"Resident span upload verified: %llu MiB\n",(unsigned long long)(uploads/256));
			return true;
		}
		if (!bulkUpload) {
			for (unsigned page=0;page<pages;page++) if (!Upload(offset+page*4096,data+page*1024)) return false;
			return true;
		}
		tt_pioneer_bulk_upload p={}; p.version=1; p.pages=pages; p.offset=offset; memcpy(p.data,data,pages*4096);
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_BULK_UPLOAD,&p,sizeof(p));
		const auto& r=p.result.transfer; cold=r.recovery_required!=0;
		bool proved=!status && r.version==1 && r.state==TT_STORE_PASSED && r.requests==6 && r.completed==6
			&& r.wrote_device && r.host_write_issued && r.host_visible && r.readback_complete && !r.mismatches && !cold
			&& p.result.pages==pages && p.result.host_words==pages*1024 && p.result.host_ready && p.result.l1_prepared;
		for (unsigned step=0;step<6;step++) for (unsigned w=0;w<4;w++)
			proved &= uint32_t(r.after[step][w]-r.before[step][w])==(w==1?64*pages:1);
		if (!proved) {
			closeOnly=true; failed=true;
			fprintf(stderr,"Resident bulk upload STOP status %d state %u offset %llu pages %u cold %u; close only, no fallback/retry\n",status,r.state,(unsigned long long)offset,pages,cold); return false;
		}
		uploads+=pages;
		if (uploads%4096<pages) fprintf(stderr,"Resident bulk upload verified: %llu MiB\n",(unsigned long long)(uploads/256));
		return true;
	}
	bool Matvec(uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* vector,uint32_t* result) {
		if (!active || failed || finished || cold || q8 || !vector || !result || !PioneerMatvecShape(inputs,outputs)) return false;
		memset(request,0,sizeof(*request)); request->version=1; request->inputs=inputs; request->outputs=outputs; request->weight_offset=offset;
		memcpy(request->input,vector,inputs*2);
		int status=ioctl(fd,fp16?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_FP16:TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC,request,sizeof(*request));
		const auto& r=request->result; cold=r.recovery_required!=0;
		if (status || r.version!=1 || r.state!=TT_RUNTIME_PASSED || !r.started || !r.completed || !r.stopped || cold) {
			closeOnly=status!=0;
			fprintf(stderr,"Resident matvec STOP: status %d state %u start/complete/stop %u/%u/%u phase/tiles/PC %x/%u/%x cold %u; no fallback/retry\n",
				status,r.state,r.started,r.completed,r.stopped,r.phase,r.tiles,r.pc[1],cold); failed=true; return false;
		}
		memcpy(result,request->output,outputs*4); calls++; return true;
	}
	bool HasLayerFfn() const { return coordinated && !coordinatedDebug && coordinatedVersion>=8 && coordinatedVersion<=25 && !decoderRegistered && active && !failed && !cold && !finished; }
	bool RegisterDecoder(const PioneerDecoderModelOffsets& offsets) {
		if (!decoderRequested || coordinatedVersion!=(decoderPhasePipeline?25U:decoderInputPull?24U:decoderCachedQuery?23U:decoderPartial?22U:decoderInline?21U:decoderPaired?20U:decoderCounters?19U:decoderRms?18U:decoderGreedy?17U:decoderGlu?16U:decoderStageRequested?15U:decoderParallel?14U:13U) || !active || failed || finished || cold || decoderRegistered) return false;
		tt_pioneer_decoder_model p={}; p.version=decoderPhasePipeline?10:decoderInputPull?9:decoderCachedQuery?8:decoderPartial?7:decoderInline?6:decoderPaired?5:decoderGreedy?4:decoderGlu?3:decoderParallel?2:1; p.offsets=offsets;
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_DECODER_MODEL,&p,sizeof(p)); cold=p.recovery_required!=0;
		if (status || p.state!=TT_RUNTIME_PASSED || p.ready!=1 || p.extents!=310 || cold) {
			fprintf(stderr,"Whole-decoder register STOP status%d state%u ready%u extents%u cold%u;close only,no retry/fallback\n",status,p.state,p.ready,p.extents,cold);
			failed=closeOnly=true; return false;
		}
		decoderRegistered=true; return true;
	}
	bool DecodeWhole(unsigned tokens,const uint32_t* embedding,const uint32_t* rope,uint32_t* output) {
		if (!decoderRequested || !decoderRegistered || !active || failed || finished || cold || !tokens || tokens>256 || !embedding || !rope || !output) return false;
		if (!decoderRequest) decoderRequest=static_cast<tt_pioneer_decoder_run*>(malloc(sizeof(tt_pioneer_decoder_run)));
		if (!decoderRequest) { failed=true; return false; }
		auto& p=*decoderRequest; memset(&p,0,offsetof(tt_pioneer_decoder_run,output)); p.version=1; p.tokens=tokens;
		memcpy(p.embedding,embedding,sizeof(p.embedding)); memcpy(p.rope,rope,sizeof(p.rope));
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_DECODER_RUN,&p,sizeof(p)); cold=p.result.master.recovery_required!=0;
		bool proved=!status && PioneerDecoderProof(p.result,tokens,coordinatorNonce+1,decoderParallel,decoderGlu,false,decoderPaired,decoderInputPull);
		for (unsigned i=0;i<151936;i++) proved &= (p.output[i]&0x7f800000)!=0x7f800000;
		if (!proved) { const auto& m=p.result.master;
			fprintf(stderr,"Whole-decoder STOP status%d state%u nonce%u s/c/s%u/%u/%u seq%u words%u PC%x marker%x cold%u;close only,no retry/fallback\n",
				status,m.state,m.nonce,m.started,m.completed,m.stopped,p.result.sequences,p.result.words,m.pc[1],m.marker,cold);
			failed=closeOnly=true; return false;
		}
		coordinatorNonce=p.result.master.nonce; decoderTokens=tokens; memcpy(output,p.output,sizeof(p.output)); calls+=197; attentionCalls+=28; wideCalls++;
		return true;
	}
	bool DecodeGreedy(unsigned tokens,const uint32_t* embedding,const uint32_t* rope,uint32_t& token,uint32_t& logit) {
		if (!decoderGreedy || !decoderRegistered || !active || failed || finished || cold || !tokens || tokens>256 || !embedding || !rope) return false;
		tt_pioneer_decoder_greedy p={}; p.version=1; p.tokens=tokens;
		memcpy(p.embedding,embedding,sizeof(p.embedding)); memcpy(p.rope,rope,sizeof(p.rope));
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_DECODER_GREEDY,&p,sizeof(p)); cold=p.result.master.recovery_required!=0;
		bool proved=!status && PioneerDecoderProof(p.result,tokens,coordinatorNonce+1,true,true,true,decoderPaired,decoderInputPull)
			&& PioneerDecoderGreedyValid(p.output,coordinatorNonce+1,tokens);
		if (!proved) { const auto& m=p.result.master;
			fprintf(stderr,"Greedy-decoder STOP status%d state%u nonce%u s/c/s%u/%u/%u seq%u words%u PC%x marker%x cold%u;close only,no retry/fallback\n",
				status,m.state,m.nonce,m.started,m.completed,m.stopped,p.result.sequences,p.result.words,m.pc[1],m.marker,cold);
			failed=closeOnly=true; return false;
		}
		coordinatorNonce=p.result.master.nonce; decoderTokens=tokens; token=p.output[3]; logit=p.output[4]; calls+=197; attentionCalls+=28; greedyCalls++;
		return true;
	}
	bool LayerGlu(unsigned n,const uint32_t* gate,const uint32_t* up,uint32_t* output) {
		if (!HasLayerFfn() || coordinatedVersion<9 || !n || n>3072 || (n&31) || !gate || !up || !output) return false;
		tt_pioneer_layer_glu p={}; p.version=1; p.outputs=n; memcpy(p.gate,gate,n*4); memcpy(p.up,up,n*4);
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_LAYER_GLU,&p,sizeof(p)); cold=p.result.master.recovery_required!=0;
		bool proved=!status && PioneerLayerGluProof(p.result,n,coordinatorNonce+1);
		for (unsigned i=0;i<n;i++) proved &= (p.output[i]&0x7f800000)!=0x7f800000;
		if (!proved) {
			const auto& m=p.result.master;
			fprintf(stderr,"Fixed GPU GLU STOP status%d state%u nonce%u start/complete/stop%u/%u/%u PC%x marker%x cold%u; no fallback/retry\n",
				status,m.state,m.nonce,m.started,m.completed,m.stopped,m.pc[1],m.marker,cold);
			failed=true; closeOnly=true; return false;
		}
		coordinatorNonce=p.result.master.nonce; memcpy(output,p.output,n*4); return true;
	}
	bool LayerFfn(uint64_t gate,uint64_t up,uint64_t down,const uint32_t* vector,const uint32_t* scales,uint32_t* output) {
		if (!HasLayerFfn() || !vector || !scales || !output) return false;
		tt_pioneer_layer_ffn p={}; p.version=1; p.workers=fleet; p.gate_offset=gate; p.up_offset=up; p.down_offset=down;
		memcpy(p.input,vector,sizeof(p.input)); memcpy(p.scale,scales,sizeof(p.scale));
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_LAYER_FFN,&p,sizeof(p)); cold=p.result.master.recovery_required!=0;
		bool proved=!status && PioneerCoordinatedLayerProof(p.result,fleet,coordinatorNonce+1);
		for (unsigned i=0;i<1024;i++) proved &= (p.output[i]&0x7f800000)!=0x7f800000;
		if (!proved) {
			const auto& m=p.result.master;
			fprintf(stderr,"Fixed FFN STOP status%d state%u nonce%u start/complete/stop%u/%u/%u PC%x marker%x cold%u; no fallback/retry\n",
				status,m.state,m.nonce,m.started,m.completed,m.stopped,m.pc[1],m.marker,cold);
			fprintf(stderr," FFN R/flits/W/flits%u/%u/%u/%u peers/N/nonce/op%u/%u/%u/%u\n",p.result.summary[0],p.result.summary[1],p.result.summary[2],p.result.summary[3],p.result.summary[4],p.result.summary[5],p.result.summary[6],p.result.summary[7]);
			failed=true; closeOnly=true; return false;
		}
		coordinatorNonce=p.result.master.nonce; lastActivationPages=p.result.staged_pages;
		memcpy(output,p.output,sizeof(p.output)); calls+=3;
		if (p.result.workers>peakWorkers) peakWorkers=p.result.workers;
		return true;
	}
	bool Q8WideMatvec(uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* vector,const uint32_t* scales,uint32_t* result,bool narrow=false) {
		PioneerFleetPlan plan;
		if (!HasLayerFfn() || coordinatedVersion<10 || !q8 || !vector || !scales || !result || !PioneerFleetPartition(inputs,outputs,fleet,plan)) return false;
		if (narrow && (!coordinatedNarrow || outputs>4096)) return false;
		if (!coordinatedRequest) coordinatedRequest=(tt_pioneer_coordinated_fleet*)malloc(sizeof(tt_pioneer_coordinated_fleet));
		if (!coordinatedRequest) { failed=true; return false; }
		size_t size=PioneerCoordinatedRequestBytes(outputs); auto& p=*coordinatedRequest;
		memset(&p,0,size); p.version=1; p.inputs=inputs; p.outputs=outputs; p.workers=fleet; p.weight_offset=offset;
		memcpy(p.input,vector,inputs*2); memcpy(p.scale,scales,inputs/32*4);
		int status=ioctl(fd,narrow?TT_PCI_PROBE_PIONEER_LAYER_NARROW:TT_PCI_PROBE_PIONEER_LAYER_WIDE,&p,size); cold=p.result.master.recovery_required!=0;
		bool proved=!status && (narrow?PioneerLayerNarrowProof(p.result,inputs,outputs,fleet,coordinatorNonce+1)
			:PioneerLayerWideProof(p.result,inputs,outputs,fleet,coordinatorNonce+1));
		for (unsigned i=0;i<outputs;i++) proved &= (p.output[i]&0x7f800000)!=0x7f800000;
		if (!proved) {
			const auto& m=p.result.master;
			fprintf(stderr,"Wide owned output STOP status%d state%u nonce%u start/complete/stop%u/%u/%u PC%x marker%x cold%u; no fallback/retry\n",status,m.state,m.nonce,m.started,m.completed,m.stopped,m.pc[1],m.marker,cold);
			fprintf(stderr," wide R/flits/W/flits%u/%u/%u/%u peers/N/nonce/op%u/%u/%u/%u\n",p.result.summary[0],p.result.summary[1],p.result.summary[2],p.result.summary[3],p.result.summary[4],p.result.summary[5],p.result.summary[6],p.result.summary[7]);
			failed=true; closeOnly=true; return false;
		}
		coordinatorNonce=p.result.master.nonce; memcpy(result,p.output,outputs*4); calls++; if (narrow) narrowCalls++; else wideCalls++;
		lastActivationPages=p.result.staged_pages; if (!lastActivationPages) activationReuses++;
		if (plan.workers>peakWorkers) peakWorkers=plan.workers;
		return true;
	}
	bool Q8Matvec(uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* vector,const uint32_t* scales,uint32_t* result) {
		if (decoderRequested) return false;
		if (coordinatedWide && outputs>4096) return Q8WideMatvec(offset,inputs,outputs,vector,scales,result);
		if (coordinatedNarrow) return Q8WideMatvec(offset,inputs,outputs,vector,scales,result,true);
		if (coordinated) {
			PioneerFleetPlan plan;
			if (!active || failed || finished || cold || !q8 || !vector || !scales || !result
				|| !PioneerFleetPartition(inputs,outputs,fleet,plan)) return false;
			if (!coordinatedRequest) coordinatedRequest=(tt_pioneer_coordinated_fleet*)malloc(sizeof(tt_pioneer_coordinated_fleet));
			if (!coordinatedRequest) { failed=true; return false; }
			size_t size=PioneerCoordinatedRequestBytes(outputs); auto& p=*coordinatedRequest;
			memset(&p,0,size); p.version=coordinatedDebug?2:1; p.inputs=inputs; p.outputs=outputs; p.workers=fleet; p.weight_offset=offset;
			memcpy(p.input,vector,inputs*2); memcpy(p.scale,scales,inputs/32*4);
			int status=ioctl(fd,TT_PCI_PROBE_PIONEER_COORDINATED_FLEET,&p,size); cold=p.result.master.recovery_required!=0;
			bool proved=!status && PioneerCoordinatedProof(p.result,inputs,outputs,fleet,coordinatorNonce+1,coordinatedDebug);
			for (unsigned i=0;i<outputs;i++) proved &= (p.output[i]&0x7f800000)!=0x7f800000;
			if (!proved) {
				const auto& m=p.result.master;
				fprintf(stderr,"Coordinated matrix STOP status%d state%u workers%u nonce%u start/complete/stop%u/%u/%u PC%x marker%x cold%u; no fallback/retry\n",
					status,m.state,p.result.workers,m.nonce,m.started,m.completed,m.stopped,m.pc[1],m.marker,cold);
				fprintf(stderr,"  scoped summary R/flits/W/flits %u/%u/%u/%u peers/N/nonce/op %u/%u/%u/%u\n",
					p.result.summary[0],p.result.summary[1],p.result.summary[2],p.result.summary[3],p.result.summary[4],p.result.summary[5],p.result.summary[6],p.result.summary[7]);
				fprintf(stderr,"  physical delta8");
				for (unsigned i=0;i<8;i++) fprintf(stderr," %u",uint32_t(m.after[i]-m.before[i]));
				fprintf(stderr,"\n");
				failed=true; closeOnly=true; return false;
			}
			coordinatorNonce=p.result.master.nonce; memcpy(result,p.output,outputs*4); calls++;
			lastActivationPages=p.result.staged_pages; if (!lastActivationPages) activationReuses++;
			if (plan.workers>peakWorkers) peakWorkers=plan.workers;
			return true;
		}
		if (fleet) {
			// Measured fixed-worker dispatch is expensive for tiny row sets.
			// Use2 for small learned projections; the whole vocabulary uses
			// up to120. More cores must not be equated with a faster decode.
			unsigned limit=outputs<=6144 && fleet>2?2:fleet;
			PioneerFleetPlan plan;
			if (!active || failed || finished || cold || !q8 || !vector || !scales || !result || !PioneerFleetPartition(inputs,outputs,limit,plan)) return false;
			if (!fleetRequest) fleetRequest=(tt_pioneer_runtime_fleet*)malloc(sizeof(tt_pioneer_runtime_fleet));
			if (!fleetRequest) { failed=true; return false; }
			size_t size=PioneerFleetRequestBytes(outputs); auto& p=*fleetRequest;
			memset(&p,0,size); p.version=2; p.inputs=inputs; p.outputs=outputs; p.workers=limit; p.weight_offset=offset;
			memcpy(p.input,vector,inputs*2); memcpy(p.scale,scales,inputs/32*4);
			int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RUNTIME_Q8_FLEET,&p,size); cold=p.result.recovery_required!=0;
			if (status || !PioneerFleetProof(p.result,inputs,outputs,limit)) {
				fprintf(stderr,"Whole-projection STOP status%d state%u workers%u s/c/s%u/%u/%u cold%u; close-only, no fallback/retry\n",status,p.result.state,p.result.workers,p.result.started,p.result.completed,p.result.stopped,cold);
				failed=true; closeOnly=true; return false;
			}
			memcpy(result,p.output,outputs*4); calls++; if (plan.workers>peakWorkers) peakWorkers=plan.workers; return true;
		}
		if (!active || failed || finished || cold || !q8 || !vector || !scales || !result || !(dual?PioneerRuntimeDualShape(inputs,outputs):compact?PioneerQ8CompactShape(inputs,outputs):PioneerMatvecShape(inputs,outputs))) return false;
		auto* quant=(tt_pioneer_runtime_q8_matvec*)request;
		memset(quant,0,requestBytes); request->version=1; request->inputs=inputs; request->outputs=outputs; request->weight_offset=offset;
		memcpy(request->input,vector,inputs*2); memcpy(quant->scale,scales,inputs/32*4);
		int status=ioctl(fd,reuse?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL_REUSE:dual?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL:f32Scales?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE_F32_SCALES:native?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE:compact?TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_COMPACT:TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8,quant,requestBytes);
		const auto& r=request->result; cold=r.recovery_required!=0;
		if (status || r.version!=1 || r.state!=TT_RUNTIME_PASSED || !r.started || !r.completed || !r.stopped || cold) {
			closeOnly=status!=0;
			fprintf(stderr,"Resident Q8 matvec STOP: status %d state %u start/complete/stop %u/%u/%u cold %u; no fallback/retry\n",status,r.state,r.started,r.completed,r.stopped,cold);
			failed=true; return false;
		}
		if (dual && !PioneerRuntimeDualProof(((tt_pioneer_runtime_dual_matvec*)request)->core,inputs,outputs)) {
			fprintf(stderr,"Resident two-core proof mismatch; output not accepted, close-only, no fallback/retry\n"); failed=true; closeOnly=true; return false;
		}
		memcpy(result,request->output,outputs*4); calls++; return true;
	}
	bool AttentionGroups(unsigned tokens,const uint32_t* query,const uint32_t* keys,const uint32_t* values,uint32_t* output) {
		if (residentKV) return false; // distinct image/ABI; never silently switch
		if (!active || failed || finished || cold || !fleet || !attention || !query || !keys || !values || !output || !PioneerFleetAttentionBytes(tokens)) return false;
		if (!fleetAttention) fleetAttention=(tt_pioneer_fleet_attention*)malloc(sizeof(tt_pioneer_fleet_attention));
		if (!fleetAttention) { failed=true; return false; }
		auto& p=*fleetAttention; size_t bytes=PioneerFleetAttentionBytes(tokens);
		memset(&p,0,bytes); p.version=1; p.tokens=tokens; memcpy(p.query,query,8192);
		for (unsigned g=0;g<8;g++) {
			if (!PioneerAttentionTileInputValid(tokens,query+g*256,keys+g*tokens*64,values+g*tokens*64)) return false;
			memcpy(p.prefix+g*tokens*128,keys+g*tokens*64,tokens*256);
			memcpy(p.prefix+g*tokens*128+tokens*64,values+g*tokens*64,tokens*256);
		}
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_FLEET_ATTENTION,&p,bytes);
		bool proved=!status;
		for (const auto& r:p.result) {
			cold |= r.recovery_required!=0;
			proved &= r.version==1 && r.state==TT_ATTN_TILE_PASSED && r.prepared && r.context_reset && r.started && r.completed && r.stopped && !r.recovery_required
				&& r.tokens==tokens && r.count==tokens && !r.nonfinite && r.staged_words==256+tokens*128
				&& r.phase==TT_ATTENTION_TILE_DONE && r.marker==TT_ATTENTION_TILE_MARKER && r.pc[0]==TT_ATTENTION_TILE_PARK && r.pc[1]==r.pc[0];
			for (unsigned w=0;w<4;w++) proved &= r.before[w]==r.after[w];
		}
		if (!proved) { failed=true; closeOnly=true; fprintf(stderr,"Whole-attention STOP status%d cold%u; close-only, no fallback/retry\n",status,cold); return false; }
		memcpy(output,p.output,8192); attentionCalls+=8; return true;
	}
	bool CoordinatorProbe(unsigned workers) {
		if (!active || failed || finished || cold || !fleet || !workers || workers>119 || coordinatorNonce>=0x7ffffffe) return false;
		tt_pioneer_coordinator_probe p={}; p.version=1; p.workers=workers;
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_COORDINATOR_PROBE,&p,sizeof(p));
		cold=p.result.recovery_required!=0;
		bool okay=!status && PioneerCoordinatorProbeProof(p.result,workers,coordinatorNonce+1);
		fprintf(stderr,"On-card coordinator read-only peers%u matched%u nonce%u launch/complete/stop%u/%u/%u state%u PC%x cold%u %s\n",
			workers,p.result.matched,p.result.nonce,p.result.started,p.result.completed,p.result.stopped,p.result.state,p.result.pc[1],cold,okay?"PASS":"FAIL");
		if (!okay) { failed=true; closeOnly=true; }
		else coordinatorNonce=p.result.nonce;
		return okay;
	}
	bool PeerPcProbe(unsigned worker) {
		if (!active || failed || finished || cold || !fleet || !worker || worker>=120) return false;
		tt_pioneer_peer_pc_read p={}; p.version=1; p.worker=worker;
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_PEER_PC_READ,&p,sizeof(p));
		cold=p.result.recovery_required!=0;
		bool okay=!status && PioneerPeerPcReadProof(p.result,worker);
		fprintf(stderr,"Peer-PC worker%u CPU%x/%x NoC%x/%x requests%u completed%u state%u cold%u %s\n",
			worker,p.result.cpu_before[0],p.result.cpu_after[1],p.result.noc_pc[0],p.result.noc_pc[1],
			p.result.requests,p.result.completed,p.result.state,cold,okay?"PASS":"FAIL");
		if (!okay) { failed=true; closeOnly=true; } // no retry, fallback or result publication
		return okay;
	}
	bool ResidentAttentionStep(unsigned tokens,unsigned layer,const uint32_t* query,const uint32_t* keys,const uint32_t* values,uint32_t* output) {
		if (!active || failed || finished || cold || !residentKV || !query || !keys || !values || !output
			|| !tokens || tokens>256 || layer>=28) return false;
		if (!residentAttention) residentAttention=(tt_pioneer_resident_attention*)malloc(sizeof(tt_pioneer_resident_attention));
		if (!residentAttention) { failed=true; return false; }
		auto& p=*residentAttention; memset(&p,0,sizeof(p)); p.version=coordinated?3:payloadBlocks?2:1; p.tokens=tokens; p.layer=layer;
		memcpy(p.input,query,8192);
		for (unsigned g=0;g<8;g++) { memcpy(p.input+2048+g*128,keys+g*64,256); memcpy(p.input+2048+g*128+64,values+g*64,256); }
		if (!PioneerResidentAttentionInput(tokens,layer,p.input)) return false;
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_RESIDENT_ATTENTION,&p,sizeof(p));
		const auto& r=p.result.math; cold=r.recovery_required!=0;
		bool proved=!status && r.version==1 && r.state==TT_ATTN_TILE_PASSED && r.prepared && r.context_reset
			&& r.started && r.completed && r.stopped && r.polls && !cold && !r.nonfinite && r.tokens==tokens && r.count==tokens
			&& r.staged_words==3072 && p.result.groups==8 && p.result.layer==layer
			&& r.phase==TT_ATTENTION_TILE_DONE && r.marker==TT_ATTENTION_TILE_MARKER && r.pc[0]==TT_ATTENTION_TILE_PARK && r.pc[1]==r.pc[0]
			&& PioneerResidentAttentionCounterProof(tokens,p.result.before,p.result.after);
		for (unsigned w=0;w<4;w++) proved &= r.before[w]==p.result.before[w] && r.after[w]==p.result.after[w];
		for (auto value:p.output) proved &= (value&0x7f800000)!=0x7f800000;
		if (!proved) {
			failed=true; closeOnly=true;
			fprintf(stderr,"Resident-KV attention STOP status%d state%u n%u layer%u s/c/s%u/%u/%u pc%x phase%x count%u marker%x cold%u; no fallback/retry\n",
				status,r.state,tokens,layer,r.started,r.completed,r.stopped,r.pc[1],r.phase,r.count,r.marker,cold); return false;
		}
		memcpy(output,p.output,8192); attentionCalls+=8; return true;
	}
	bool AttentionTile(unsigned tokens,const uint32_t* query,const uint32_t* keys,const uint32_t* values,uint32_t* result) {
		if (!active || failed || finished || cold || !attention || !result
			|| !PioneerAttentionTileInputValid(tokens,query,keys,values)) return false;
		if (!attentionRequest) attentionRequest=(tt_pioneer_attention_tile_test*)malloc(sizeof(tt_pioneer_attention_tile_test));
		if (!attentionRequest) { failed=true; return false; }
		memset(attentionRequest,0,sizeof(*attentionRequest));
		auto& p=*attentionRequest; p.version=1; p.tokens=tokens;
		memcpy(p.query,query,1024); memcpy(p.key,keys,tokens*256); memcpy(p.value,values,tokens*256);
		int status=ioctl(fd,TT_PCI_PROBE_PIONEER_ATTENTION_TILE_SHARED,&p,sizeof(p));
		const auto& r=p.result; cold=r.recovery_required!=0;
		bool proved=!status && r.version==1 && r.state==TT_ATTN_TILE_PASSED && r.prepared && r.started
			&& r.completed && r.stopped && !cold && r.tokens==tokens && r.count==tokens && !r.nonfinite
			&& r.staged_words==256+tokens*128 && r.phase==TT_ATTENTION_TILE_DONE && r.marker==TT_ATTENTION_TILE_MARKER
			&& r.pc[0]==TT_ATTENTION_TILE_PARK && r.pc[1]==r.pc[0];
		for (unsigned w=0;w<4;w++) proved &= r.before[w]==r.after[w];
		if (!proved) {
			closeOnly=status!=0; failed=true;
			fprintf(stderr,"Resident attention STOP status %d state %u launch/complete/stop %u/%u/%u phase/count/pc %x/%u/%x cold %u; no fallback/retry\n",
				status,r.state,r.started,r.completed,r.stopped,r.phase,r.count,r.pc[1],cold); return false;
		}
		memcpy(result,p.output,1024); attentionCalls++; return true;
	}
	bool Finish() {
		if (finished) return !cold && !closeOnly;
		finished=true;
		if (active && !cold && !closeOnly) {
			tt_resident_session_result s={};
			int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_END,&s,sizeof(s));
			cold=status || s.state!=TT_RESIDENT_SESSION_CLOSED || !s.pci_restored || !s.host_released || s.cold;
			fprintf(stderr,"Resident END: state %u PCI/host %u/%u cold %u\n",s.state,s.pci_restored,s.host_released,cold);
		}
		if (cold) fprintf(stderr,"STOP: cold recovery required; no retry/reset/module reload\n");
		if (closeOnly) fprintf(stderr,"STOP: request failed / state uncertain; close-only, no further card commands\n");
		if (fd>=0) { close(fd); fd=-1; }
		active=false; return !cold && !closeOnly;
	}
};
#endif
