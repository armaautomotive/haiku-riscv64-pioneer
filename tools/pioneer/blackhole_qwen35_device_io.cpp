/* SPDX-License-Identifier: MIT */
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen35DeviceIO.h"
#include "PioneerQwen35Embedding.h"
#include "PioneerQwen35RootImageLayout.h"
#include "PioneerQwen35WorkSchedule.h"
#include "PioneerQ38ProjectionInput.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"

extern "C" unsigned q35_dispatch_matrix();

namespace PioneerQwen35 {
namespace {
enum : unsigned {
	Niu=0xffb20000, Bounce=RootBounceAddress, BounceBytes=RootBounceBytes,
	RootRoute=129, Activation=0xa0000, Scales=RootScaleAddress, MatrixJob=0x74000,
	RootMatrixJob=0x74f60, DispatchStateAddress=0x7fc00,
	MatrixResult=0x84000, RootProjectionResult=0x9e000
};
constexpr unsigned CounterOffset[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x228};

volatile uint32_t* Words(unsigned address) { return reinterpret_cast<volatile uint32_t*>(address); }
void Fence() { asm volatile("fence" ::: "memory"); }
unsigned Counter(unsigned i) { return *Words(Niu+CounterOffset[i]); }

bool Quiet()
{
	for (unsigned i=0;i<4;i++)
		if (*Words(Niu+0x40+i*0x800)) return false;
	for (unsigned i=0;i<32;i++)
		if (*Words(Niu+0x240+i*4)&255) return false;
	return true;
}

bool Program(const uint32_t* command)
{
	if (!Quiet() || (command[5]!=RootRoute && command[2]!=RootRoute))
		return false;
	for (unsigned i=0;i<10;i++) Words(Niu)[i]=command[i];
	Fence();
	for (unsigned i=0;i<10;i++)
		if (Words(Niu)[i]!=command[i]) return false;
	return true;
}

bool Matches(const uint32_t* command)
{
	if (!Quiet()) return false;
	for (unsigned i=0;i<10;i++)
		if (Words(Niu)[i]!=command[i]) return false;
	return true;
}

bool InBank(const Range& range)
{
	return range.bank<Banks && range.address>=BankBase && range.address<BankLimit
		&& range.bytes && range.bytes<=BankLimit-range.address && !((range.address|range.bytes)&63);
}

__UINT64_TYPE__ Cycles()
{
	for (unsigned i=0;i<16;i++) {
		uint32_t hi, lo, check;
		asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
			: "=&r"(hi),"=&r"(lo),"=&r"(check) :: "memory");
		if (hi==check) return (static_cast<__UINT64_TYPE__>(hi)<<32)|lo;
	}
	return 0;
}

bool RecordCycleTicks(volatile RequestProgress& progress,unsigned slot,__UINT64_TYPE__ cycles)
{
	if (slot<ProgressActivationPackTicksSlot || slot>ProgressMatrixDispatchTicksSlot)
		return false;
	const __UINT64_TYPE__ ticks=cycles>>8;
	if (ticks>0xffffffffU-progress.reserved[slot]) return false;
	progress.reserved[slot]+=unsigned(ticks);
	return true;
}
}

DeviceIO::DeviceIO(const DeviceLayout& layout,Range ropeTable,volatile RequestProgress& status,
	volatile uint32_t* generatedIds)
	: model(layout), rope(ropeTable), progress(status), tokens(generatedIds), started(Cycles())
{
}

void DeviceIO::Trace(unsigned stage)
{
	progress.reserved[ProgressReadyFailureSlot]=stage;
	Fence();
}

bool DeviceIO::Ready()
{
	const auto reject=[&](unsigned reason) {
		progress.reserved[ProgressReadyFailureSlot]=reason;
		Fence();
		return false;
	};
	if (!started) return reject(1);
	if (!tokens) return reject(2);
	if (model.magic!=LayoutMagic) return reject(3);
	if (model.version!=LayoutVersion) return reject(4);
	if (!model.context || model.context>MaxPositions) return reject(5);
	if (model.reserved) return reject(6);
	if (!InBank(rope)) return reject(7);
	if (rope.bytes!=model.context*Rotary*4) return reject(8);
	if (!Quiet()) return reject(9);
	if (*Words(Niu+0x14)!=RootRoute) return reject(10);
	return true;
}

