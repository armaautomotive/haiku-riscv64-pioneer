/* SPDX-License-Identifier: MIT */
#define PIONEER_WIDE_CONTROLLER_MOCK_ONLY
#include "pioneer_wide_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerSpanUpload.h"
struct SpanIO:WideControllerIO {
	bool corruptEcho=false;
	void RuntimeResetWrite(uint32_t value) {
		if (value!=0x46800) { WideControllerIO::RuntimeResetWrite(value); return; }
		assert(!current && !quarantined && !wideQuarantined && !wideDirty && C().reset==0x87ffff
			&& C().code[0]==TT_LAYER_BOOT && layer->running && data[1]==7 && data[2]==1 && !data[3]);
		assert(data[7]==uint32_t(widePhysical) && data[8]==uint32_t(widePhysical>>32) && data[9]==TT_WIDE_OUTPUT_BYTES);
		for (unsigned w=1;w<120;w++) assert(fleet[w].persistentIdle && !fleet[w].Compute().running && !fleet[w].persistentSequence);
		C().reset=value; starts++; if (executeTimeout) return;
		unsigned bytes=data[4],chunks=(bytes+16383)/16384,r=4*chunks,rf=bytes/16,w=3*chunks,wf=3*(bytes/64);
		const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		data[3072]=r; data[3073]=rf; data[3074]=w; data[3075]=wf; data[3076]=bytes/4096;
		data[3077]=bytes; data[3078]=data[6]-(staleSummary?1:0); data[3079]=7;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++; if (badSummary) data[3088]++;
		if (corruptEcho) wideData[bytes/8]^=1;
		status[0]=TT_LAYER_DONE; status[1]=bytes/4096+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
	}
};
struct SpanFixture:LayerFixtureBase<SpanIO> {
	PioneerWideOutputLease wide; tt_layer_span_result span={};
	bool PrepareSpan() { return Prepare() && PioneerPrepareWideOutput(io,host,transfer,wide); }
	bool Upload(unsigned bytes=TT_WIDE_OUTPUT_BYTES,uint64_t offset=0) {
		std::vector<uint32_t> source((bytes+3)/4);
		for (unsigned i=0;i<source.size();i++) source[i]=0x7f800000U^(i*0x1234567U);
		bool okay=PioneerSpanUpload(io,io.core[0]->Baseline(),host,transfer,wide,fleet.data(),bulk,coordinator,code,false,
			UINT64_C(0x80000000),offset,bytes,source.data(),span);
		if (okay) {
			assert(PioneerSpanProof(span,bytes,coordinator.nonce) && !wide.pending && !code.running && !coordinator.running
				&& !coordinator.inputValid && !io.pioWords && io.C().pciCommand[0]==0x402 && io.C().pciCommand[1]==0x402);
			for (unsigned i=0;i<source.size();i++) assert(io.wideData[i]==source[i]);
		}
		return okay;
	}
};
int main()
{
	(void)Run; (void)Input;
	{
		SpanFixture f; assert(f.PrepareSpan());
		for (unsigned bytes:{4096U,12288U,16384U,20480U,65536U,1048576U,4096U}) {
			f.coordinator.inputValid=true; assert(f.Upload(bytes));
		}
		assert(f.io.starts==f.io.stops && !f.io.inputReads && f.span.master.nonce==7);
		for (unsigned word=0;word<sizeof(f.span)/4;word++) {
			auto r=f.span; reinterpret_cast<uint32_t*>(&r)[word]^=1;
			if (word!=10) assert(!PioneerSpanProof(r,4096,f.coordinator.nonce));
		}
		assert(f.Close() && PioneerReleaseWideOutput(f.io,f.wide,true));
	}
	for (unsigned fault=0;fault<12;fault++) {
		SpanFixture f; assert(f.PrepareSpan());
		f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.corruptEcho=fault==9; f.io.layerLost=fault==10; f.io.wideLost=fault==11;
		assert(!f.Upload() && f.io.quarantined && f.io.wideQuarantined && f.transfer.quarantined && f.wide.quarantined
			&& f.code.unsafe && !f.coordinator.inputValid && f.span.master.recovery_required && !f.io.pioWords
			&& f.io.C().pciCommand[0]==0x402 && f.io.C().pciCommand[1]==0x402);
		if (fault!=5 && fault!=9) assert(!f.io.stops);
		unsigned writes=f.io.layerWrites; assert(!f.Close() && f.io.layerWrites==writes
			&& !PioneerReleaseWideOutput(f.io,f.wide,false) && !f.io.wideReleased);
	}
	{ SpanFixture f; assert(f.PrepareSpan()); f.io.badWidePage=255; assert(!f.Upload() && !f.io.starts && f.wide.quarantined); }
	{ SpanFixture f; assert(f.PrepareSpan()); for (unsigned b:{0U,1U,4095U,1048577U}) assert(!f.Upload(b) && !f.io.starts);
		assert(!f.Upload(1048576,0x7e3ff000) && !f.io.starts); assert(f.Close() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	puts("PASS OFFLINE span controller,all sizes/raw bits,complete frozen-input echo,all proof mutations,input invalidation,unknown source/map/host/completion => all-owner quarantine without reset/retry/restore/free");
}
