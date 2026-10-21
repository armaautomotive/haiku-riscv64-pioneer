/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_COHORT_SESSION_H
#define PIONEER_Q38_COHORT_SESSION_H
#include "PioneerQ38PublicModelSource.h"
#include "PioneerQwen38CohortHost.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#ifdef PIONEER_Q38_MODEL_ROOT_PROFILE
#if !defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_ROOT_STATS_BENCH) || defined(PIONEER_Q38_BENCHMARK_HOST) || defined(PIONEER_Q38_FFN_REPEAT_BENCH)
#error Model root profile is isolated fixed-public original-TC THREE32 Work ONLY
#endif
#include "PioneerQ38ModelRootProfile.h"
#endif
#ifdef PIONEER_Q38_ROOT_STATS_BENCH
#if !defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT)
#error Root stats are isolated original-TC Work projection diagnostics ONLY
#endif
#include "PioneerQ38RootStatsProfile.h"
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#endif
#ifdef PIONEER_Q38_CPU_VALIDATION
#ifdef PIONEER_Q38_WORK_INFERENCE
#error Work mode must never contain a CPU reference path
#endif
#include "PioneerQ38CpuValidation.h"
#endif
template<class IO> auto PioneerQ38SessionNow(IO& io,int) -> decltype(io.NowNanos()) { return io.NowNanos(); }
template<class IO> uint64_t PioneerQ38SessionNow(IO&,long) { return 0; } // offline transport without clock
struct PioneerQ38CohortSessionMode {
	static constexpr unsigned Workers=120,Operation=TT_PCI_PROBE_Q38_COHORT_RUN;
	template<class IO> bool Support(IO& io,int fd) {
		tt_q38_cohort_support support={};
		return io.Call(fd,TT_PCI_PROBE_Q38_COHORT_SUPPORT,&support,sizeof(support)) && support.driver==TT_Q38_COHORT_DRIVER
			&& support.version==TT_Q38_COHORT_ABI && support.workers==120 && support.rows==256 && support.max_inputs==17408
			&& support.banks==7 && support.offset_alignment==64 && !support.model_inference;
	}
	static auto Request(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input) { return PioneerQ38CohortRequest(wave,input); }
	bool Collect(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input,const tt_q38_cohort_probe& reply,float* scratch,unsigned rows)
	{ return PioneerQ38CohortCollect(wave,input,reply,scratch,rows); }
};
// Explicit transport strategy;production instantiates ONLY the fixed fully
// authenticated source. Offline tests use a synthetic source+NO-device IO.
// No retry,substituted output or physical-coordinate capability. CPU learned
// products exist ONLY in the separately compiled CPU_VALIDATION diagnostic.
template<class IO,class Source=PioneerQ38PublicModelSource,class Mode=PioneerQ38CohortSessionMode> class PioneerQ38CohortSession {
	IO& io; Source& source;
	Mode mode;
	PioneerQwen38PackedPlan layout;
	std::vector<uint8_t> uploaded;
	int fd=-1;
	bool attempted=false,active=false,failed=false,finished=false,clean=false;
	bool (*observe)(void*,uint64_t,uint64_t)=nullptr;
	void* observer=nullptr;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	bool spanPrimed=false,spanAttempted=false;
#endif
	void Elapsed(uint64_t start,uint64_t& total) {
		uint64_t end=PioneerQ38SessionNow(io,0);
		if (start && end>=start) total+=end-start; // diagnostic ONLY,not a completion/timeout authority
	}
	bool Stop() {
		failed=true; active=false; finished=true;
		if (fd>=0) { io.Close(fd); fd=-1; } // CLOSE ONLY:kernel retains UNKNOWN owners
		return false;
	}
	static bool BulkProof(const tt_q38_bank_bulk& b) {
		const auto& t=b.result.transfer;
		if (b.result.pages!=b.pages || b.result.host_words!=b.pages*1024 || b.result.host_ready!=1 || b.result.l1_prepared!=1
			|| t.version!=1 || t.state!=TT_STORE_PASSED || t.requests!=6 || t.completed!=6 || t.wrote_device!=1
			|| t.host_write_issued!=1 || t.host_visible!=1 || t.readback_complete!=1 || t.mismatches || t.recovery_required) return false;
		for (unsigned s=0;s<6;s++) for (unsigned c=0;c<4;c++) if (uint32_t(t.after[s][c]-t.before[s][c])!=(c==1?b.pages*64:1)) return false;
		return true;
	}
	bool UploadTensor(unsigned tensor) {
		if (!source.Ready() || !layout.MatchesSource(source.Model())) return Stop();
		const auto& parts=layout.Parts(); // immutable private metadata; do not fstat on every loop condition
#ifdef PIONEER_Q38_SPAN_PRELOAD
		// NEVER fall back to the legacy upload, or replace code after peers
		// have booted. Only the explicit pre-cohort priming owns this path.
		if (!spanPrimed) return Stop();
		for (unsigned f=0;f<parts.size();f++) {
			const auto& p=parts[f];
			if (p.kind==PIONEER_Q38_WEIGHT && p.tensor==tensor && !uploaded[f]) return Stop();
		}
		return source.Ready() || Stop();
#else
		// The session exclusively owns this immutable layout. Only metadata
		// iteration is cached;EVERY page read/upload and result publication
		// retains its existing live-source authentication and proof gates.
		for (unsigned f=0;f<parts.size();f++) {
			const auto& part=parts[f]; if (part.kind!=PIONEER_Q38_WEIGHT || part.tensor!=tensor || uploaded[f]) continue;
			for (unsigned page=0;page<part.allocated/4096;) {
				tt_q38_bank_bulk bulk={}; bulk.version=1; bulk.bank=part.bank;
				bulk.offset=uint64_t(part.proposedAddress)-layout.Base+uint64_t(page)*4096;
				bulk.pages=part.allocated/4096-page; if (bulk.pages>4) bulk.pages=4;
				uint64_t readStart=PioneerQ38SessionNow(io,0);
				for (unsigned p=0;p<bulk.pages;p++) if (!source.ReadWeightPage(layout,f,page+p,
					reinterpret_cast<uint8_t*>(bulk.data+p*1024),4096)) return Stop();
				Elapsed(readStart,sourceReadNanos);
				if (!source.Ready()) return Stop();
				uint64_t uploadStart=PioneerQ38SessionNow(io,0);
				bool transferred=io.Call(fd,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&bulk,sizeof(bulk));
				Elapsed(uploadStart,bulkUploadNanos);
				if (!transferred || !BulkProof(bulk)) return Stop();
				pagesUploaded+=bulk.pages; page+=bulk.pages;
			}
			if (!source.Ready()) return Stop();
			uploaded[f]=1; bytesRetained+=part.allocated;
		}
		return source.Ready() || Stop();
#endif
	}
public:
	uint64_t pagesUploaded=0,bytesRetained=0,calls=0;
#ifdef PIONEER_Q38_MODEL_ROOT_PROFILE
	PioneerQ38ModelRootProfile modelRootProfile;
#endif
	uint64_t sourceReadNanos=0,bulkUploadNanos=0,runNanos=0;
	unsigned peakWorkers=0;
#ifdef PIONEER_Q38_ROOT_STATS_BENCH
	PioneerQ38RootStatsProfile rootStats;
#endif
	PioneerQ38CohortSession(IO& transport,Source& authenticatedSource):io(transport),source(authenticatedSource) {}
	~PioneerQ38CohortSession() { Finish(); }
	PioneerQ38CohortSession(const PioneerQ38CohortSession&)=delete;
	PioneerQ38CohortSession& operator=(const PioneerQ38CohortSession&)=delete;
	bool Failed() const { return failed; }
	// Diagnostics only. False means LOST caller/channel: CLOSE ONLY, not
	// a user cancellation or permission to skip any transport proof.
	void Observe(bool (*callback)(void*,uint64_t,uint64_t),void* owner) { observe=callback; observer=owner; }
	bool Abort() { return Stop(); } // CLOSE ONLY; never END/retry after a graph error
	bool Open() {
		if (attempted || failed || finished) return false;
		attempted=true;
		if (!source.Ready() || !layout.Build(source.Model(),8192,true)) return Stop(); // BEFORE accelerator FD
		uploaded.assign(layout.Parts().size(),0);
		fd=io.Open(); if (fd<0) return Stop();
		tt_q38_bank_support storage={};
		if (!mode.Support(io,fd)) return Stop();
#ifdef PIONEER_Q38_SPAN_PRELOAD
		tt_q38_span_support span={};
		if (!io.Call(fd,
#ifdef PIONEER_Q38_SPAN_FAST
			TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,
#else
			TT_PCI_PROBE_Q38_SPAN_SUPPORT,
#endif
			&span,sizeof(span)) || span.driver!=55 || span.version!=1
			|| span.banks!=7 || span.page_bytes!=4096 || span.max_bytes!=1048576 || span.before_cohort!=1
			|| span.explicit_end!=1 || span.reserved) return Stop(); // BEFORE BANK_BEGIN
#endif
		if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&storage,sizeof(storage)) || storage.driver!=45 || storage.version!=1 || storage.banks!=7
			|| storage.page_bytes!=4096 || storage.max_pages!=4 || storage.storage_only!=1 || storage.bytes_per_bank!=PioneerResidentBanks::Capacity) return Stop();
		tt_q38_bank_session session={}; session.version=1;
		if (!source.Ready() || !io.Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))) return Stop();
		const auto& r=session.result;
		if (r.version!=1 || r.state!=TT_RESIDENT_SESSION_READY || r.active!=1 || r.host_ready!=1 || r.cold || r.capabilities
			|| r.bytes!=PioneerResidentBanks::Total) return Stop();
		active=true; return true;
	}