bool DeviceIO::TransferWords(Range range,void* local,bool write)
{
	if (!InBank(range) || !local || (reinterpret_cast<__UINTPTR_TYPE__>(local)&3)) return false;
	auto* localBytes=static_cast<unsigned char*>(local);
	unsigned route=PioneerQ38ProjectionRoute(range.bank);
	for (unsigned offset=0;offset<range.bytes;) {
		unsigned bytes=range.bytes-offset;
		if (bytes>BounceBytes) bytes=BounceBytes;
		unsigned address=range.address+offset;
		auto* buffer=Words(Bounce);
		if (write)
			for (unsigned at=0;at<bytes;at+=4) {
				uint32_t bits;
				__builtin_memcpy(&bits,localBytes+offset+at,4);
				buffer[at/4]=bits;
			}
		const uint32_t command[]={write?Bounce:address,0,write?RootRoute:route,
			write?address:Bounce,0,write?route:RootRoute,0,write?0x212U:0,bytes,0};
		if (!Program(command)) return false;
		const unsigned increment[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,
			write?1U:0U,write?bytes/64:0U,write?1U:0U};
		uint32_t expected[7];
		for (unsigned i=0;i<7;i++) expected[i]=Counter(i)+increment[i];
		*Words(Niu+0x40)=1;
		(void)*Words(Niu+0x40);
		Fence();
		bool done=false;
		for (unsigned attempt=0;attempt<65536;attempt++) {
			done=!*Words(Niu+0x40);
			for (unsigned i=0;i<7;i++) done &= Counter(i)==expected[i];
			if (done) break;
		}
		if (!done || !Matches(command)) return false;
		Fence();
		if (!write)
			for (unsigned at=0;at<bytes;at+=4) {
				uint32_t bits=buffer[at/4];
				__builtin_memcpy(localBytes+offset+at,&bits,4);
			}
		offset+=bytes;
	}
	return true;
}

bool DeviceIO::Transfer(Range range,void* local) { return TransferWords(range,local,false); }
bool DeviceIO::Write(Range range,const void* local) { return TransferWords(range,const_cast<void*>(local),true); }

bool DeviceIO::Embed(unsigned token,float* hidden,unsigned values)
{
	Trace(0xd1000000U|(token&0x00ffffffU));
	namespace E=PioneerQwen35Embedding;
	Range range;
	unsigned lane;
	if (values!=Hidden || !model.ResolveEmbedding(token,range,lane) || range.bytes!=E::GroupBytes)
		return false;
	auto* tile=reinterpret_cast<unsigned char*>(0x90000);
	for (unsigned block=0;block<E::Blocks;block++)
		if (!Transfer({range.bank,range.address+block*E::TileBytes,E::TileBytes},tile)
			|| !E::DecodeBlock(tile,E::TileBytes,lane,hidden+block*E::BlockValues,E::BlockValues))
			return false;
	return true;
}

bool DeviceIO::ReadWeight(unsigned layer,unsigned role,unsigned offset,float* output,unsigned values)
{
	Trace(0xd2000000U|((layer&0xffU)<<16)|((role&0xffU)<<8)|((offset>>6)&0xffU));
	const auto* tensor=model.Weight(layer,role);
	if (!tensor || tensor->type || values>Hidden || offset>ConvChannels*4) return false;
	unsigned first=0, rows=1, byteOffset=offset*4;
	if (layer<Layers && !Dense(layer) && role==ConvWeights) {
		if ((offset&63) || values!=64) return false;
		first=offset>>2;
		rows=16;
		byteOffset=0;
	}
	Range range;
	return tensor->place.Resolve(first,rows,byteOffset,values*4,range) && Transfer(range,output);
}

bool DeviceIO::ReadRope(unsigned at,float* output,unsigned values)
{
	Trace(0xd3000000U|(at&0x00ffffffU));
	return at==position && position<model.context && values==Rotary
		&& Transfer({rope.bank,rope.address+position*Rotary*4,Rotary*4},output);
}

bool DeviceIO::ReadConv(unsigned layer,unsigned slot,unsigned channel,float* output,unsigned values)
{
	Trace(0xd4000000U|((layer&0xffU)<<16)|((slot&1U)<<15)|((channel>>4)&0x7fffU));
	Range range;
	return position && slot==(position&1) && values==48 && model.ResolveConv(layer,slot,channel,range)
		&& Transfer(range,output);
}

