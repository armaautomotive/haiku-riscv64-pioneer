/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define main CohortHostTestMain
#include "pioneer_qwen38_cohort_host_test.cpp"
#undef main
#include "../PioneerQ38CohortSession.h"
struct FakeSource {
	PioneerQwen38Gguf model; uint64_t epoch=0; bool valid=true;
	explicit FakeSource(FILE* f) { assert(model.Open(f)); epoch=model.Epoch(); }
	bool Ready() const { return valid && model.Epoch()==epoch && model.Stable(); }
	const PioneerQwen38Gguf& Model() const { return model; }
	bool ReadWeightPage(const PioneerQwen38PackedPlan& p,unsigned f,unsigned n,uint8_t* data,unsigned bytes) {
		return Ready() && p.ReadWeightPage(model,f,n,data,bytes);
	}
};
struct SessionIO {
	FakeSource& source; unsigned tensor,opens=0,closes=0,begins=0,ends=0,uploads=0,runs=0,fault=0,failWave=0;
	bool stopped=false; unsigned cursor=0;
	PioneerQwen38PackedPlan layout; PioneerQwen38FleetPlan schedule;
	SessionIO(FakeSource& s,unsigned t):source(s),tensor(t) { assert(layout.Build(s.Model(),8192,true) && schedule.Build(s.Model(),layout,t,1)); }
	int Open() { assert(!stopped); opens++; return 73; }
	int Close(int fd) { assert(fd==73); closes++; return 0; }
	bool Call(int fd,uint32_t op,void* data,size_t bytes) {
		assert(fd==73 && !stopped);
		if (op==TT_PCI_PROBE_Q38_COHORT_SUPPORT) {
			assert(bytes==sizeof(tt_q38_cohort_support));
			*static_cast<tt_q38_cohort_support*>(data)={fault==1?51U:unsigned(TT_Q38_COHORT_DRIVER),TT_Q38_COHORT_ABI,120,256,17408,7,64,0}; return true;
		}
		if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
			*static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,fault==2?3U:4U,1,PioneerResidentBanks::Capacity}; return true;
		}
		if (op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_BANK_END) {
			auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={}; r.version=1;
			if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) { begins++; r.state=TT_RESIDENT_SESSION_READY; r.active=r.host_ready=1; r.bytes=PioneerResidentBanks::Total; }
			else { ends++; r.state=TT_RESIDENT_SESSION_CLOSED; r.pci_restored=r.host_released=1; if (fault==9) r.cold=1; }
			return true;
		}
		if (op==TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD) {
			assert(bytes==sizeof(tt_q38_bank_bulk)); auto& bulk=*static_cast<tt_q38_bank_bulk*>(data); uploads++;
			assert(PioneerQ38BankBulkRange(bulk.bank,bulk.offset,bulk.pages));
			bulk.result={}; bulk.result.pages=bulk.pages; bulk.result.host_words=bulk.pages*1024; bulk.result.host_ready=bulk.result.l1_prepared=1;
			auto& t=bulk.result.transfer; t.version=1; t.state=TT_STORE_PASSED; t.requests=t.completed=6;
			t.wrote_device=t.host_write_issued=t.host_visible=t.readback_complete=1;
			for (unsigned s=0;s<6;s++) for (unsigned c=0;c<4;c++) { t.before[s][c]=UINT32_MAX-10; t.after[s][c]=t.before[s][c]+(c==1?bulk.pages*64:1); }
			if (fault==3) { t.recovery_required=1; stopped=true; }
			return true;
		}
		assert(op==TT_PCI_PROBE_Q38_COHORT_RUN && bytes==sizeof(tt_q38_cohort_probe)); runs++;
		auto& r=*static_cast<tt_q38_cohort_probe*>(data);
		unsigned waves=schedule.Jobs().back().wave+1;
		PioneerQ38CohortWave wave; assert(wave.Build(source.Model(),layout,schedule,cursor%waves));
		assert(!memcmp(r.part,wave.part,sizeof(r.part))); Complete(wave,r); cursor++;
		if (runs==failWave) switch(fault) {
			case 4:stopped=true; return false;
			case 5:r.result.core[r.workers-1].after[7]++;stopped=true;break;
			case 6:r.output[r.workers-1][r.part[r.workers-1].groups*16-1]=0x7fc00000;stopped=true;break;
			case 7:source.valid=false;stopped=true;break;
			case 8:r.activation[0]^=1;stopped=true;break;
		}
		return true;
	}
};
int main()
{
	(void)&Check; (void)&CohortHostTestMain;
	Fixture fixture; FakeSource source(fixture.file);
	const auto* tensor=source.Model().Find("blk.3.attn_k.weight"); assert(tensor);
	unsigned index=unsigned(tensor-source.Model().Tensors().data());
	std::vector<float> input(5120,.125f),output(1024,-99),before=output;
	{
		SessionIO io(source,index); PioneerQ38CohortSession<SessionIO,FakeSource> session(io,source);
		assert(session.Open() && !session.Open() && io.begins==1);
		assert(session.Multiply(index,input.data(),5120,output.data(),1024,1));
		for (unsigned row=0;row<output.size();row++) assert(output[row]==float(row+1));
		unsigned uploads=io.uploads,runs=io.runs; assert(uploads && runs>1);
		assert(session.Multiply(index,input.data(),5120,output.data(),1024,1) && io.uploads==uploads && io.runs==runs*2);
		assert(session.pagesUploaded && session.bytesRetained && session.calls==runs*2 && session.peakWorkers==1);
		assert(session.Finish() && session.Finish() && io.ends==1 && io.closes==1);
	}
	for (unsigned fault=1;fault<=9;fault++) {
		source.valid=true; SessionIO io(source,index); io.fault=fault; io.failWave=2;
		PioneerQ38CohortSession<SessionIO,FakeSource> session(io,source);
		output=before;
		bool opened=session.Open(); assert(opened==(fault>2));
		if (opened) {
			bool result=session.Multiply(index,input.data(),5120,output.data(),1024,1);
			assert(result==(fault==9));
			if (fault!=9) assert(output==before && io.runs==(fault==3?0U:2U));
		}
		assert(!session.Finish() && session.Failed() && !session.Open() && io.closes==1 && io.ends==(fault==9?1U:0U));
		unsigned calls=io.uploads+io.runs+io.ends;
		assert(!session.Multiply(index,input.data(),5120,output.data(),1024,1) && !session.Finish());
		assert(calls==io.uploads+io.runs+io.ends);
	}
	source.valid=false;
	{
		SessionIO io(source,index); PioneerQ38CohortSession<SessionIO,FakeSource> session(io,source);
		assert(!session.Open() && !io.opens && !io.begins && !session.Finish());
	}
	source.valid=true;
	puts("PASS OFFLINE persistent cohort session:source before accelerator FD,old ABI/storage refusal,ALL page proofs,retained weights across repeated complete multiwave matrices,late-wave failures preserve caller output,CLOSE ONLY without END/retry/fallback,clean END exactly once;SYNTHETIC source/device,NOT native inference");
}
