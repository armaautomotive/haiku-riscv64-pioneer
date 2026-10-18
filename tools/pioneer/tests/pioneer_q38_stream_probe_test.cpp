/* SPDX-License-Identifier: MIT */
// Real fullK prepare/run/restore policy;synthetic MMIO fabric ONLY.
#define TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
#define TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
#define TT_PIONEER_EXPERIMENTAL_Q38_STREAM
#define PIONEER_Q38_PANEL_POLICY_MOCK_ONLY
#include "pioneer_q38_panel_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StreamProbe.h"
struct StreamIO:PanelIO {
	void RuntimeResetWrite(uint32_t v) {
		PanelIO::RuntimeResetWrite(v);
		if (v!=0x46800 || scenario==1 || scenario==2) return;
		if (view[(0x74000-TT_Q38_PANEL_VIEW_BASE)/4]!=0x31533851) return;
		if (scenario==3) { destination[2034]=TT_Q38_STREAM_FAULT; return; }
		unsigned type=view[(0x74004-TT_Q38_PANEL_VIEW_BASE)/4],inputs=view[(0x74014-TT_Q38_PANEL_VIEW_BASE)/4];
		unsigned blocks=inputs/PioneerQ38StreamBlockInputs(type),bytes=PioneerQ38PanelProbeBytes(type);
		assert(command[0]==view[(0x74018-TT_Q38_PANEL_VIEW_BASE)/4] && !command[1] && command[3]==0x90000
			&& !command[4] && command[5]==(1|(2<<6)) && !command[6] && !command[7] && command[8]==bytes && !command[9]);
		destination[2032]=TT_Q38_STREAM_DONE; destination[2033]=inputs/16+(scenario==11);
		destination[2034]=TT_Q38_STREAM_MARKER^(scenario==12);
		counters[0]+=blocks; counters[1]+=blocks*(bytes/64); accepted+=blocks; counters[3]+=blocks;
		command[0]+=(blocks-1)*bytes;
		if (scenario==8) view[(0x84000-TT_Q38_PANEL_VIEW_BASE)/4]=0x7f800000;
		if (scenario==13) command[2]^=1;
		if (scenario==14) counters[1]++;
		if (scenario==15) writeCounters[0]++;
	}
};
static tt_q38_stream_probe StreamJob(unsigned type=12,unsigned inputs=5120,unsigned bank=0)
{
	tt_q38_stream_probe r={}; r.version=1; r.type=type; r.inputs=inputs; r.bank=bank;
	for (unsigned w=0;w<inputs/2;w++) r.activation[w]=PioneerInt8Operand(-128)|(uint32_t(PioneerInt8Operand(127))<<16);
	for (unsigned b=0;b<inputs/PioneerQ38StreamBlockInputs(type);b++) r.scales[b]=0x3c800000;
	return r;
}
static bool PrepareStream(StreamIO& io,PanelOwners& o)
{
	return PioneerQ38StreamPrepare(io,o.dram,o.noc,o.store,o.banks,o.panel);
}
static bool Stream(StreamIO& io,PanelOwners& o,const std::vector<uint8_t>& initialized,tt_q38_stream_probe& job)
{
	return PioneerQ38StreamRun(io,o.dram,o.noc,o.store,o.banks,o.panel,initialized.data(),unsigned(initialized.size()),job);
}
#ifndef PIONEER_Q38_STREAM_POLICY_MOCK_ONLY
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	(void)&PreparePanel; (void)&Run; (void)&Job;
	std::vector<uint8_t> initialized(PioneerResidentBanks::Pages,1);
	for (unsigned type:{8U,12U,14U}) for (unsigned inputs:{5120U,6144U,17408U}) for (unsigned bank=0;bank<7;bank++) {
		StreamIO io; auto before=io.view; auto entry=io.entry; PanelOwners own(io); auto job=StreamJob(type,inputs,bank);
		assert(PrepareStream(io,own) && Stream(io,own,initialized,job) && job.result.stopped && !job.result.recovery_required);
		assert(job.result.panels==inputs/16 && job.result.marker==TT_Q38_STREAM_MARKER);
		assert(PrepareStream(io,own) && Stream(io,own,initialized,job) && io.launches==2);
		assert(ClosePanel(io,own) && io.view==before && io.entry==entry); Finish(io,own);
	}
	for (unsigned scenario=1;scenario<=15;scenario++) {
		StreamIO io; PanelOwners own(io); auto job=StreamJob(); assert(PrepareStream(io,own)); io.scenario=scenario;
		assert(!Stream(io,own,initialized,job) && io.launches==1);
		bool knownBad=scenario==7 || scenario==8;
		if (knownBad) {
			assert(job.result.completed && job.result.stopped && !job.result.recovery_required && ClosePanel(io,own)); Finish(io,own);
		} else {
			assert(job.result.recovery_required && own.panel.unsafe && own.store.recovery_required);
			unsigned writes=io.panelWrites,resets=unsigned(io.resets.size());
			assert(!Stream(io,own,initialized,job) && !ClosePanel(io,own));
			assert(writes==io.panelWrites && resets==io.resets.size() && !io.PageMock::releases);
		}
	}
	for (unsigned missing:{0U,5U,11U}) {
		StreamIO io; PanelOwners own(io); auto job=StreamJob(); assert(PrepareStream(io,own)); auto writes=io.panelWrites;
		initialized[missing]=0; assert(!Stream(io,own,initialized,job) && !io.launches && io.panelWrites==writes);
		initialized[missing]=1; assert(ClosePanel(io,own)); Finish(io,own);
	}
	puts("PASS OFFLINE fullK REAL launch policy:ALL63 type/width/private-bank shapes,reuse/exact264KiB+entry restore,exact read/shared-accepted and zero write counts,ALL15 unknown/result/command/flit/write faults and missing extent refusal;NO retry/reset/restore/free after UNKNOWN");
}
#endif