bool DeviceIO::WriteConv(unsigned layer,unsigned slot,unsigned channel,const float* input,unsigned values)
{
	Trace(0xd5000000U|((layer&0xffU)<<16)|((slot&1U)<<15)|((channel>>4)&0x7fffU));
	Range range;
	return slot==((position&1)^1) && values==48 && model.ResolveConv(layer,slot,channel,range)
		&& Write(range,input);
}

bool DeviceIO::ReadDelta(unsigned layer,unsigned slot,unsigned head,float* output,unsigned values)
{
	Trace(0xd6000000U|((layer&0xffU)<<16)|((slot&1U)<<15)|(head&0x7fffU));
	Range range;
	return position && slot==(position&1) && values==DeltaStateWords && model.ResolveDelta(layer,slot,head,range)
		&& Transfer(range,output);
}

bool DeviceIO::WriteDelta(unsigned layer,unsigned slot,unsigned head,const float* input,unsigned values)
{
	Trace(0xd7000000U|((layer&0xffU)<<16)|((slot&1U)<<15)|(head&0x7fffU));
	Range range;
	return slot==((position&1)^1) && values==DeltaStateWords && model.ResolveDelta(layer,slot,head,range)
		&& Write(range,input);
}

bool DeviceIO::ReadKv(unsigned layer,unsigned head,unsigned at,uint32_t* k,uint32_t* v,unsigned words)
{
	Trace(0xd8000000U|((layer&0xffU)<<16)|((head&0xffU)<<8)|(at&0xffU));
	Range key,value;
	return at<=position && words==DenseHeadWidth/2 && model.ResolveKv(layer,head,at,false,key)
		&& model.ResolveKv(layer,head,at,true,value) && Transfer(key,k) && Transfer(value,v);
}

bool DeviceIO::WriteKv(unsigned layer,unsigned head,unsigned at,const uint32_t* k,const uint32_t* v,unsigned words)
{
	Trace(0xd9000000U|((layer&0xffU)<<16)|((head&0xffU)<<8)|(at&0xffU));
	Range key,value;
	return at==position && words==DenseHeadWidth/2 && model.ResolveKv(layer,head,at,false,key)
		&& model.ResolveKv(layer,head,at,true,value) && Write(key,k) && Write(value,v);
}

bool DeviceIO::StageInput(unsigned type,const float* input,unsigned inputs)
{
	if (!PioneerQwen35Projection::Supports(type,inputs)) return false;
	unsigned scalesPerChunk=type==8?8:1;
	unsigned blocks=inputs>>(type==8?5:8),padded=(blocks+15)&~15U;
	if (padded>RootScaleMaxWords) return false;
	volatile uint32_t* evidence=Words(RootScaleEvidenceAddress);
	for (unsigned i=0;i<RootScaleEvidenceWords;i++) evidence[i]=0;
	for (unsigned chunk=0;chunk<inputs/256;chunk++) {
		PioneerQ38ProjectionInput packed;
		// Q5_K uses the same 256-lane Q8_K activation as Q4_K.
		if (!PioneerQ38ProjectionQuantize(type==13?12:type,input+chunk*256,256,&packed)) return false;
		for (unsigned w=0;w<128;w++) Words(Activation)[chunk*128+w]=packed.operand[w];
		for (unsigned w=0;w<scalesPerChunk;w++) Words(Scales)[chunk*scalesPerChunk+w]=packed.scale[w];
	}
	if (type==13 && blocks<padded) evidence[0]=Words(Scales)[blocks];
	for (unsigned block=blocks;block<padded;block++) Words(Scales)[block]=0;
	Fence();
	if (type==13 && blocks<padded) evidence[1]=Words(Scales)[blocks];
	return true;
}

bool DeviceIO::StageBand(const TensorSpace& tensor,float* output,float* best,unsigned* chosen)
{
	const TensorSpace* tensors[1]={&tensor};
	float* outputs[1]={output};
	return StageBands(tensors,1,outputs,best,chosen);
}

