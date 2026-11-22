/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>

#define PIONEER_Q35_OPERATOR_TEST 1
#include "../PioneerQwen35Operators.h"
#include "../PioneerQwen35DataflowBlackholeRmsWorker.h"

using namespace PioneerQwen35;

static const unsigned TestBankRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

static void Rows(RowSpace& space,unsigned rows,uint32_t base)
{
	space={};
	space.rows=rows;
	space.rowBytes=sizeof(float);
	space.count=Banks;
	space.unit=16;
	const unsigned groups=rows/16;
	const unsigned baseGroups=groups/Banks,extra=groups%Banks;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=baseGroups+unsigned(bank<extra);
		space.parts[bank]={bank,base+bank*0x01000000,first*16,count*16};
		first+=count;
	}
}

static DeviceLayout MakeLayout()
{
	DeviceLayout model{};
	model.magic=LayoutMagic;
	model.version=LayoutVersion;
	model.context=64;
	TensorSpace& norm=model.layers[0].weights[AttentionNorm];
	norm.type=0;
	norm.inputs=Hidden;
	norm.place.rows=1;
	norm.place.rowBytes=Hidden*sizeof(float);
	norm.place.count=1;
	norm.place.unit=1;
	norm.place.parts[0]={1,BankBase+0x10000000,0,1};
	Rows(model.activation[0],Ffn,BankBase+0x20000000);
	Rows(model.activation[1],Ffn,BankBase+0x30000000);
	return model;
}

struct MockRmsIO {
	const DataflowRmsInvocation& invocation;
	const float* input;
	const float* weight;
	float* output;
	DataflowRmsScaleRecord record{};
	bool recordReady=false;
	unsigned failStage=0;

	bool ReadHiddenInput(float* destination) const
	{
		if (invocation.core!=0 || failStage==DataflowRmsWorkerHiddenRead) return false;
		memcpy(destination,input,Hidden*sizeof(float));
		return true;
	}
	bool ReadInputPanel(unsigned row,float* destination) const
	{
		if ((row&15) || row>=Hidden || failStage==DataflowRmsWorkerInputRead) return false;
		memcpy(destination,input+row,16*sizeof(float));
		return true;
	}
	bool ReadWeightPanel(unsigned row,float* destination) const
	{
		if ((row&15) || row>=Hidden || failStage==DataflowRmsWorkerWeightRead) return false;
		memcpy(destination,weight+row,16*sizeof(float));
		return true;
	}
	bool WriteScaleRecord(const DataflowRmsScaleRecord& value)
	{
		if (invocation.core!=0 || failStage==DataflowRmsWorkerRecordWrite || !value.Valid(1,invocation)) return false;
		record=value;
		recordReady=true;
		return true;
	}
	bool ReadScaleRecord(DataflowRmsScaleRecord& value) const
	{
		if (invocation.core==0 || !recordReady) return false;
		value=record;
		return true;
	}
	bool WriteOutput(unsigned bank,uint32_t address,const uint8_t* bytes,
		unsigned size) const
	{
		if (!bytes || size!=16*sizeof(float) || failStage==DataflowRmsWorkerOutputWrite) return false;
		for (unsigned i=0;i<invocation.segmentCount;i++) {
			const auto& segment=invocation.segment[i];
			if (segment.bank==bank && segment.address==address
				&& segment.bytes==size) {
				memcpy(output+segment.firstRow,bytes,size);
				return true;
			}
		}
		return false;
	}
};

struct MockScaleNoc {
	uint32_t* expectedPage;
	uint32_t remote[16]{};
	bool fail=false;
	bool Write(uint32_t address,unsigned route,const uint8_t* local,unsigned bytes,DataflowNocDiagnostic*) {
		assert(address==0x1000 && route==3 && local==reinterpret_cast<uint8_t*>(expectedPage)
			&& bytes==sizeof(remote));
		if (fail) return false;
		memcpy(remote,local,bytes);return true;
	}
	bool Read(uint32_t address,unsigned route,uint8_t* local,unsigned bytes,DataflowNocDiagnostic*) {
		assert(address==0x1000 && route==3 && local==reinterpret_cast<uint8_t*>(expectedPage)
			&& bytes==sizeof(remote));
		if (fail) return false;
		memcpy(local,remote,bytes);return true;
	}
};

