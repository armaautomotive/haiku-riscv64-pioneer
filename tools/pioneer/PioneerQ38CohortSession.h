/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_COHORT_SESSION_H
#define PIONEER_Q38_COHORT_SESSION_H
#include "PioneerQ38PublicModelSource.h"
#include "PioneerQwen38CohortHost.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
template<class IO> auto PioneerQ38SessionNow(IO& io,int) -> decltype(io.NowNanos()) { return io.NowNanos(); }
template<class IO> uint64_t PioneerQ38SessionNow(IO&,long) { return 0; } // offline transport without clock
// Explicit transport strategy;production instantiates ONLY the fixed fully
// authenticated source. Offline tests use a synthetic source+NO-device IO.
// No retry,CPU products,substituted output or physical-coordinate capability.
template<class IO,class Source=PioneerQ38PublicModelSource> class PioneerQ38CohortSession {
	IO& io; Source& source;
	PioneerQwen38PackedPlan layout;
	std::vector<uint8_t> uploaded;
	int fd=-1;
	bool attempted=false,active=false,failed=false,finished=false,clean=false;
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
		for (unsigned f=0;f<layout.Parts().size();f++) {
			const auto& part=layout.Parts()[f]; if (part.kind!=PIONEER_Q38_WEIGHT || part.tensor!=tensor || uploaded[f]) continue;
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
		return true;
	}
public:
	uint64_t pagesUploaded=0,bytesRetained=0,calls=0;
	uint64_t sourceReadNanos=0,bulkUploadNanos=0,runNanos=0;
	unsigned peakWorkers=0;
	PioneerQ38CohortSession(IO& transport,Source& authenticatedSource):io(transport),source(authenticatedSource) {}
	~PioneerQ38CohortSession() { Finish(); }
	PioneerQ38CohortSession(const PioneerQ38CohortSession&)=delete;
	PioneerQ38CohortSession& operator=(const PioneerQ38CohortSession&)=delete;
	bool Failed() const { return failed; }
	bool Abort() { return Stop(); } // CLOSE ONLY; never END/retry after a graph error
	bool Open() {
		if (attempted || failed || finished) return false;
		attempted=true;
		if (!source.Ready() || !layout.Build(source.Model(),8192,true)) return Stop(); // BEFORE accelerator FD
		uploaded.assign(layout.Parts().size(),0);
		fd=io.Open(); if (fd<0) return Stop();
		tt_q38_cohort_support support={}; tt_q38_bank_support storage={};
		if (!io.Call(fd,TT_PCI_PROBE_Q38_COHORT_SUPPORT,&support,sizeof(support)) || support.driver!=TT_Q38_COHORT_DRIVER
			|| support.version!=TT_Q38_COHORT_ABI || support.workers!=120 || support.rows!=256 || support.max_inputs!=17408
			|| support.banks!=7 || support.offset_alignment!=64 || support.model_inference) return Stop();
		if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&storage,sizeof(storage)) || storage.driver!=45 || storage.version!=1 || storage.banks!=7
			|| storage.page_bytes!=4096 || storage.max_pages!=4 || storage.storage_only!=1 || storage.bytes_per_bank!=PioneerResidentBanks::Capacity) return Stop();
		tt_q38_bank_session session={}; session.version=1;
		if (!source.Ready() || !io.Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))) return Stop();
		const auto& r=session.result;
		if (r.version!=1 || r.state!=TT_RESIDENT_SESSION_READY || r.active!=1 || r.host_ready!=1 || r.cold || r.capabilities
			|| r.bytes!=PioneerResidentBanks::Total) return Stop();
		active=true; return true;
	}
	bool Multiply(unsigned tensor,const float* input,unsigned inputs,float* output,unsigned outputs,unsigned workers=120) {
		if (!active || failed || finished) return false;
		if (!source.Ready() || tensor>=source.Model().Tensors().size()) return Stop();
		const auto& t=source.Model().Tensors()[tensor];
		if (inputs!=t.inputs || outputs!=t.outputs || !PioneerQ38Disjoint(input,inputs,output,outputs)) return Stop();
		PioneerQwen38FleetPlan schedule;
		if (!schedule.Build(source.Model(),layout,tensor,workers)) return Stop();
		PioneerQ38CohortInput quantized;
		if (!PioneerQ38CohortQuantize(t.type,inputs,input,quantized) || !UploadTensor(tensor)) return Stop();
		std::vector<float> scratch(outputs,0); // NEVER caller output until EVERY wave succeeds
		unsigned count=schedule.Jobs().back().wave+1;
		for (unsigned waveNumber=0;waveNumber<count;waveNumber++) {
			PioneerQ38CohortWave wave;
			if (!source.Ready() || !wave.Build(source.Model(),layout,schedule,waveNumber)) return Stop();
			auto request=PioneerQ38CohortRequest(wave,quantized);
			if (!request) return Stop();
			uint64_t runStart=PioneerQ38SessionNow(io,0);
			bool executed=io.Call(fd,TT_PCI_PROBE_Q38_COHORT_RUN,request.get(),sizeof(*request));
			Elapsed(runStart,runNanos);
			if (!executed) return Stop();
			if (!source.Ready() || !PioneerQ38CohortCollect(wave,quantized,*request,scratch.data(),outputs)) return Stop();
			calls++; if (peakWorkers<wave.workers) peakWorkers=wave.workers;
		}
		if (!source.Ready()) return Stop();
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