bool DeviceIO::StageBands(const TensorSpace* const* tensors,unsigned tensorCount,
	float* const* outputs,float* best,unsigned* chosen)
{
	auto fail=[&](unsigned reason) { Trace(0xe5000000U|reason); return false; };
	__UINT64_TYPE__ phaseStart=Cycles();
	if (!phaseStart) return fail(18);
	if (!tensors || !tensorCount || tensorCount>4 || !tensors[0] || (!!best!=!!chosen)
		|| ((best || chosen) && tensorCount!=1)) return fail(1);
	const bool argmax=best && chosen;
	if (!argmax && !outputs) return fail(1);
	const unsigned type=tensors[0]->type,inputs=tensors[0]->inputs;
	const unsigned tile=PioneerQwen35Projection::TileBytes(type);
	if (!tile || !PioneerQwen35Projection::Supports(type,inputs)) return fail(2);
	if (type==13) {
		Words(RootScaleEvidenceAddress)[3]=Words(Scales)[inputs>>8];
		Fence();
	}
	PioneerQ35WorkFragment fragments[PioneerQ35WorkMaxFragments]={};
	unsigned fragmentTensor[PioneerQ35WorkMaxFragments]={},tensorBaseGroup[4]={},tensorBaseRow[4]={};
	unsigned cursor[PioneerQ35WorkMaxFragments]={},fragmentCount=0,groups=0,totalRows=0;
	for (unsigned t=0;t<tensorCount;t++) {
		if (!tensors[t]) return fail(1);
		const TensorSpace& tensor=*tensors[t];
		if (!tensor.place.Valid() || tensor.type!=type || tensor.inputs!=inputs
			|| tensor.place.unit!=16 || (tensor.place.rows&15)) return fail(1);
		if (!argmax && !outputs[t]) return fail(1);
		if (groups>0xffffffffU-tensor.place.rows/16 || totalRows>0xffffffffU-tensor.place.rows)
			return fail(4);
		tensorBaseGroup[t]=groups;
		tensorBaseRow[t]=totalRows;
		PioneerQ35WorkFragment localParts[Banks]={};
		for (unsigned partIndex=0;partIndex<tensor.place.count;partIndex++) {
			const auto& part=tensor.place.parts[partIndex];
			if ((part.firstRow|part.rows)&15) return fail(3);
			localParts[partIndex]={part.firstRow/16,part.rows/16};
		}
		if (!PioneerQ35AppendWorkFragments(localParts,tensor.place.count,
			tensorBaseGroup[t],t,fragments,fragmentTensor,fragmentCount)) return fail(3);
		groups+=tensor.place.rows/16;
		totalRows+=tensor.place.rows;
	}
	if (type==13) {
		Words(RootScaleEvidenceAddress)[4]=Words(Scales)[inputs>>8];
		Fence();
	}
	if (type==13) {
		Words(RootScaleEvidenceAddress)[5]=Words(Scales)[inputs>>8];
		Fence();
	}
	unsigned expectedGroups=0;
	for (unsigned t=0;t<tensorCount;t++) expectedGroups+=tensors[t]->place.rows/16;
	if (!fragmentCount || !groups || groups!=expectedGroups || !totalRows) return fail(4);
	volatile uint32_t* state=Words(DispatchStateAddress);
	if (state[0]!=RootDispatchStateMagic || state[1]>=0x7ffffffe) return fail(5);
	if (type==13) {
		Words(RootScaleEvidenceAddress)[6]=Words(Scales)[inputs>>8];
		Fence();
	}
	unsigned completed=0;
	while (completed<groups) {
		if (type==13) {
			Words(RootScaleEvidenceAddress)[7]=Words(Scales)[inputs>>8];
			Fence();
		}
		PioneerQ35WorkBatch batch{};
		if (type==13) {
			Words(RootScaleEvidenceAddress)[8]=Words(Scales)[inputs>>8];
			Fence();
		}
		if (!PioneerQ35BuildWorkBatch(fragments,fragmentCount,cursor,batch)) return fail(6);
		if (type==13) {
			Words(RootScaleEvidenceAddress)[9]=Words(Scales)[inputs>>8];
			Fence();
		}
		const unsigned rootGroups=batch.rootGroups,workers=batch.workerCount;
		if (type==13) {
			Words(RootScaleEvidenceAddress)[10]=Words(Scales)[inputs>>8];
			Fence();
		}
		if (progress.reserved[ProgressProjectionWavesSlot]==0xffffffffU
			|| (workers==WorkTensixPeerCount && rootGroups==1
				&& progress.reserved[ProgressFullCoreWavesSlot]==0xffffffffU)) return fail(7);
		struct Task { unsigned firstGroup,groups; Range range; unsigned sequence; } tasks[119];
		for (unsigned worker=0;worker<119;worker++) tasks[worker]={};
		unsigned processed=rootGroups;
		for (unsigned worker=0;worker<workers;worker++) {
			const auto& scheduled=batch.worker[worker];
			if (scheduled.fragment>=fragmentCount) return fail(8);
			const unsigned tensorIndex=fragmentTensor[scheduled.fragment];
			if (tensorIndex>=tensorCount || scheduled.firstGroup<tensorBaseGroup[tensorIndex]) return fail(8);
			Range range;
			const unsigned localGroup=scheduled.firstGroup-tensorBaseGroup[tensorIndex];
			if (!tensors[tensorIndex]->ResolveBand(localGroup*16,scheduled.groups,range) || !InBank(range)) return fail(8);
			unsigned sequence=state[2+worker]+1;
			if (!sequence || sequence>=0x7ffffffe) return fail(9);
			tasks[worker]={scheduled.firstGroup,scheduled.groups,range,sequence};
			processed+=scheduled.groups;
		}
		if (!workers || processed!=batch.processedGroups || !processed || completed+processed>groups)
			return fail(10);
		const unsigned nonce=state[1]+1;
		if (!nonce || nonce>=0x7fffffff) return fail(11);
		volatile uint32_t* rootJob=Words(RootMatrixJob);
		for (unsigned i=0;i<16;i++) rootJob[i]=0;
		if (rootGroups) {
			if (batch.rootFragment>=fragmentCount) return fail(12);
			const unsigned tensorIndex=fragmentTensor[batch.rootFragment];
			if (tensorIndex>=tensorCount || batch.rootFirstGroup<tensorBaseGroup[tensorIndex]) return fail(12);
			Range range;
			const unsigned localGroup=batch.rootFirstGroup-tensorBaseGroup[tensorIndex];
			if (!tensors[tensorIndex]->ResolveBand(localGroup*16,rootGroups,range) || !InBank(range)) return fail(12);
			rootJob[0]=PioneerQwen35Projection::Magic; rootJob[1]=type;
			rootJob[2]=tile; rootJob[5]=inputs; rootJob[6]=range.address;
			rootJob[7]=rootGroups; rootJob[8]=PioneerQ38ProjectionRoute(range.bank); rootJob[9]=range.bank;
		}
		if (type==13) {
			Words(RootScaleEvidenceAddress)[11]=Words(Scales)[inputs>>8];
			Fence();
		}
		volatile uint32_t* table=Words(0x74080);
		for (unsigned worker=0;worker<119;worker++) for (unsigned word=0;word<8;word++)
			table[worker*8+word]=0;
		for (unsigned worker=0;worker<workers;worker++) {
			const auto& task=tasks[worker];
			const unsigned values[8]={task.range.address,task.range.bank,task.groups,task.sequence,
				PioneerQ38ProjectionRoute(task.range.bank),Words(0x7d800)[worker*16],0,0};
			if (!values[5]) return fail(13);
			for (unsigned word=0;word<8;word++) table[worker*8+word]=values[word];
		}
		if (type==13) {
			Words(RootScaleEvidenceAddress)[12]=Words(Scales)[inputs>>8];
			Fence();
		}
		volatile uint32_t* header=Words(MatrixJob);
		for (unsigned word=0;word<16;word++) header[word]=0;
		header[0]=0x314d3851; header[1]=1; header[2]=1; header[3]=type;
		header[4]=inputs; header[5]=workers; header[6]=nonce; header[8]=1;
		Fence();
		Trace(0xda000000U|workers);
		if (type==13) {
			Words(RootScaleEvidenceAddress)[13]=Words(Scales)[inputs>>8];
			Fence();
		}
		const __UINT64_TYPE__ dispatchStart=Cycles();
		if (dispatchStart<phaseStart || !RecordCycleTicks(progress,
			ProgressProjectionSetupTicksSlot,dispatchStart-phaseStart)) return fail(19);
		if (q35_dispatch_matrix()!=workers) return fail(14);
		const __UINT64_TYPE__ dispatchEnd=Cycles();
		if (dispatchEnd<dispatchStart || !RecordCycleTicks(progress,
			ProgressMatrixDispatchTicksSlot,dispatchEnd-dispatchStart)) return fail(20);
		phaseStart=dispatchEnd;
		if (rootGroups) {
			for (unsigned row=0;row<rootGroups*16;row++) {
				float value=PioneerAttentionFloat(Words(RootProjectionResult)[row]);
				if (!Finite(value)) return fail(15);
				const unsigned logicalRow=batch.rootFirstGroup*16+row;
				if (argmax) {
					if (*chosen==Vocabulary || value>*best || (value==*best && logicalRow<*chosen)) {
						*best=value; *chosen=logicalRow;
					}
				} else {
					unsigned t=0;
					while (t<tensorCount && logicalRow>=tensorBaseRow[t]+tensors[t]->place.rows) t++;
					if (t==tensorCount || logicalRow<tensorBaseRow[t]) return fail(15);
					outputs[t][logicalRow-tensorBaseRow[t]]=value;
				}
			}
		}
		for (unsigned worker=0;worker<workers;worker++) {
			const auto& task=tasks[worker];
			for (unsigned row=0;row<task.groups*16;row++) {
				float value=PioneerAttentionFloat(Words(RootWorkerResultAddress+worker*256*4)[row]);
				if (!Finite(value)) return fail(16);
				const unsigned logicalRow=task.firstGroup*16+row;
				if (argmax) {
					if (*chosen==Vocabulary || value>*best || (value==*best && logicalRow<*chosen)) {
						*best=value; *chosen=logicalRow;
					}
				} else {
					unsigned t=0;
					while (t<tensorCount && logicalRow>=tensorBaseRow[t]+tensors[t]->place.rows) t++;
					if (t==tensorCount || logicalRow<tensorBaseRow[t]) return fail(16);
					outputs[t][logicalRow-tensorBaseRow[t]]=value;
				}
			}
			state[2+worker]=task.sequence;
		}
		state[1]=nonce; Fence();
		progress.reserved[ProgressProjectionWavesSlot]++;
		if (workers==WorkTensixPeerCount && rootGroups==1)
			progress.reserved[ProgressFullCoreWavesSlot]++;
		Fence();
		for (unsigned fragment=0;fragment<fragmentCount;fragment++) cursor[fragment]=batch.nextGroup[fragment];
		completed+=processed;
	}
	const __UINT64_TYPE__ phaseEnd=Cycles();
	if (phaseEnd<phaseStart || !RecordCycleTicks(progress,
		ProgressProjectionSetupTicksSlot,phaseEnd-phaseStart)) return fail(21);
	return completed==groups || fail(17);
}

