/* SPDX-License-Identifier: MIT */
#define PIONEER_LAYER_CONTROLLER_MOCK_ONLY
#include "pioneer_layer_controller_test.cpp"
struct WideControllerIO:LayerIO {
	std::vector<uint32_t> wideData=std::vector<uint32_t>(TT_WIDE_OUTPUT_WORDS,0xabcdef00);
	uint64_t widePhysical=UINT64_C(0x140000000);
	bool wideAllocated=false,wideCached=true,wideDirty=false,wideQuarantined=false,wideReleased=false;
	bool wideLost=false,omitWidePayload=false;
	unsigned pageMapCalls=0;
	int badWidePage=-1;
	int AllocateWideOutput() { assert(!wideAllocated); wideAllocated=true; return 0; }
	int WideOutputMap(uint64_t& at,uint64_t& bytes) { assert(wideAllocated && !wideReleased); at=widePhysical+(wideLost && starts?TT_WIDE_OUTPUT_BYTES:0); bytes=TT_WIDE_OUTPUT_BYTES; return 0; }
	int WideOutputPageMap(unsigned p,uint64_t& at,uint64_t& bytes) { assert(p<256); pageMapCalls++; at=widePhysical+p*4096+(int(p)==badWidePage?4096:0); bytes=4096; return 0; }
	int SetWideOutputNonCached() { wideCached=false; return 0; }
	void WideHostFence() { wideDirty=false; }
	void BulkHostFence() { CoordinatedIO::BulkHostFence(); wideDirty=false; }
	void WideHostStore(unsigned w,uint32_t value) { assert(wideAllocated && !wideReleased && !wideQuarantined && !wideCached && w<wideData.size() && C().reset==0x47800); wideData[w]=value; wideDirty=true; }
	uint32_t WideHostLoad(unsigned w) { assert(!wideDirty && wideAllocated && !wideReleased && !wideQuarantined && !wideCached && w<wideData.size() && C().reset==0x47800); return wideData[w]; }
	int ReleaseWideOutput() { assert(!wideReleased && !wideQuarantined); wideReleased=true; return 0; }
	void QuarantineWideOutput() { wideQuarantined=true; }
	void RuntimeResetWrite(uint32_t value) {
		if (value!=0x46800 || data[1]!=6) {
			if (C().reset==0x46800 && value==0x87ffff) assert(!wideLost);
			LayerIO::RuntimeResetWrite(value); return;
		}
		assert(!current && !quarantined && !wideQuarantined && !wideDirty && C().reset==0x87ffff && C().code[0]==TT_LAYER_BOOT && layer->running);
		assert(data[7]==uint32_t(widePhysical) && data[8]==uint32_t(widePhysical>>32) && data[9]==TT_WIDE_OUTPUT_BYTES);
		for (unsigned i=0;i<8192;i++) assert(layerProgram[i]==BlackholeLayerCodeWord(i));
		PioneerFleetPlan p; assert(PioneerFleetPartition(data[3],data[4],data[2],p)); unsigned n=data[4];
		for (unsigned w=1;w<=p.workers;w++) assert(fleet[w].Compute().running && !fleet[w].persistentIdle && fleet[w].persistentSequence==data[64+w-1]);
		C().reset=value; starts++; if (executeTimeout) return;
		unsigned r=5*p.workers,rf=r-2*p.workers+n/8,w=3*p.workers,wf=2*p.workers+n/16;
		const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		data[3072]=r; data[3073]=rf; data[3074]=w; data[3075]=wf; data[3076]=p.workers;
		data[3077]=n; data[3078]=data[6]-(staleSummary?1:0); data[3079]=6;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++; if (badSummary) data[3088]++;
		if (!omitWidePayload) for (unsigned i=0;i<n;i++) wideData[i]=nonfinite && !i?0x7f800000:0x40000000;
		status[0]=TT_LAYER_DONE; status[1]=p.workers+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
	}
};
struct WideFixture:LayerFixtureBase<WideControllerIO> {
	PioneerWideOutputLease wide;
	bool PrepareWide() { return Prepare() && PioneerPrepareWideOutput(io,host,transfer,wide); }
	bool RunWide(unsigned k=1024,unsigned n=6144,unsigned workers=119) {
		std::vector<uint32_t> input(k/2,0x40014001),scales(k/32,0x3f800000),output(n,0xabcdef00);
		PioneerLayerJob job(&code,0,0,6,&wide);
		bool okay=PioneerCoordinatedMatvec(io,io.core[0]->Baseline(),host,transfer,fleet.data(),bulk,coordinator,false,
			UINT64_C(0x80000000),0,k,n,workers,input.data(),scales.data(),output.data(),result,&job);
		if (okay) {
			assert(PioneerLayerWideProof(result,k,n,workers,coordinator.nonce) && !wide.pending && !transfer.pending
				&& !io.pioWords && !code.running && coordinator.inputValid && io.C().pciCommand[0]==0x402 && io.C().pciCommand[1]==0x402);
			for (auto value:output) assert(value==0x40000000);
		}
		return okay;
	}
};
#ifndef PIONEER_WIDE_CONTROLLER_MOCK_ONLY
int main()
{
	(void)Run; (void)Input;
	{
		WideFixture f; assert(f.PrepareWide());
		for (unsigned n:{32U,64U,6144U,8192U,151936U,262144U,32U}) assert(f.RunWide(1024,n));
		assert(f.io.starts==f.io.stops && f.io.inputReads==1 && f.result.staged_pages==0);
		for (unsigned k:{2048U,3072U,5120U,6144U}) assert(f.RunWide(k,6144));
		for (unsigned word=0;word<sizeof(f.result)/4;word++) {
			auto r=f.result; reinterpret_cast<uint32_t*>(&r)[word]^=word==offsetof(tt_coordinated_fleet_result,staged_pages)/4?8:1;
			if (word!=10) assert(!PioneerLayerWideProof(r,6144,6144,119,f.coordinator.nonce));
		}
		assert(f.Run() && !f.coordinator.inputValid && f.RunWide() && f.result.staged_pages==1);
		assert(f.Close() && PioneerReleaseWideOutput(f.io,f.wide,true));
	}
	for (unsigned fault=0;fault<12;fault++) {
		WideFixture f; assert(f.PrepareWide());
		f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9; f.io.omitWidePayload=fault==10; f.io.wideLost=fault==11;
		assert(!f.RunWide() && f.io.quarantined && f.io.wideQuarantined && f.transfer.quarantined && f.wide.quarantined
			&& f.code.unsafe && !f.coordinator.inputValid && f.result.master.recovery_required && !f.io.pioWords
			&& f.io.C().pciCommand[0]==0x402 && f.io.C().pciCommand[1]==0x402);
		if (fault!=5 && fault!=9 && fault!=10) assert(!f.io.stops);
		unsigned writes=f.io.layerWrites; assert(!f.Close() && f.io.layerWrites==writes && !PioneerReleaseWideOutput(f.io,f.wide,false) && !f.io.wideReleased);
	}
	{ WideFixture f; assert(f.PrepareWide()); f.io.badWidePage=255; assert(!f.RunWide() && !f.io.starts && f.io.wideQuarantined && f.result.master.recovery_required); }
	puts("PASS OFFLINE wide-result controller,all shapes/full vocabulary,independent third owner,exact cache reuse/FFN invalidation,all proof words,unknown map/source/completion => no reset/readback/restore/free");
}
#endif
