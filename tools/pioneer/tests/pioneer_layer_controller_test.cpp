/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATED_FLEET_MOCK_ONLY
#include "pioneer_coordinated_fleet_test.cpp"
struct LayerIO:CoordinatedIO {
	std::vector<uint32_t> layerProgram=std::vector<uint32_t>(8192),layerBackup=std::vector<uint32_t>(8192);
	unsigned layerWrites=0,dropLayer=0,readsAfterStop=0;
	bool layerLost=false,layerRestoreFails=false;
	PioneerLayerLease* layer=nullptr;
	LayerIO() { for (unsigned i=0;i<8192;i++) layerProgram[i]=0x12340000+i; }
	bool LayerMappingStable() { return !current && !(layerLost && starts); }
	uint32_t* LayerSavedWords() { return layerBackup.data(); }
	uint32_t LayerCodeRead(unsigned i) { assert(i<8192); return layerProgram[i]; }
	void LayerCodeWrite(unsigned i,uint32_t value) {
		assert(!current && !quarantined && C().reset==0x47800 && i<8192 && !layer->running);
		if (++layerWrites!=dropLayer && !(layerRestoreFails && value==0x12340000+i)) layerProgram[i]=value;
	}
	uint32_t RuntimePcRead() { assert(!current && C().reset==0x46800); return badPc?0x64000:TT_LAYER_PARK; }
	void RuntimeResetWrite(uint32_t value) {
		if (value!=0x46800) {
			if (C().reset==0x46800 && value==0x87ffff) assert(!layerLost);
			CoordinatorIO::RuntimeResetWrite(value); return;
		}
		assert(!current && !quarantined && C().reset==0x87ffff && C().code[0]==TT_LAYER_BOOT && data[1]==4 && layer->running);
		for (unsigned i=0;i<8192;i++) assert(layerProgram[i]==BlackholeLayerCodeWord(i));
		assert(C().pciCommand[0]==0x406 && C().pciCommand[1]==0x406 && data[3]==1024 && data[4]==1024
			&& data[7]==uint32_t(physical) && data[8]==uint32_t(physical>>32) && data[9]==16384);
		unsigned pg=std::min(data[2],96U),pd=std::min(data[2],32U),p=2*pg+pd;
		for (unsigned w=1;w<=pg;w++) assert(fleet[w].Compute().running && !fleet[w].persistentIdle
			&& fleet[w].persistentSequence==data[64+w-1]+(w<=pd?2:1));
		C().reset=value; starts++; if (executeTimeout) return;
		unsigned r=4*p+1,rf=r-p+511,w=2*p+1,wf=2*p+64;
		const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		data[3072]=r; data[3073]=rf; data[3074]=w; data[3075]=wf; data[3076]=pg;
		data[3077]=1024; data[3078]=data[6]-(staleSummary?1:0); data[3079]=4;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++; if (badSummary) data[3088]++;
		if (!omitFusedPayload) for (unsigned i=0;i<1024;i++) OutputBufferMock::data[i]=nonfinite && !i?0x7f800000:0x40000000;
		status[0]=TT_LAYER_DONE; status[1]=pg+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
	}
};
struct LayerFixture {
	LayerIO io; std::vector<PioneerFleetWorker> fleet=std::vector<PioneerFleetWorker>(120);
	PioneerBulkUploadLease bulk; PioneerOutputBufferLease transfer; PioneerCoordinatorLease coordinator;
	PioneerLayerLease code; tt_coordinated_fleet_result result={}; tt_host_page_result host={};
	LayerFixture() {
		io.layer=&code; io.compactMode=io.fusedMode=true; io.fleet=fleet.data();
		for (unsigned w=0;w<120;w++) {
			auto& c=*io.core[w]; auto& f=fleet[w]; assert(Prepared(c,host,f.Store())); f.noc=c.Compute();
			if (w) f.noc.version=PioneerWorkerVersion(w);
			f.Compute().ready=f.Compute().saved=f.Compute().bulk=true; f.Compute().buffer_words=TT_Q8_BULK_SAVED_BUFFER_WORDS;
			c.bufferWords=TT_Q8_BULK_SAVED_BUFFER_WORDS; f.preludeDirty=true; c.code[0]=0x0006b06f;
			if (w) f.coordinated=f.persistentPrepared=f.persistentActive=f.persistentIdle=true;
		}
		bulk.saved=bulk.ready=true; assert(PioneerPrepareOutputBuffer(io,host,transfer));
		assert(PioneerCoordinatorPrepare(io,io.core[0]->Baseline(),fleet[0].noc,fleet[0].Store(),coordinator,true,false,true,true));
	}
	bool Prepare() { return PioneerLayerPrepare(io,io.core[0]->Baseline(),fleet[0].noc,fleet[0].Store(),coordinator,code); }
	bool Close(bool safe=true) { return PioneerLayerClose(io,io.core[0]->Baseline(),fleet[0].noc,fleet[0].Store(),code,safe); }
	bool Run(unsigned peers=119,uint64_t up=0x800000,uint64_t down=0x1000000) {
		uint32_t input[512],scales[32],output[1024];
		for (auto& value:input) value=0x40014001; for (auto& value:scales) value=0x3f800000;
		for (auto& value:output) value=0xabcdef00;
		PioneerLayerJob job={&code,up,down};
		bool okay=PioneerCoordinatedMatvec(io,io.core[0]->Baseline(),host,transfer,fleet.data(),bulk,coordinator,false,
			UINT64_C(0x80000000),0,1024,1024,peers,input,scales,output,result,&job);
		if (okay) {
			assert(PioneerCoordinatedLayerProof(result,peers,coordinator.nonce) && !coordinator.inputValid && !code.running
				&& !transfer.pending && io.C().pciCommand[0]==0x402 && io.C().pciCommand[1]==0x402 && !io.pioWords);
			for (auto value:output) assert(value==0x40000000);
		}
		return okay;
	}
};
int main()
{
	(void)Run; (void)Input; // shared fixture helpers,not exercised by this graph test
	{
		LayerFixture f; assert(f.Prepare());
		for (unsigned workers:{1U,2U,32U,64U,119U,32U,119U}) assert(f.Run(workers));
		assert(f.io.starts==f.io.stops && f.io.inputReads==7);
		for (unsigned w=0;w<sizeof(f.result)/4;w++) {
			auto r=f.result; reinterpret_cast<uint32_t*>(&r)[w]^=1;
			if (w!=10) assert(!PioneerCoordinatedLayerProof(r,119,f.coordinator.nonce));
		}
		assert(f.Close()); for (unsigned i=0;i<8192;i++) assert(f.io.layerProgram[i]==0x12340000+i);
	}
	for (unsigned fault=0;fault<12;fault++) {
		LayerFixture f; assert(f.Prepare());
		f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9; f.io.omitFusedPayload=fault==10; f.io.layerLost=fault==11;
		assert(!f.Run() && f.io.quarantined && f.transfer.quarantined && f.code.unsafe && !f.coordinator.inputValid
			&& f.result.master.recovery_required && !f.io.pioWords && f.io.C().pciCommand[0]==0x402 && f.io.C().pciCommand[1]==0x402);
		if (fault!=5 && fault!=9 && fault!=10) assert(!f.io.stops);
		unsigned writes=f.io.layerWrites; assert(!f.Close() && f.io.layerWrites==writes);
	}
	for (unsigned i:{1U,2335U,8192U}) { LayerFixture f; f.io.dropLayer=i; assert(!f.Prepare() && f.code.saved && !f.io.starts); assert(f.Close()); }
	{ LayerFixture f; assert(f.Prepare()); f.io.layerRestoreFails=true; assert(!f.Close() && f.io.quarantined); }
	{ LayerFixture f; assert(!f.Run() && !f.io.starts); }
	{ LayerFixture f; assert(f.Prepare()); assert(!f.Run(119,0x7e3ff000) && !f.io.starts); assert(f.Close()); }
	{ LayerFixture f; assert(f.Prepare()); f.fleet[96].persistentSequence=0x7ffffffb; assert(!f.Run() && !f.io.starts); assert(f.Close()); }
	puts("PASS OFFLINE fixed FFN controller,independent code lease,all cohort sizes,sequences,full proof mutation,source/map/PCI/host/timeout faults; no reset/result/code restore on unknown completion");
}