bool DeviceIO::Project(unsigned layer,unsigned role,const float* input,unsigned inputs,float* output,unsigned outputs)
{
	Trace(0xdb000000U|((layer&0xffU)<<16)|((role&0xffU)<<8)|((inputs>>8)&0xffU));
	const auto* tensor=model.Weight(layer,role);
	if (!tensor || tensor==&model.tied || tensor->inputs!=inputs || tensor->place.rows!=outputs
		|| !tensor->place.Valid()) return false;
	const __UINT64_TYPE__ packStart=Cycles();
	if (!packStart || !StageInput(tensor->type,input,inputs)) return false;
	const __UINT64_TYPE__ packEnd=Cycles();
	if (packEnd<packStart || !RecordCycleTicks(progress,
		ProgressActivationPackTicksSlot,packEnd-packStart)) return false;
	if (tensor->type==13) {
		Words(RootScaleEvidenceAddress)[2]=Words(Scales)[inputs>>8];
		Fence();
	}
	return StageBand(*tensor,output);
}

bool DeviceIO::ProjectPair(unsigned layer,unsigned firstRole,unsigned secondRole,
	const float* input,unsigned inputs,float* firstOutput,float* secondOutput,unsigned outputs)
{
	const unsigned roles[2]={firstRole,secondRole};
	const unsigned sizes[2]={outputs,outputs};
	float* destinations[2]={firstOutput,secondOutput};
	return ProjectGroup(layer,roles,input,inputs,destinations,sizes,2);
}

