/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowKernels.h"
#include <assert.h>

using namespace PioneerQwen35;

struct ContractOps {
	enum { TensixMatrixEngine=1 };
	bool ReadActivation(const RowSpace&,unsigned,uint8_t*,unsigned) { return false; }
	bool ReadWeight(unsigned,uint32_t,uint8_t*,unsigned) { return false; }
	bool PackActivation(unsigned,unsigned,unsigned,const float*,uint8_t*,unsigned) { return false; }
	bool TensixBeginOutput(unsigned) { return false; }
	bool TensixMatmul(unsigned,unsigned,const uint8_t*,const uint8_t*) { return false; }
	bool TensixFinishOutput(uint8_t*,unsigned) { return false; }
	bool WriteOutput(unsigned,uint32_t,const uint8_t*,unsigned) { return false; }
};

struct PackTestOps {
	bool PackActivation(unsigned type,unsigned inputs,unsigned chunk,
		const float* source,uint8_t* output,unsigned bytes)
	{
		if ((type!=8 && type!=12 && type!=14)
			|| inputs%256 || chunk>=inputs/256 || bytes!=DataflowInputPageBytes
			|| source[chunk*256]!=float(chunk*256))
			return false;
		for (unsigned i=0;i<bytes;i++) output[i]=uint8_t(chunk);
		output[bytes-1]=uint8_t(chunk^0x5a);
		return true;
	}
};

static void CompileRoleInterfaces()
{
	DataflowCoreInvocation program{};
	DataflowQueueView queues[3]{};
	uint8_t activation[DataflowActivationCacheBytes]{};
	DataflowDeviceContext context{};
	ContractOps ops{};
	if (false) {
		DataflowBindLocalContext(context,DataflowFailureQueue);
		PioneerQwen35DataflowReader(program,queues[0],queues[1],activation,ops);
		PioneerQwen35DataflowCompute(program,queues[0],queues[1],queues[2],ops);
		PioneerQwen35DataflowWriter(program,queues[2],ops);
		PioneerQwen35DataflowReaderEntry(ops);
		PioneerQwen35DataflowComputeEntry(ops);
		PioneerQwen35DataflowWriterEntry(ops);
	}
}

static void Rows(RowSpace& space,unsigned rows,unsigned rowBytes,uint32_t base)
{
	space={};
	space.rows=rows;
	space.rowBytes=rowBytes;
	space.unit=16;
	const unsigned groups=rows/16;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=groups/Banks+unsigned(bank<groups%Banks);
		space.parts[bank]={bank,base+bank*0x01000000,first*16,count*16};
		space.count++;
		first+=count;
	}
}

