/* SPDX-License-Identifier: MIT */
// Launch/cleanup POLICY tests. The real arithmetic image is independently
// executed by pioneer_qwen38_panel_image_test; this mock models MMIO faults.
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38PanelProbe.h"
struct PanelIO:BanksMock {
	std::vector<uint32_t> view,saved;
	std::vector<uint32_t> resets;
	uint32_t entry=0x1250006f,coreReset=0x47800;
	unsigned panelWrites=0,dropWrite=0,control=0,launches=0,scenario=0;
	bool mapping=true,lateCounter=false;
	PanelIO():view(TT_Q38_PANEL_VIEW_WORDS),saved(TT_Q38_PANEL_VIEW_WORDS+1) {
		for (unsigned w=0;w<view.size();w++) view[w]=0xab860000+w;
	}
	bool Q38PanelMappingStable() { return mapping; }
	bool MapQ38Panel() { return mapping; }
	uint32_t* Q38PanelSavedWords() { return saved.data(); }
	uint32_t Q38PanelRead(uint32_t a) {
		assert(!(a&3) && a>=TT_Q38_PANEL_VIEW_BASE && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES);
		return view[(a-TT_Q38_PANEL_VIEW_BASE)/4];
	}
	void Q38PanelWrite(uint32_t a,uint32_t value) {
		assert(!(a&3) && a>=TT_Q38_PANEL_VIEW_BASE && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES);
		assert(coreReset==0x47800); if (++panelWrites!=dropWrite) view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=value;
	}
	uint32_t RuntimeCodeRead(unsigned w) { assert(w==0); return entry; }
	void RuntimeCodeWrite(unsigned w,uint32_t value) { assert(!w && coreReset==0x47800); entry=value; }
	uint32_t RuntimeControlRead(unsigned i) { assert(i<3); return i==0?control:0; }
	uint32_t ResetRead() { return coreReset; }
	uint32_t RuntimeStatusRead(unsigned w) { assert(w<3); return destination[2032+w]; }
	void RuntimeStatusWrite(unsigned w,uint32_t value) { assert(w<3); destination[2032+w]=value; }
	uint32_t RuntimePcRead() { return scenario==4?TT_Q38_LOCAL_PARK+4:TT_Q38_LOCAL_PARK; }
	uint32_t ResidentNocRead(uint32_t offset) {
		uint32_t value=StoreMock::ResidentNocRead(offset);
		return value+(lateCounter && offset==0x208?1:0);
	}
	void RuntimeResetWrite(uint32_t value) {
		assert(value==0x87ffff || value==0x46800 || value==0x47800); resets.push_back(value);
		if (value==0x87ffff && launches && scenario==9) return;
		if (value==0x46800 && scenario==1) { launches++; return; }
		coreReset=value;
		if (value!=0x46800) return;
		launches++;
		if (scenario==2) return; // timeout, NO successful completion
		if (scenario==3) { destination[2034]=TT_Q38_PANEL_FAULT_MARKER; return; }
		destination[2032]=TT_Q38_LOCAL_DONE;
		destination[2033]=PioneerQ38PanelExpectedCount(view[(0x74004-TT_Q38_PANEL_VIEW_BASE)/4]);
		destination[2034]=TT_Q38_PANEL_SUCCESS_MARKER;
		for (unsigned row=0;row<16;row++) {
			view[(0x84000-TT_Q38_PANEL_VIEW_BASE)/4+row]=(scenario==7 && !row)?
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
				0x7f800000:
#else
				0x80000000:
#endif
				row;
			view[(0x91200-TT_Q38_PANEL_VIEW_BASE)/4+row]=(scenario==8 && !row)?0x7f800000:0x3f800000;
			view[(0x91240-TT_Q38_PANEL_VIEW_BASE)/4+row]=0;
		}
		lateCounter=scenario==5;
		if (scenario==6) config^=1; // a source changed after start
		if (scenario==10) view[(0x72000-TT_Q38_PANEL_VIEW_BASE)/4]^=1;
		if (scenario==11) destination[2033]++;
		if (scenario==12) destination[2034]^=1;
	}
	void Pause() {}
};
struct PanelOwners {
	tt_dram_samples_result dram={}; tt_compute_noc_result noc={};
	tt_host_page_result host={}; PioneerResidentLease store;
	PioneerResidentBanks banks; PioneerQ38PanelLease panel;
	explicit PanelOwners(PanelIO& io) {
		dram=BankProof(io); noc=io.Compute(); assert(banks.Bind(dram)); assert(Prepared(io,host,store));
	}
};
static tt_q38_panel_probe Job(unsigned type=12,unsigned panel=0)
{
	tt_q38_panel_probe r={}; r.version=TT_Q38_PANEL_ABI_VERSION; r.type=type; r.bytes=PioneerQ38PanelProbeBytes(type); r.panel=panel;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	r.activation_scale=0xbc800000; //-.015625
#endif
	for (unsigned w=0;w<PioneerQ38PanelInputWords(type);w++) r.activation[w]=PioneerInt8Operand(-128)|(uint32_t(PioneerInt8Operand(127))<<16);
	return r;
}
static bool PreparePanel(PanelIO& io,PanelOwners& o)
{
	return PioneerQ38PanelPrepare(io,o.dram,o.noc,o.store,o.banks,o.panel);
}
static bool Run(PanelIO& io,PanelOwners& o,tt_q38_panel_probe& job)
{
	return PioneerQ38PanelRun(io,o.dram,o.noc,o.store,o.banks,o.panel,job);
}
static bool ClosePanel(PanelIO& io,PanelOwners& o,bool clean=true)
{
	return PioneerQ38PanelClose(io,o.dram,o.noc,o.store,o.banks,o.panel,clean);
}
static void Finish(PanelIO& io,PanelOwners& o)
{
	assert(PioneerResidentLeaseEnd(io,o.noc,o.store) && io.ReleaseHostPage()==0); io.Safe();
}
#ifndef PIONEER_Q38_PANEL_POLICY_MOCK_ONLY
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	for (unsigned type:{8U,12U,14U}) for (unsigned panel=0;panel<
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
		1;
#else
		(type==8?2U:16U);
#endif
		panel++) {
		PanelIO io; auto before=io.view; uint32_t entry=io.entry; PanelOwners own(io); auto job=Job(type,panel);
		assert(PioneerQ38PanelProbeValid(job) && PreparePanel(io,own) && Run(io,own,job));
		assert(job.result.started && job.result.completed && job.result.stopped && !job.result.recovery_required);
		assert(io.launches==1 && io.resets==std::vector<uint32_t>({0x87ffff,0x46800,0x87ffff,0x47800}));
		assert(PreparePanel(io,own)); // same immutable lease, no second backup
		assert(Run(io,own,job) && io.launches==2 && ClosePanel(io,own));
		assert(io.view==before && io.entry==entry && !io.requests); Finish(io,own);
	}
	for (unsigned scenario=1;scenario<=12;scenario++) {
		PanelIO io; PanelOwners own(io); auto job=Job(); assert(PreparePanel(io,own)); io.scenario=scenario;
		assert(!Run(io,own,job) && io.launches==1);
		bool knownBad=scenario==7 || scenario==8;
		if (knownBad) {
			assert(job.result.completed && job.result.stopped && !job.result.recovery_required && !own.panel.unsafe);
			assert(job.result.state==TT_RUNTIME_NONFINITE && ClosePanel(io,own)); Finish(io,own);
		} else {
			assert(job.result.recovery_required && own.panel.unsafe && own.store.recovery_required);
			unsigned writes=io.panelWrites,resets=unsigned(io.resets.size());
			assert(!Run(io,own,job) && !ClosePanel(io,own) && !ClosePanel(io,own,false));
			assert(writes==io.panelWrites && resets==io.resets.size() && io.launches==1 && !io.PageMock::releases);
			assert(resets==(scenario==9?3U:2U)); // NO extra reset after UNKNOWN
		}
	}
	for (unsigned fault=0;fault<11;fault++) {
		auto job=Job();
		if (fault==0) job.version=TT_Q38_PANEL_ABI_VERSION+1;
		if (fault==1) job.type=0;
		if (fault==2) job.bytes--;
		if (fault==3) job.panel=1U<<4;
		if (fault==4) job.reserved[0]=1;
		if (fault==5) job.reserved[1]=1;
		if (fault==6) job.activation[7]=0;
		if (fault==7) job.tile[4]=0x7c000000; // high scale byte of row3
		if (fault==8) job.tile[12]=0x7c000000; // Q4 minimum high byte row3
		if (fault==9) { job=Job(8); job.tile[848-1]=1; }
		if (fault==10) { job=Job(14); job.tile[3360/4]=1; }
		assert(!PioneerQ38PanelProbeValid(job));
		PanelIO io; PanelOwners own(io); assert(PreparePanel(io,own)); unsigned writes=io.panelWrites;
		assert(!Run(io,own,job) && !io.launches && writes==io.panelWrites && ClosePanel(io,own)); Finish(io,own);
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	for (uint32_t scale:{0x7f800000U,0xff800000U,0x7fc00001U,1U,0x2f7fffffU,0x47800001U}) {
		auto job=Job(); job.activation_scale=scale; assert(!PioneerQ38PanelProbeValid(job));
		PanelIO io; PanelOwners own(io); assert(PreparePanel(io,own)); unsigned writes=io.panelWrites;
		assert(!Run(io,own,job) && !io.launches && writes==io.panelWrites && ClosePanel(io,own)); Finish(io,own);
	}
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
	// Every additional vector word is validated BEFORE card writes. Q8
	// unused tail must be zero,not silently treated as another operand.
	for (unsigned type:{8U,12U,14U}) for (unsigned w=0;w<TT_Q38_PANEL_INPUT_WORDS;w++) {
		auto job=Job(type); job.activation[w]=w<PioneerQ38PanelInputWords(type)?0:1;
		assert(!PioneerQ38PanelProbeValid(job));
		PanelIO io; PanelOwners own(io); assert(PreparePanel(io,own)); unsigned writes=io.panelWrites;
		assert(!Run(io,own,job) && !io.launches && writes==io.panelWrites && ClosePanel(io,own)); Finish(io,own);
	}
#endif
	// Lost preparation/stage/restore writes never become accepted execution.
	{
		PanelIO io; auto before=io.view; PanelOwners own(io); io.dropWrite=1;
		assert(!PreparePanel(io,own) && !io.launches && ClosePanel(io,own)); assert(io.view==before); Finish(io,own);
	}
	{
		PanelIO io; PanelOwners own(io); assert(PreparePanel(io,own)); auto job=Job(); io.dropWrite=io.panelWrites+2;
		assert(!Run(io,own,job) && !io.launches && ClosePanel(io,own)); Finish(io,own);
	}
	{
		PanelIO io; PanelOwners own(io); assert(PreparePanel(io,own)); auto job=Job(); assert(Run(io,own,job));
		io.dropWrite=io.panelWrites+(0x70000-TT_Q38_PANEL_VIEW_BASE)/4+1;
		assert(!ClosePanel(io,own) && own.panel.unsafe && own.store.recovery_required && !io.PageMock::releases);
	}
	printf("PASS: OFFLINE driver%u local panel/block launch policy/reuse/exact L1+entry restoration,zero NoC,bounds+finite+padding guards,lost writes and ALL12 start/completion/counter/source/result/stop/code/count/marker faults;NO reset/restore/free after UNKNOWN\n",unsigned(TT_Q38_PANEL_CURRENT_DRIVER));
}
#endif