bool DeviceIO::ProjectGroup(unsigned layer,const unsigned* roles,const float* input,unsigned inputs,
	float* const* outputs,const unsigned* outputSizes,unsigned roleCount)
{
	if (!roles || !outputs || !outputSizes || !roleCount || roleCount>4) return false;
	const TensorSpace* tensors[4]={};
	bool processed[4]={};
	for (unsigned i=0;i<roleCount;i++) {
		Trace(0xdb000000U|((layer&0xffU)<<16)|((roles[i]&0xffU)<<8)|((inputs>>8)&0xffU));
		tensors[i]=model.Weight(layer,roles[i]);
		if (!tensors[i] || tensors[i]==&model.tied || !outputs[i]
			|| tensors[i]->inputs!=inputs || tensors[i]->place.rows!=outputSizes[i]
			|| !tensors[i]->place.Valid()) return false;
		for (unsigned previous=0;previous<i;previous++) if (tensors[previous]==tensors[i]) return false;
	}
	for (unsigned i=0;i<roleCount;i++) {
		if (processed[i]) continue;
		const unsigned type=tensors[i]->type;
		const __UINT64_TYPE__ packStart=Cycles();
		if (!packStart || !StageInput(type,input,inputs)) return false;
		const __UINT64_TYPE__ packEnd=Cycles();
		if (packEnd<packStart || !RecordCycleTicks(progress,
			ProgressActivationPackTicksSlot,packEnd-packStart)) return false;
		if (type==13) {
			Words(RootScaleEvidenceAddress)[2]=Words(Scales)[inputs>>8];
			Fence();
		}
		const TensorSpace* sameType[4]={};
		float* sameTypeOutputs[4]={};
		unsigned count=0;
		for (unsigned candidate=i;candidate<roleCount;candidate++) {
			if (!processed[candidate] && tensors[candidate]->type==type) {
				sameType[count]=tensors[candidate];
				sameTypeOutputs[count]=outputs[candidate];
				processed[candidate]=true;
				count++;
			}
		}
		if (!StageBands(sameType,count,sameTypeOutputs)) return false;
	}
	return true;
}

