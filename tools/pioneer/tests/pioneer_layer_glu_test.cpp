/* SPDX-License-Identifier: MIT */
#define PIONEER_LAYER_CONTROLLER_MOCK_ONLY
#include "pioneer_layer_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerLayerGlu.h"
struct GluIO:LayerIO {
	unsigned pioStores=0,dropPio=0;
	uint32_t RuntimeBufferRead(unsigned w) { assert(!current && C().reset==0x47800 && !quarantined); return C().RuntimeBufferRead(w); }
	void RuntimeBufferWrite(unsigned w,uint32_t value) { assert(!current && C().reset==0x47800 && !quarantined); if (++pioStores!=dropPio) C().RuntimeBufferWrite(w,value); }
	void RuntimeResetWrite(uint32_t value) {
		if (value!=0x46800) { LayerIO::RuntimeResetWrite(value); return; }
		assert(!current && !quarantined && C().reset==0x87ffff && C().code[0]==TT_LAYER_BOOT && data[1]==5 && layer->running);
		for (unsigned i=0;i<8192;i++) assert(layerProgram[i]==BlackholeLayerCodeWord(i));
		unsigned n=data[4]; assert(n && n<=3072 && !(n&31) && !data[3] && !data[5] && data[2]==1 && !data[10] && !data[11]);
		assert(C().pciCommand[0]==0x406 && C().pciCommand[1]==0x406 && data[7]==uint32_t(physical) && data[8]==uint32_t(physical>>32) && data[9]==16384);
		for (unsigned w=1;w<120;w++) assert(!fleet[w].Compute().running && fleet[w].persistentIdle);
		C().reset=value; starts++; if (executeTimeout) return;
		const unsigned delta[]={1,n/16,2,1,1,n/16,2,1};
		data[3072]=data[3074]=data[3076]=1; data[3073]=data[3075]=n/16;
		data[3077]=n; data[3078]=data[6]-(staleSummary?1:0); data[3079]=5;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++; if (badSummary) data[3088]++;
		if (!omitFusedPayload) for (unsigned i=0;i<n;i++) OutputBufferMock::data[i]=nonfinite && !i?0x7f800000:0x40000000;
		status[0]=TT_LAYER_DONE; status[1]=1+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
	}
};
using GluFixture=LayerFixtureBase<GluIO>;
static bool Glu(GluFixture& f,unsigned n=3072)
{
	std::vector<uint32_t> gate(n,0x3f800000),up(n,0x40000000),output(n,0xabcdef01);
	bool passed=PioneerLayerGlu(f.io,f.io.core[0]->Baseline(),f.host,f.transfer,f.fleet.data(),f.bulk,f.coordinator,f.code,false,
		n,gate.data(),up.data(),output.data(),f.result);
	if (passed) {
		assert(PioneerLayerGluProof(f.result,n,f.coordinator.nonce) && !f.transfer.pending && !f.code.running && !f.coordinator.inputValid);
		for (auto value:output) assert(value==0x40000000);
		for (unsigned w=1;w<120;w++) assert(f.fleet[w].persistentIdle && !f.fleet[w].persistentSequence);
	}
	return passed;
}
int main()
{
	(void)Run; (void)Input;
	{
		GluFixture f; assert(f.Prepare());
		for (unsigned n:{32U,64U,1024U,2048U,3072U}) assert(Glu(f,n));
		for (unsigned word=0;word<sizeof(f.result)/4;word++) {
			auto r=f.result; reinterpret_cast<uint32_t*>(&r)[word]^=1;
			if (word!=10) assert(!PioneerLayerGluProof(r,3072,f.coordinator.nonce));
		}
		assert(f.io.starts==f.io.stops && !f.io.inputReads && f.Close());
	}
	for (unsigned fault=0;fault<12;fault++) {
		GluFixture f; assert(f.Prepare());
		f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9; f.io.omitFusedPayload=fault==10; f.io.layerLost=fault==11;
		assert(!Glu(f) && f.io.quarantined && f.transfer.quarantined && f.code.unsafe && f.result.master.recovery_required);
		if (fault!=5 && fault!=9 && fault!=10) assert(!f.io.stops);
		unsigned writes=f.io.layerWrites; assert(!f.Close() && f.io.layerWrites==writes);
	}
	for (unsigned word:{1U,2U,3071U,3072U,6143U,6144U}) {
		GluFixture f; assert(f.Prepare()); f.io.dropPio=word; assert(!Glu(f) && !f.io.starts && !f.result.master.recovery_required); assert(f.Close());
	}
	puts("PASS OFFLINE separately scheduled GPU GLU,all bounded shapes,full input readback,proof mutations,source/map/host/timeout faults,no reset/code restore after unknown completion");
}
