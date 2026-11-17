/* SPDX-License-Identifier: MIT */
// Real Q8T4 host policy with a fake hardware endpoint, never card I/O.
#define PIONEER_Q38_STREAM_POLICY_MOCK_ONLY
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_stream_probe_test.cpp"
#include "../PioneerQ38TwoTokenQ4Probe.h"
#include <memory>

namespace Q=PioneerQ38TwoTokenQ4Probe;
namespace L=Q::L;
struct PinnedImage {
	enum : unsigned {BootWord=0x0006c06f,Bytes=7784,Marker=0x34543851};
	static uint32_t words[Bytes/4];
	static uint32_t Word(unsigned i) {assert(i<Bytes/4);return words[i];}
};
uint32_t PinnedImage::words[PinnedImage::Bytes/4]={};

struct PairIO:PanelIO {
	uint64_t ticks=10000;
	unsigned readsAfterLaunch=0,variant=0;
	bool unstableSave=false;
	unsigned tornRead=0;
	uint64_t Q38PairNowUs(){return variant==16?ticks:(ticks+=37);}
	static uint32_t Expected(unsigned row) {return 0x3f800000+row;}
	uint32_t Q38PanelRead(uint32_t at) {
		const uint32_t v=PanelIO::Q38PanelRead(at);
		if(unstableSave && at==L::VectorInput1)return v^(++tornRead==2);
		if(launches && coreReset==0x46800 && at>=L::Output && at<L::Output+128) {
			readsAfterLaunch++;
			if(variant==17 && readsAfterLaunch==32)counters[0]++;
		}
		return v;
	}
	void RuntimeResetWrite(uint32_t v) {
		// Check staging before any simulated endpoint output is installed.
		if(v==0x46800){
			const unsigned magic=Q38PanelRead(L::Descriptor),inputs=Q38PanelRead(L::Descriptor+20);
			assert(magic==L::VectorMagic && command[0]==Q38PanelRead(L::Descriptor+24));
			assert(command[2]==PioneerQ38ProjectionRoute(Q38PanelRead(L::Descriptor+28))
			 && command[3]==L::Tile && command[5]==L::Self && command[8]==L::TileBytes);
			for(unsigned i=0;i<32;i++)assert(Q38PanelRead(L::Output+i*4)==Q::Poison);
			for(unsigned t=0;t<2;t++){
				assert(Q38PanelRead(L::VectorInput(t))==Activation(t));
				assert(Q38PanelRead(L::VectorInput(t)+inputs*2-4)==Activation(t));
				assert(Q38PanelRead(L::Scale(t))==Scale(t));
			}
		}
		PanelIO::RuntimeResetWrite(v);
		if(v!=0x46800 || scenario==1 || scenario==2)return;
		const unsigned width=Q38PanelRead(L::Descriptor+20),blocks=width/256;
		destination[2032]=Q::Done;destination[2033]=width/16+(scenario==11);
		destination[2034]=scenario==3?L::VectorFault:L::VectorMagic^(scenario==12);
		for(unsigned i=0;i<32;i++)view[(L::Output-L::SavedBase)/4+i]=Expected(i);
		if(variant==8)view[(L::Output-L::SavedBase)/4+31]=0x7fc00000;
		counters[0]+=blocks;counters[1]+=blocks*36;accepted+=blocks;counters[3]+=blocks;
		command[0]+=(blocks-1)*L::TileBytes;
		if(variant==13)command[2]^=1;
		if(variant==14)counters[1]++;
		if(variant==15)writeCounters[0]++;
	}
	static uint32_t Activation(unsigned t) {
		return t?PioneerInt8Operand(55)|(uint32_t(PioneerInt8Operand(-73))<<16)
			:PioneerInt8Operand(-128)|(uint32_t(PioneerInt8Operand(127))<<16);
	}
	static uint32_t Scale(unsigned t){return t?0xbe000000:0x3c800000;}
};
static std::unique_ptr<Q::Job> PairJob(unsigned width=5120,unsigned bank=0) {
	auto job=std::make_unique<Q::Job>();job->inputs=width;job->bank=bank;
	for(unsigned t=0;t<2;t++){
		for(unsigned w=0;w<width/2;w++)job->activation[t][w]=PairIO::Activation(t);
		for(unsigned b=0;b<width/256;b++)job->scale[t][b]=PairIO::Scale(t);
	}
	return job;
}
static bool Prepare(PairIO& io,PanelOwners& o,Q::Lease& p){
	return Q::Prepare<PinnedImage>(io,o.dram,o.noc,o.store,o.banks,p);
}
static bool Run(PairIO& io,PanelOwners& o,Q::Lease& p,const std::vector<uint8_t>& pages,Q::Job& j){
	return Q::Run<PinnedImage>(io,o.dram,o.noc,o.store,o.banks,p,pages.data(),unsigned(pages.size()),j);
}
static bool Close(PairIO& io,PanelOwners& o,Q::Lease& p){
	return Q::Close(io,o.dram,o.noc,o.store,o.banks,p);
}
int main(int argc,char** argv) {
	assert(argc==2);(void)&Input;(void)&Upload;(void)&PrepareStream;(void)&Stream;(void)&StreamJob;
	(void)&PreparePanel;(void)&ClosePanel;
	auto* image=fopen(argv[1],"rb");
	assert(image && fread(PinnedImage::words,1,sizeof(PinnedImage::words),image)==sizeof(PinnedImage::words));
	assert(fgetc(image)==EOF);fclose(image);
	std::vector<uint8_t> initialized(PioneerResidentBanks::Pages,1);
	unsigned success=0,refused=0,unknown=0;
	for(unsigned width:{5120U,6144U,17408U})for(unsigned bank=0;bank<7;bank++){
		PairIO io; PanelOwners o(io); Q::Lease p; auto job=PairJob(width,bank);
		const auto before=io.view;const auto entry=io.entry;
		assert(Prepare(io,o,p) && !Prepare(io,o,p) && Run(io,o,p,initialized,*job));
		assert(job->result.stopped && !job->result.recovery_required && job->elapsed_us==37);
		assert(io.resets==std::vector<uint32_t>({0x87ffff,0x46800,0x87ffff,0x47800}));
		for(unsigned i=0;i<32;i++)assert(job->output[i]==PairIO::Expected(i));
		assert(Close(io,o,p) && io.view==before && io.entry==entry);Finish(io,o);success++;
	}
	for(unsigned which=0;which<13;which++){
		PairIO io;PanelOwners o(io);Q::Lease p;auto job=PairJob();
		if(which==0){
			io.unstableSave=true;const auto writes=io.panelWrites;
			assert(!Prepare(io,o,p) && !p.panel.saved && io.panelWrites==writes && io.resets.empty());
			Finish(io,o);refused++;continue;
		}
		assert(Prepare(io,o,p));
		unsigned missing=5;
		if(which==1)initialized[missing]=0;
		if(which==2)initialized[missing]=2;
		if(which==3)job->inputs=5136;
		if(which==4)job->bank=7;
		if(which==5)job->offset=0x800;
		if(which==6)job->offset=PioneerResidentBanks::Capacity-4096;
		if(which==7)job->activation[1][2559]=0;
		if(which==8)job->activation[1][2560]=PairIO::Activation(1);
		if(which==9)job->scale[1][19]=0x7f800000;
		if(which==10)job->scale[1][20]=0x80000000;
		if(which==11)io.dropWrite=io.panelWrites+16+20+2560+20+2560;
		if(which==12){
			// Even a forged "initialized" tag cannot turn a mutable page
			// in the middle of the weight extent into immutable weights.
			const PioneerResidentMutableRange range={0,missing,1};
			initialized[missing]=0;
			assert(o.banks.ReserveMutable(o.dram,&o,&range,1,initialized.data(),unsigned(initialized.size())));
			initialized[missing]=1;
		}
		const auto writes=io.panelWrites;
		assert(!Run(io,o,p,initialized,*job) && !io.launches && io.resets.empty() && !job->result.recovery_required);
		assert(io.panelWrites==writes || which==11);
		initialized[missing]=1;
		assert(Close(io,o,p));Finish(io,o);refused++;
	}
	for(unsigned s:{1U,2U,3U,4U,5U,6U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U}){
		PairIO io;PanelOwners o(io);Q::Lease p;auto job=PairJob();
		assert(Prepare(io,o,p));io.scenario=s;io.variant=s;
		assert(!Run(io,o,p,initialized,*job) && io.launches==1 && job->result.recovery_required
		 && !job->elapsed_us && !job->result.stopped && p.panel.unsafe && o.store.recovery_required);
		const auto writes=io.panelWrites,resets=unsigned(io.resets.size());
		for(auto result:job->output)assert(!result);
		assert(!Run(io,o,p,initialized,*job) && !Close(io,o,p));
		assert(writes==io.panelWrites && resets==io.resets.size() && !io.PageMock::releases);
		unknown++;
	}
	printf("PASS isolated Q8T4 host policy: %u bank/width exact restores, %u prelaunch refusals, "
	 "%u sticky failures; mocked hardware, not a live driver\n",success,refused,unknown);
}