bool DeviceIO::TiedArgmax(const float* input,unsigned inputs,unsigned& token)
{
	Trace(0xdc000000U|(inputs&0x00ffffffU));
	const auto& tensor=model.tied;
	if (inputs!=Hidden || tensor.inputs!=Hidden || tensor.type!=14 || tensor.place.rows!=Vocabulary
		|| !tensor.place.Valid() || !StageInput(tensor.type,input,inputs)) return false;
	float best=0;
	unsigned chosen=Vocabulary;
	if (!StageBand(tensor,nullptr,&best,&chosen)) return false;
	token=chosen;
	return chosen<Vocabulary;
}

bool DeviceIO::PrefillDone(unsigned positions)
{
	Trace(0xdd000000U|(positions&0x00ffffffU));
	if (decodeStarted || positions!=position+1) return false;
	decodeStarted=Cycles();
	if (decodeStarted<started) return false;
	__UINT64_TYPE__ cycles=decodeStarted-started;
	progress.promptCyclesLow=uint32_t(cycles);
	progress.promptCyclesHigh=uint32_t(cycles>>32);
	Fence();
	progress.prefilled=positions;
	return true;
}

bool DeviceIO::Commit(unsigned at,unsigned token,bool sampled,unsigned next)
{
	Trace(0xde000000U|((at&0xffU)<<16)|((sampled?1U:0U)<<15)|(next&0x7fffU));
	if (at!=position || at>=model.context || token>=Vocabulary || next>=Vocabulary
		|| sampled!=(decodeStarted!=0) || !Quiet()) return false;
	if (sampled) {
		__UINT64_TYPE__ now=Cycles();
		if (now<decodeStarted || generated>=model.context) return false;
		__UINT64_TYPE__ cycles=now-decodeStarted;
		tokens[generated++]=next;
		progress.decodeCyclesLow=uint32_t(cycles);
		progress.decodeCyclesHigh=uint32_t(cycles>>32);
		progress.generated=generated;
	}
	Fence();
	progress.committed=++position;
	return true;
}

bool DeviceIO::FinishRequest(unsigned positions,unsigned outputs)
{
	Trace(0xdf000000U|((positions&0xffffU)<<8)|(outputs&0xffU));
	if (positions!=position || !outputs || outputs!=generated || !Quiet()) return false;
	progress.reserved[ProgressReadyFailureSlot]=0;
	Fence();
	progress.terminal=1;
	Fence();
	return true;
}
}
