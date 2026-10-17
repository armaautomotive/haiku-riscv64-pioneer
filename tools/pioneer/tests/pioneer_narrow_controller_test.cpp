/* SPDX-License-Identifier: MIT */
#define PIONEER_LAYER_CONTROLLER_MOCK_ONLY
#include "pioneer_layer_controller_test.cpp"
struct NarrowIO:LayerIO {
	void RuntimeResetWrite(uint32_t value) {
		if (value!=0x46800) { LayerIO::RuntimeResetWrite(value); return; }
		assert(!current && !quarantined && C().reset==0x87ffff && C().code[0]==TT_LAYER_BOOT && layer->running && data[1]==3);
		PioneerFleetPlan p; assert(PioneerFleetPartition(data[3],data[4],data[2],p)); unsigned n=data[4];
		assert(n<=4096 && data[7]==uint32_t(physical) && data[8]==uint32_t(physical>>32) && data[9]==16384);
		for (unsigned w=1;w<=p.workers;w++) assert(fleet[w].Compute().running && !fleet[w].persistentIdle && fleet[w].persistentSequence==data[64+w-1]);
		C().reset=value; starts++; if (executeTimeout) return;
		unsigned r=4*p.workers+1,rf=r-p.workers+n/8-1,w=2*p.workers+1,wf=2*p.workers+n/16;
		const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		data[3072]=r; data[3073]=rf; data[3074]=w; data[3075]=wf; data[3076]=p.workers;
		data[3077]=n; data[3078]=data[6]-(staleSummary?1:0); data[3079]=3;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++; if (badSummary) data[3088]++;
		if (!omitFusedPayload) for (unsigned i=0;i<n;i++) OutputBufferMock::data[i]=nonfinite && !i?0x7f800000:0x40000000;
		status[0]=TT_LAYER_DONE; status[1]=p.workers+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
	}
};
struct NarrowFixture:LayerFixtureBase<NarrowIO> {
	bool Narrow(unsigned k=1024,unsigned n=1024,unsigned peers=119) {
		std::vector<uint32_t> input(k/2,0x40014001),scales(k/32,0x3f800000),output(n,0xabcdef00);
		PioneerLayerJob job(&code,0,0,3);
		bool okay=PioneerCoordinatedMatvec(io,io.core[0]->Baseline(),host,transfer,fleet.data(),bulk,coordinator,false,
			UINT64_C(0x80000000),0,k,n,peers,input.data(),scales.data(),output.data(),result,&job);
		if (okay) { assert(PioneerLayerNarrowProof(result,k,n,peers,coordinator.nonce) && !transfer.pending && !code.running && coordinator.inputValid && !io.pioWords);
			for (auto value:output) assert(value==0x40000000); }
		return okay;
	}
};
int main()
{
	(void)Run; (void)Input;
	{ NarrowFixture f; assert(f.Prepare()); for (unsigned n:{32U,64U,1024U,2048U,3072U,4096U,32U}) assert(f.Narrow(1024,n));
		assert(f.io.inputReads==1 && !f.result.staged_pages && f.io.starts==f.io.stops);
		for (unsigned k:{2048U,3072U,5120U,6144U}) assert(f.Narrow(k,4096));
		for (unsigned word=0;word<sizeof(f.result)/4;word++) { auto r=f.result; reinterpret_cast<uint32_t*>(&r)[word]^=word==offsetof(tt_coordinated_fleet_result,staged_pages)/4?8:1;
			if (word!=10) assert(!PioneerLayerNarrowProof(r,6144,4096,119,f.coordinator.nonce)); }
		assert(!f.Narrow(1024,6144) && f.Close()); }
	for (unsigned fault=0;fault<12;fault++) {
		NarrowFixture f; assert(f.Prepare()); f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9; f.io.omitFusedPayload=fault==10; f.io.layerLost=fault==11;
		assert(!f.Narrow() && f.io.quarantined && f.transfer.quarantined && f.code.unsafe && !f.coordinator.inputValid
			&& f.result.master.recovery_required && !f.io.pioWords && f.io.C().pciCommand[0]==0x402 && f.io.C().pciCommand[1]==0x402);
		if (fault!=5 && fault!=9 && fault!=10) assert(!f.io.stops);
		unsigned writes=f.io.layerWrites; assert(!f.Close() && f.io.layerWrites==writes);
	}
	puts("PASS OFFLINE narrow controller,all K/N,complete output/cache proofs,all result mutations,unknown state => quarantine without reset/retry/restore");
}