#ifdef PIONEER_Q38_SPAN_PRELOAD
	// UINT32_MAX means ALL original learned projection weights. CPU-only
	// embeddings/FP32 metadata are not needlessly copied to the card. A FFN
	// graph may request just its ONE tensor for bounded transport comparison.
	// Neither path can prime again or upload after the first cohort RUN.
	bool PrimeSpan(unsigned tensor=UINT32_MAX) {
		if (!active || failed || finished || spanAttempted || calls || !source.Ready()
			|| (tensor!=UINT32_MAX && tensor>=source.Model().Tensors().size())) return Stop();
		spanAttempted=true; auto request=std::make_unique<tt_q38_span_upload>(); unsigned nonce=0,parts=0;
		const auto& fragments=layout.Parts();
		uint64_t plannedBytes=0;
		for (const auto& p:fragments) if (p.kind==PIONEER_Q38_WEIGHT && (tensor==UINT32_MAX || p.tensor==tensor)) {
			const auto& t=source.Model().Tensors()[p.tensor];
			if (t.type && t.rank==2 && t.name!="token_embd.weight") plannedBytes+=p.allocated;
		}
		for (unsigned f=0;f<fragments.size();f++) {
			const auto& p=fragments[f]; if (p.kind!=PIONEER_Q38_WEIGHT || (tensor!=UINT32_MAX && p.tensor!=tensor)) continue;
			const auto& t=source.Model().Tensors()[p.tensor];
			if (!t.type || t.rank!=2 || t.name=="token_embd.weight") continue;
			for (unsigned page=0;page<p.allocated/4096;) {
				unsigned pages=unsigned(p.allocated/4096)-page; if (pages>256) pages=256;
				request->version=1; request->bank=p.bank; request->bytes=pages*4096; request->reserved=0;
				request->offset=uint64_t(p.proposedAddress)-layout.Base+uint64_t(page)*4096; request->result={};
				uint64_t start=PioneerQ38SessionNow(io,0);
				for (unsigned at=0;at<pages;at++) if (!source.ReadWeightPage(layout,f,page+at,
					reinterpret_cast<uint8_t*>(request->data+at*1024),4096)) return Stop();
				Elapsed(start,sourceReadNanos);
				if (!source.Ready()) return Stop();
				start=PioneerQ38SessionNow(io,0);
				bool okay=io.Call(fd,
#ifdef PIONEER_Q38_SPAN_FAST
					TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,
#else
					TT_PCI_PROBE_Q38_SPAN_UPLOAD,
#endif
					request.get(),sizeof(*request));
				Elapsed(start,bulkUploadNanos); const auto& r=request->result;
				if (!okay || !source.Ready() || r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.bytes!=pages*4096
					|| r.pages!=pages || r.nonce!=++nonce || r.started!=1 || r.completed!=1 || r.held!=1
#ifdef PIONEER_Q38_SPAN_FAST
					|| r.host_exact || r.matched_words
#else
					|| r.host_exact!=1 || r.matched_words!=pages*1024
#endif
					|| r.recovery_required || r.reserved) return Stop();
				pagesUploaded+=pages; page+=pages;
				if (observe && !observe(observer,pagesUploaded*4096,plannedBytes)) return Stop();
			}
			if (!source.Ready()) return Stop();
			uploaded[f]=1; bytesRetained+=p.allocated; parts++;
		}
		if (!parts || !source.Ready()) return Stop();
		tt_q38_span_end end={1,0,0,0};
		if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end)) || end.version!=1 || end.reserved
			|| end.closed!=1 || end.recovery_required || !source.Ready()) return Stop();
		spanPrimed=true; return true;
	}