int main()
{
	const DeviceLayout model=MakeLayout();
	DataflowRmsProgram program{};
	assert(DataflowRmsProgramBuilder::Build(model,0,AttentionNorm,0,1,
		TestBankRoutes,program));
	float input[Hidden],weight[Hidden],expected[Hidden],actual[Hidden]{};
	for (unsigned i=0;i<Hidden;i++) {
		input[i]=float(int(i%67)-33)/96.0f;
		weight[i]=0.75f+float(i%23)/128.0f;
	}
	assert(Rms<Hidden>(input,weight,expected));
	DataflowRmsScaleRecord barrier{};
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowRmsInvocation invocation{};
		assert(DataflowRmsProgramBuilder::BindCore(program,core,invocation));
		MockRmsIO io{invocation,input,weight,actual,barrier,core!=0};
		float inputCache[Hidden],inputPage[16],weightPage[16],outputPage[16];
		assert(PioneerQwen35DataflowRmsWorker(invocation,1,inputCache,inputPage,
			weightPage,outputPage,io));
		if (!core) barrier=io.record;
	}
	assert(memcmp(expected,actual,sizeof(expected))==0);
	alignas(64) uint32_t stagedScale[16]{};
	DataflowRmsScaleToPage(barrier,stagedScale);
	DataflowRmsScaleRecord stagedRoundTrip{};
	DataflowRmsScaleFromPage(stagedScale,stagedRoundTrip);
	assert(std::memcmp(&barrier,&stagedRoundTrip,sizeof(barrier))==0);
	MockScaleNoc noc{stagedScale};const Range scratch{0,0x1000,64};
	assert(DataflowRmsWriteScalePage(noc,scratch,3,barrier,stagedScale,nullptr));
	std::memset(stagedScale,0,sizeof(stagedScale));stagedRoundTrip={};
	assert(DataflowRmsReadScalePage(noc,scratch,3,stagedRoundTrip,stagedScale,nullptr)
		&& std::memcmp(&barrier,&stagedRoundTrip,sizeof(barrier))==0);
	noc.fail=true;stagedRoundTrip={};
	assert(!DataflowRmsReadScalePage(noc,scratch,3,stagedRoundTrip,stagedScale,nullptr)
		&& stagedRoundTrip.word[0]==0);
	static_assert(DataflowInputBufferAddress>=DataflowLocalBufferStart
		&& DataflowInputBufferAddress+sizeof(DataflowRmsScaleRecord)<=DataflowLocalBufferEnd,
		"scale staging belongs to the permitted NoC local range");
	DataflowRmsScaleRecord corrupt=barrier;
	corrupt.word[4]^=1;
	DataflowRmsInvocation invocation{};
	assert(DataflowRmsProgramBuilder::BindCore(program,0,invocation));
	MockRmsIO failed{invocation,input,weight,actual};
	float cache[Hidden],inputPage[16],weightPage[16],outputPage[16];
	unsigned stage=0;
	const unsigned faults[]={DataflowRmsWorkerHiddenRead,DataflowRmsWorkerRecordWrite,
		DataflowRmsWorkerInputRead,DataflowRmsWorkerWeightRead,DataflowRmsWorkerOutputWrite};
	for (unsigned fault:faults) {
		failed.failStage=fault;
		assert(!PioneerQwen35DataflowRmsWorker(invocation,1,cache,inputPage,weightPage,outputPage,failed,&stage)
			&& stage==fault);
	}
	assert(!corrupt.Valid(1,invocation));
	assert(!barrier.Valid(2,invocation));
	std::puts("PASS sharded Qwen35 device RMS matches the pinned hidden operator bitwise");
	return 0;
}
