/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#include "../PioneerQwen35DataflowStreamRecorder.h"
#include "../PioneerQwen35Roles.h"
#include <cassert>
#include <cstdio>
#include <memory>

using namespace PioneerQwen35;
struct Packed {
	bool Workspace(unsigned bank,uint32_t& address,uint32_t& bytes) const {
		if (bank>=Banks) return false;address=0x90000000;bytes=64*1024*1024;return true;
	}
};
struct Fixture {
	Packed packed;
	DeviceLayout manifest{};
	DataflowGraphWorkspace workspace{};
	Range rope{};
	unsigned routes[2][Banks]{};
	uint32_t cursor[Banks]{};
	Fixture() {
		manifest.magic=LayoutMagic;manifest.version=LayoutVersion;manifest.context=32;
		for (unsigned b=0;b<Banks;b++) cursor[b]=BankBase;
		for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) routes[n][b]=1+n*16+b;
		for (unsigned layer=0;layer<=Layers;layer++) for (unsigned role=0;;role++) {
			const auto* shape=PioneerQ35Role(layer,role);if (!shape) break;
			TensorSpace* t=layer==Layers?(role==TiedOutput?&manifest.tied:&manifest.finalNorm)
				:&manifest.layers[layer].weights[role];
			t->type=shape->type;t->inputs=shape->inputs;
			const unsigned unit=t->type?16:1;
			const unsigned bytes=t->type?(t->inputs/(t->type==8?32U:256U))
				*(PioneerQwen35Projection::TileBytes(t->type)/16):t->inputs*4;
			Rows(t->place,shape->outputs,bytes,unit);
		}
		for (unsigned layer=0;layer<Layers;layer++) {
			if (Dense(layer)) Rows(manifest.layers[layer].kv,2*DenseKeyHeads,
				manifest.context*DenseHeadWidth*2,1);
			else for (unsigned slot=0;slot<2;slot++) {
				Rows(manifest.layers[layer].delta[slot],DeltaHeads,DeltaStateWords*4,1);
				Rows(manifest.layers[layer].conv[slot],ConvChannels,3*4,1);
			}
		}
		rope={0,0x90000000,manifest.context*Rotary*4};
		PioneerQwen35ActivationPlan activations;
		assert(activations.InitializeFromPacked(packed,(rope.bytes+4095)&~4095U));
		for (unsigned i=0;i<ActivationBuffers;i++) {
			PioneerQ35ActivationBuffer b{};
			assert(activations.AllocateRows(Ffn,4,b) && DataflowActivationBufferToRows(b,manifest.activation[i]));
		}
		assert(DataflowGraphWorkspaceBuilder::Build(packed,manifest,workspace));
	}
	void Rows(RowSpace& space,unsigned rows,unsigned rowBytes,unsigned unit) {
		space={};space.rows=rows;space.rowBytes=rowBytes;space.unit=unit;
		const unsigned stripeUnit=(unit==1 && rows>=16 && !(rows&15))?16:unit;
		const unsigned groups=rows/stripeUnit;unsigned first=0;
		for (unsigned bank=0;bank<Banks;bank++) {
			const unsigned part=(groups/Banks+unsigned(bank<groups%Banks))*stripeUnit;
			if (!part) continue;
			space.parts[space.count++]={bank,cursor[bank],first,part};first+=part;
			cursor[bank]+=(part*rowBytes+4095)&~4095U;
			assert(cursor[bank]<0x80000000);
		}
		assert(space.Valid());
	}
};
struct Sink {
	unsigned writes=0,aborts=0;int failAt=-1;bool began=false,sealed=false,failBegin=false,failSeal=false;
	unsigned kinds[9]{};bool coreChecked[9]{};
	bool Begin(const DataflowStreamPlan&) { began=true;return !failBegin; }
	bool WriteAndVerify(const DataflowStreamBinding& b,const Range& lease,unsigned index,const DataflowStreamRecord& record) {
		if (int(writes++)==failAt) return false;
		assert(b.core==(writes-1)%DataflowCoreCount && index==(writes-1)/DataflowCoreCount);
		if (lease.bank!=b.bank || !DataflowStreamRecordStructureValid(record,b.epoch,index+1,b.core)) {
			std::fprintf(stderr,"invalid stream index=%u core=%u magic=%08x payload=%08x,%08x,%08x header=%08x,%08x,%08x\n",index,b.core,record.header[5],record.payload[0],record.payload[1],record.payload[2],record.header[2],record.header[3],record.header[4]);
			assert(false);
		}
		const unsigned magic=record.header[5];
		const unsigned types[]={DataflowEmbeddingMagic,DataflowProgramMagic,DataflowRmsMagic,
			DataflowSiluMagic,DataflowResidualMagic,DataflowConvMagic,DataflowDeltaMagic,
			DataflowDenseAttentionMagic,DataflowArgmaxMagic};
		bool found=false;for (unsigned i=0;i<9;i++) if (magic==types[i]) {
			kinds[i]++;found=true;
			if (!coreChecked[i]) {
				DataflowStreamRecord forged=record;
				forged.payload[DataflowStreamCoreWord(magic)]^=1;
				forged.header[7]=DataflowStreamChecksum(forged.payload,forged.header[6]/4);
				assert(!DataflowStreamRecordStructureValid(forged,b.epoch,index+1,b.core));
				coreChecked[i]=true;
			}
		}
		assert(found);return true;
	}
	bool Seal(const DataflowStreamPlan&) { sealed=true;return !failSeal; }
	void Abort() { aborts++; }
};
int main() {
	auto f=std::make_unique<Fixture>();
	auto live=std::make_unique<DataflowProjectionAuthority>();
	assert(live->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
	auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	auto candidate=std::make_unique<DataflowProjectionAuthority>();
	auto scratch=std::make_unique<DataflowStreamRecord>();
	auto projection=std::make_unique<DataflowProjectionProgram>();
	Sink sink;unsigned stage=~0U;
	const bool built=DataflowStreamRecording::Build(f->packed,*live,*candidate,*plan,f->routes,0,42,0,true,
		sink,*scratch,*projection,stage);
	if (!built) std::fprintf(stderr,"recording failed stage=%u writes=%u begin=%u seal=%u abort=%u\n",
		stage,sink.writes,unsigned(sink.began),unsigned(sink.sealed),sink.aborts);
	assert(built);
	assert(stage==DataflowDecoderSequenceComplete && sink.began && sink.sealed && !sink.aborts
		&& sink.writes==548*DataflowCoreCount);
	for (unsigned i=0;i<9;i++) assert(sink.kinds[i] && sink.coreChecked[i]);
	assert(!live->ActivationReady(0) && live->ConvNextPosition(0)==0 && live->DeltaNextPosition(0)==0);
	assert(candidate->ActivationReady(0) && candidate->ConvNextPosition(0)==1 && candidate->DeltaNextPosition(0)==1);
	// Continuing from the speculative copy exercises position-dependent state;
	// this is only a planning test and does not promote it to the live owner.
	auto next=std::make_unique<DataflowProjectionAuthority>();Sink nextSink;
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,100,548,*plan));
	assert(DataflowStreamRecording::Build(f->packed,*candidate,*next,*plan,f->routes,1,43,0,true,
		nextSink,*scratch,*projection,stage));
	assert(next->ConvNextPosition(0)==2 && candidate->ConvNextPosition(0)==1
		&& live->ConvNextPosition(0)==0 && nextSink.writes==548*DataflowCoreCount);
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	auto failed=std::make_unique<DataflowProjectionAuthority>();Sink lost;lost.failAt=121;
	assert(!DataflowStreamRecording::Build(f->packed,*live,*failed,*plan,f->routes,0,42,0,true,
		lost,*scratch,*projection,stage));
	assert(lost.aborts==1 && !lost.sealed && !live->ActivationReady(0) && live->ConvNextPosition(0)==0);
	auto beginFailure=std::make_unique<DataflowProjectionAuthority>();Sink badBegin;badBegin.failBegin=true;
	assert(!DataflowStreamRecording::Build(f->packed,*live,*beginFailure,*plan,f->routes,0,42,0,true,
		badBegin,*scratch,*projection,stage));
	assert(badBegin.began && badBegin.aborts==1 && !badBegin.writes);
	auto sealFailure=std::make_unique<DataflowProjectionAuthority>();Sink badSeal;badSeal.failSeal=true;
	assert(!DataflowStreamRecording::Build(f->packed,*live,*sealFailure,*plan,f->routes,0,42,0,true,
		badSeal,*scratch,*projection,stage));
	assert(badSeal.sealed && badSeal.aborts==1 && badSeal.writes==548*DataflowCoreCount
		&& !live->ActivationReady(0));
	auto invalidPosition=std::make_unique<DataflowProjectionAuthority>();Sink noWrite;
	assert(!DataflowStreamRecording::Build(f->packed,*live,*invalidPosition,*plan,f->routes,f->manifest.context,42,0,true,
		noWrite,*scratch,*projection,stage));
	assert(!noWrite.began && !invalidPosition->Bound());
	auto rejected=std::make_unique<DataflowProjectionAuthority>();Sink rejectedSink;
	plan->binding[3].base+=4096;
	assert(!DataflowStreamRecording::Build(f->packed,*live,*rejected,*plan,f->routes,0,42,0,true,
		rejectedSink,*scratch,*projection,stage));
	assert(!rejectedSink.began && !rejected->Bound());
	puts("PASS speculative model-authorized stream: 548 steps, 120 cores, isolated live state and aborted writes; SIMULATED ONLY");
}