#endif
	bool Multiply(unsigned tensor,const float* input,unsigned inputs,float* output,unsigned outputs,unsigned workers=Mode::Workers) {
		if (!active || failed || finished) return false;
		if (!source.Ready() || tensor>=source.Model().Tensors().size()) return Stop();
		const auto& t=source.Model().Tensors()[tensor];
		if (inputs!=t.inputs || outputs!=t.outputs || !PioneerQ38Disjoint(input,inputs,output,outputs)) return Stop();
		PioneerQwen38FleetPlan schedule;
		if (!workers || workers>Mode::Workers || !schedule.Build(source.Model(),layout,tensor,workers)) return Stop();
		PioneerQ38CohortInput quantized;
		if (!PioneerQ38CohortQuantize(t.type,inputs,input,quantized) || !UploadTensor(tensor)) return Stop();
		std::vector<float> scratch(outputs,0); // NEVER caller output until EVERY wave succeeds
		unsigned count=schedule.Jobs().back().wave+1;
		for (unsigned waveNumber=0;waveNumber<count;waveNumber++) {
			PioneerQ38CohortWave wave;
			if (!source.Ready() || !wave.Build(source.Model(),layout,schedule,waveNumber)) return Stop();
			auto request=mode.Request(wave,quantized);
			if (!request) return Stop();
			uint64_t runStart=PioneerQ38SessionNow(io,0);
			bool executed=io.Call(fd,Mode::Operation,request.get(),sizeof(*request));
			Elapsed(runStart,runNanos);
			if (!executed) return Stop();
			if (!source.Ready() || !mode.Collect(wave,quantized,*request,scratch.data(),outputs)) return Stop();
#ifdef PIONEER_Q38_MODEL_ROOT_PROFILE
			modelRootProfile.Add(wave.type,wave.inputs,wave.workers,request->result.stats); // data ONLY,never gates Stop/END
#endif
#ifdef PIONEER_Q38_ROOT_STATS_BENCH
			rootStats.Add(request->result.stats); // diagnostic data ONLY;NEVER controls Stop/END
#endif
			calls++; if (peakWorkers<wave.workers) peakWorkers=wave.workers;
		}
		if (!source.Ready()) return Stop();
#ifdef PIONEER_Q38_CPU_VALIDATION
		// No result publication and NO END/retry/substitution on mismatch.
		if (!PioneerQ38CpuValidate(source,tensor,input,inputs,scratch.data(),outputs)) return Stop();
#endif
		memcpy(output,scratch.data(),outputs*4); return true;
	}
	bool Finish() {
		if (finished) return clean && !failed;
		finished=true;
		if (fd<0) { clean=!attempted && !failed; return clean; }
		if (failed) return Stop();
		tt_q38_bank_session session={}; session.version=1;
		if (!active || !io.Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session))) return Stop();
		const auto& r=session.result;
		clean=r.version==1 && r.state==TT_RESIDENT_SESSION_CLOSED && !r.active && r.pci_restored==1 && r.host_released==1 && !r.cold;
		active=false;
		if (io.Close(fd)) clean=false;
		fd=-1; failed=!clean; return clean;
	}
};
#endif