int main()
{
	CompileRoleInterfaces();
	TensorSpace weight{};
	weight.type=14;
	weight.inputs=Hidden;
	Rows(weight.place,Ffn,2120,0x08000000);
	RowSpace input{},output{};
	Rows(input,Ffn,sizeof(float),0x04000000);
	Rows(output,Ffn,sizeof(float),0x06000000);

	DataflowProjectionProgram program{};
	assert(DataflowProgramBuilder::Build(weight,input,Hidden,output,program));
	assert(program.Valid());
	assert(program.activeCores==120 && program.kernelRoles==DataflowAllRoles);
	assert(sizeof(DataflowCoreInvocation)<=DataflowProgramRegionBytes
		&& DataflowProgramAddress==0xaa000
		&& DataflowProgramRegionEnd==0xab000);
	assert(program.groups==Ffn/16);
	assert(program.circularBuffer[0].pageBytes==3392
		&& program.circularBuffer[0].pages==2);
	assert(program.circularBuffer[1].pageBytes==576
		&& program.circularBuffer[1].pages==2);
	assert(program.circularBuffer[2].pageBytes==64
		&& program.circularBuffer[2].pages==2);
	DataflowCoreInvocation corePlan{};
	assert(DataflowProgramBuilder::BindCore(program,0,corePlan));
	DataflowQueueControl controls[DataflowCircularBufferCount]{};
	uint8_t local[DataflowLocalBufferEnd-DataflowWeightBufferAddress]{};
	DataflowQueueView queues[DataflowCircularBufferCount]{};
	assert(DataflowBindQueues(corePlan,controls,local,queues));
	for (unsigned i=0;i<DataflowCircularBufferCount;i++)
		assert(queues[i].local==local+(corePlan.circularBuffer[i].address-DataflowWeightBufferAddress));
	uint32_t ticket=0;
	uint8_t* page=nullptr;
	assert(DataflowQueueReserveBack(queues[0],ticket) && ticket==0);
	page=DataflowQueueWritePage(queues[0],ticket);
	assert(page==local && page[0]==0);
	page[0]=0x5a;
	assert(DataflowQueuePublishBack(queues[0],ticket));
	uint8_t* readPage=nullptr;
	assert(DataflowQueueWaitFront(queues[0],ticket,readPage)
		&& ticket==0 && readPage==page && readPage[0]==0x5a);
	assert(DataflowQueuePopFront(queues[0],ticket));
	assert(controls[0].produced==1 && controls[0].consumed==1);
	controls[0].produced=UINT32_MAX;
	controls[0].consumed=UINT32_MAX;
	assert(DataflowQueueReserveBack(queues[0],ticket) && ticket==UINT32_MAX);
	assert(DataflowQueuePublishBack(queues[0],ticket));
	assert(DataflowQueueWaitFront(queues[0],ticket,readPage)
		&& ticket==UINT32_MAX && readPage==local+corePlan.circularBuffer[0].pageBytes);
	assert(DataflowQueuePopFront(queues[0],ticket));
	assert(controls[0].produced==0 && controls[0].consumed==0);
	DataflowQueueFail(queues[0],DataflowFailureCompute);
	DataflowQueueFail(queues[0],DataflowFailureWriter);
	assert(DataflowLoadAcquire(controls[0].failure)==DataflowFailureCompute);
	assert(!DataflowQueueReserveBack(queues[0],ticket));
	static uint8_t activationCache[DataflowActivationCacheBytes];
	float* activation=reinterpret_cast<float*>(activationCache);
	PackTestOps packOps{};
	for (unsigned i=0;i<Hidden;i++) activation[i]=float(i);
	assert(DataflowPackActivationCache(corePlan,queues[1],activationCache,packOps));
	for (unsigned chunk=0;chunk<Hidden/256;chunk++) {
		assert(activationCache[chunk*DataflowInputPageBytes]==uint8_t(chunk));
		assert(activationCache[(chunk+1)*DataflowInputPageBytes-1]
			==uint8_t(chunk^0x5a));
	}
	assert(DataflowActivationChunk(14,9)==9);
	assert(DataflowActivationChunk(8,79)==9);
	assert(DataflowActivationChunk(13,9)==9);
	assert(DataflowActivationPackingType(8)==8
		&& DataflowActivationPackingType(12)==12
		&& DataflowActivationPackingType(13)==12
		&& DataflowActivationPackingType(14)==14);
	unsigned covered=0,splitOutputs=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		assert(program.core[core].active);
		DataflowCoreInvocation invocation{};
		assert(DataflowProgramBuilder::BindCore(program,core,invocation));
		assert(invocation.Valid() && invocation.core==core
			&& invocation.assignment.groups==program.core[core].groups);
		for (unsigned segment=0;segment<program.core[core].segmentCount;segment++) {
			const auto& weightSegment=program.core[core].weight[segment];
			const auto& outputSegment=program.core[core].output[segment];
			assert(weightSegment.firstGroup==covered);
			assert(outputSegment.firstRow==weightSegment.firstGroup*16);
			assert(outputSegment.rows==weightSegment.groups*16);
			covered+=weightSegment.groups;
			if (program.core[core].segmentCount>1) splitOutputs++;
		}
	}
	assert(covered==program.groups && splitOutputs>0);

	DataflowProjectionProgram corrupt=program;
	corrupt.core[0].weight[0].firstGroup++;
	assert(!corrupt.Valid());
	corrupt=program;
	corrupt.core[119].output[0].bank=(corrupt.core[119].output[0].bank+1)%Banks;
	assert(!corrupt.Valid());
	DataflowCoreInvocation corruptCore{};
	assert(DataflowProgramBuilder::BindCore(program,119,corruptCore));
	corruptCore.assignment.output[0].bank=(corruptCore.assignment.output[0].bank+1)%Banks;
	assert(!DataflowCoreHeaderValid(corruptCore));

	TensorSpace q4Weight{};
	q4Weight.type=12;
	q4Weight.inputs=Ffn;
	Rows(q4Weight.place,Hidden,5184,0x08000000);
	DataflowProjectionProgram q4Program{};
	assert(DataflowProgramBuilder::Build(q4Weight,input,Ffn,output,q4Program));
	assert(q4Program.blocks==Ffn/256
		&& q4Program.circularBuffer[0].pageBytes==2304);
	DataflowCoreInvocation q4Core{};
	assert(DataflowProgramBuilder::BindCore(q4Program,0,q4Core));
	for (unsigned i=0;i<Ffn;i++) activation[i]=float(i);
	assert(DataflowPackActivationCache(q4Core,queues[1],activationCache,packOps));
	for (unsigned chunk=0;chunk<Ffn/256;chunk++)
		assert(activationCache[chunk*DataflowInputPageBytes]==uint8_t(chunk));

	TensorSpace q5Weight{};
	q5Weight.type=13;
	q5Weight.inputs=Hidden;
	Rows(q5Weight.place,Ffn,Hidden/256*(PioneerQwen35Projection::TileBytes(13)/16),
		0x08000000);
	DataflowProjectionProgram q5Program{};
	assert(DataflowProgramBuilder::Build(q5Weight,input,Hidden,output,q5Program));
	assert(q5Program.blocks==Hidden/256
		&& q5Program.circularBuffer[0].pageBytes==2816);
	DataflowCoreInvocation q5Core{};
	assert(DataflowProgramBuilder::BindCore(q5Program,0,q5Core));
	for (unsigned i=0;i<Hidden;i++) activation[i]=float(i);
	assert(DataflowPackActivationCache(q5Core,queues[1],activationCache,packOps));
	for (unsigned chunk=0;chunk<Hidden/256;chunk++)
		assert(activationCache[chunk*DataflowInputPageBytes]==uint8_t(chunk));

	struct ProjectionShape { unsigned type,inputs; };
	const ProjectionShape modelShapes[]={{8,2560},{12,2560},{13,2560},{14,2560},
		{12,4096},{13,4096},{12,9216},{14,9216}};
	for (const auto& shape:modelShapes) {
		TensorSpace shapeWeight{};
		shapeWeight.type=shape.type;
		shapeWeight.inputs=shape.inputs;
		const unsigned shapeBlocks=shape.inputs/(shape.type==8?32:256);
		const unsigned shapeRowBytes=shapeBlocks
			*(PioneerQwen35Projection::TileBytes(shape.type)/16);
		Rows(shapeWeight.place,Ffn,shapeRowBytes,0x08000000);
		DataflowProjectionProgram shapeProgram{};
		assert(DataflowProgramBuilder::Build(shapeWeight,input,shape.inputs,output,
			shapeProgram));
		assert(shapeProgram.blocks==shapeBlocks
			&& shapeProgram.circularBuffer[0].pageBytes
				==PioneerQwen35Projection::TileBytes(shape.type));
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowCoreInvocation invocation{};
			assert(DataflowProgramBuilder::BindCore(shapeProgram,core,invocation));
			assert(invocation.Valid() && invocation.assignment.groups>0);
		}
	}

	TensorSpace q8Weight{};
	q8Weight.type=8;
	q8Weight.inputs=Hidden;
	Rows(q8Weight.place,Ffn,2880,0x08000000);
	DataflowProjectionProgram q8Program{};
	assert(DataflowProgramBuilder::Build(q8Weight,input,Hidden,output,q8Program));
	assert(q8Program.blocks==Hidden/32);
	DataflowCoreInvocation q8Core{};
	assert(DataflowProgramBuilder::BindCore(q8Program,0,q8Core));
	for (unsigned i=0;i<Hidden;i++) activation[i]=float(i);
	assert(DataflowPackActivationCache(q8Core,queues[1],activationCache,packOps));
	for (unsigned chunk=0;chunk<Hidden/256;chunk++)
		assert(activationCache[chunk*DataflowInputPageBytes]==uint8_t(chunk));

	TensorSpace tooFew=weight;
	tooFew.place.rows=Hidden/2;
	tooFew.place.count=1;
	tooFew.place.parts[0]={0,0x08000000,0,Hidden/2};
	for (unsigned i=1;i<Banks;i++) tooFew.place.parts[i]={};
	DataflowProjectionProgram rejected{};
	assert(!DataflowProgramBuilder::Build(tooFew,input,Hidden,output,rejected));
	return 0;
}
