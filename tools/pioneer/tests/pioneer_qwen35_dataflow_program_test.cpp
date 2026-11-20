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
		if ((type!=8 && type!=12 && type!=14) || inputs!=Hidden
			|| bytes!=DataflowInputPageBytes || source[chunk*256]!=float(chunk*256))
			return false;
		for (unsigned i=0;i<bytes;i++) output[i]=uint8_t(chunk);
		output[bytes-1]=uint8_t(chunk^0x5a);
		return true;
	}
};

static void CompileRoleInterfaces()
{
	DataflowProjectionProgram program{};
	DataflowQueueView queues[3]{};
	uint8_t activation[DataflowActivationCacheBytes]{};
	ContractOps ops{};
	if (false) {
		PioneerQwen35DataflowReader(program,0,queues[0],queues[1],activation,ops);
		PioneerQwen35DataflowCompute(program,0,queues[0],queues[1],queues[2],ops);
		PioneerQwen35DataflowWriter(program,0,queues[2],ops);
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
	Rows(weight.place,Hidden,2120,0x08000000);
	RowSpace input{},output{};
	Rows(input,Ffn,sizeof(float),0x04000000);
	Rows(output,Ffn,sizeof(float),0x06000000);

	DataflowProjectionProgram program{};
	assert(DataflowProgramBuilder::Build(weight,input,Hidden,output,program));
	assert(program.Valid());
	assert(program.activeCores==120 && program.kernelRoles==DataflowAllRoles);
	assert(program.groups==Hidden/16);
	assert(program.circularBuffer[0].pageBytes==3392
		&& program.circularBuffer[0].pages==2);
	assert(program.circularBuffer[1].pageBytes==576
		&& program.circularBuffer[1].pages==2);
	assert(program.circularBuffer[2].pageBytes==64
		&& program.circularBuffer[2].pages==2);
	DataflowQueueControl controls[DataflowCircularBufferCount]{};
	uint8_t local[DataflowLocalBufferEnd-DataflowWeightBufferAddress]{};
	DataflowQueueView queues[DataflowCircularBufferCount]{};
	assert(DataflowBindQueues(program,controls,local,queues));
	for (unsigned i=0;i<DataflowCircularBufferCount;i++)
		assert(queues[i].local==local+(program.circularBuffer[i].address-DataflowWeightBufferAddress));
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
		&& ticket==UINT32_MAX && readPage==local+program.circularBuffer[0].pageBytes);
	assert(DataflowQueuePopFront(queues[0],ticket));
	assert(controls[0].produced==0 && controls[0].consumed==0);
	static uint8_t activationCache[DataflowActivationCacheBytes];
	float* activation=reinterpret_cast<float*>(activationCache);
	PackTestOps packOps{};
	for (unsigned i=0;i<Hidden;i++) activation[i]=float(i);
	assert(DataflowPackActivationCache(program,queues[1],activationCache,packOps));
	for (unsigned chunk=0;chunk<Hidden/256;chunk++) {
		assert(activationCache[chunk*DataflowInputPageBytes]==uint8_t(chunk));
		assert(activationCache[(chunk+1)*DataflowInputPageBytes-1]
			==uint8_t(chunk^0x5a));
	}
	assert(DataflowActivationChunk(14,9)==9);
	assert(DataflowActivationChunk(8,79)==9);
	unsigned covered=0,splitOutputs=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		assert(program.core[core].active);
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
	assert(!DataflowCoreHeaderValid(corrupt,119));

	TensorSpace tooFew=weight;
	tooFew.place.rows=Hidden/2;
	tooFew.place.count=1;
	tooFew.place.parts[0]={0,0x08000000,0,Hidden/2};
	for (unsigned i=1;i<Banks;i++) tooFew.place.parts[i]={};
	DataflowProjectionProgram rejected{};
	assert(!DataflowProgramBuilder::Build(tooFew,input,Hidden,output,rejected));
	return 0;
}
