/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowBlackholeConvWorker.h"
#include "../PioneerQwen35DataflowAuthorization.h"
#include "../PioneerQwen35DataflowOwner.h"
#include <assert.h>
#include <cmath>
#include <cstring>

using namespace PioneerQwen35;

static const unsigned TestRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

static RowSpace MakeRows(unsigned rows,unsigned rowBytes,unsigned unit,
	unsigned base)
{
	RowSpace result{};
	result.rows=rows;
	result.rowBytes=rowBytes;
	result.unit=unit;
	const unsigned groups=rows/16;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=groups/Banks+unsigned(bank<groups%Banks);
		if (!count) continue;
		result.parts[result.count++]={bank,base+bank*0x01000000,first*16,count*16};
		first+=count;
	}
	assert(result.Valid());
	return result;
}

struct ConvMockIO {
	float qkv[ConvChannels]{};
	float weights[ConvChannels*4]{};
	float history[2][ConvChannels*3]{};
	unsigned qkvReads[DataflowConvGroups]{};
	unsigned weightReads[DataflowConvGroups]{};
	unsigned historyReads[DataflowConvGroups]{};
	unsigned qkvWrites[DataflowConvGroups]{};
	unsigned historyWrites[DataflowConvGroups]{};
	unsigned readSlot=0,writeSlot=0;
	bool bound=false;

	bool Bind(const DataflowConvInvocation& invocation)
	{
		if (!invocation.Valid()) return false;
		readSlot=invocation.position&1;
		writeSlot=readSlot^1;
		bound=true;
		return true;
	}

	bool ReadQkvPanel(unsigned row,float* output)
	{
		if (!bound || row>=ConvChannels || (row&15)) return false;
		const unsigned group=row/16;
		qkvReads[group]++;
		memcpy(output,qkv+row,16*sizeof(float));
		return true;
	}

	bool ReadWeightPanel(unsigned row,float* output)
	{
		if (!bound || row>=ConvChannels || (row&15)) return false;
		const unsigned group=row/16;
		weightReads[group]++;
		memcpy(output,weights+row*4,64*sizeof(float));
		return true;
	}

	bool ReadHistoryPanel(unsigned row,float* output)
	{
		if (!bound || row>=ConvChannels || (row&15)) return false;
		const unsigned group=row/16;
		historyReads[group]++;
		memcpy(output,history[readSlot]+row*3,48*sizeof(float));
		return true;
	}

	bool WriteHistoryPanel(unsigned row,const float* input)
	{
		if (!bound || row>=ConvChannels || (row&15)) return false;
		const unsigned group=row/16;
		historyWrites[group]++;
		memcpy(history[writeSlot]+row*3,input,48*sizeof(float));
		return true;
	}

	bool WriteQkvPanel(unsigned row,const float* input)
	{
		if (!bound || row>=ConvChannels || (row&15)) return false;
		const unsigned group=row/16;
		qkvWrites[group]++;
		memcpy(qkv+row,input,16*sizeof(float));
		return true;
	}
};

struct ConvOwnerMockIO {
	DataflowProjectionAuthority& authority;
	DataflowWorkerSnapshot snapshot[DataflowCoreCount]{};
	DataflowConvInvocation invocation[DataflowCoreCount]{};
	bool written[DataflowCoreCount]{};

	explicit ConvOwnerMockIO(DataflowProjectionAuthority& boundAuthority)
		:authority(boundAuthority)
	{
		for (unsigned core=0;core<DataflowCoreCount;core++)
			snapshot[core].state=DataflowWorkerReady;
	}

	bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& result) const
	{
		if (core>=DataflowCoreCount) return false;
		result=snapshot[core];
		return true;
	}

	bool CoreReady(unsigned core,const DataflowWorkerSnapshot& observed) const
	{
		return core<DataflowCoreCount && DataflowWorkerSnapshotReady(observed);
	}

	bool AuthorizeConv(const DataflowConvProgram& program) const
		{ return authority.AuthorizeConv(program); }
	bool BeginConv(const DataflowConvProgram& program)
		{ return authority.BeginConv(program); }

	bool WriteConvInvocation(unsigned core,const DataflowConvInvocation& value)
	{
		if (core>=DataflowCoreCount || value.core!=core || !value.Valid()) return false;
		invocation[core]=value;
		written[core]=true;
		return true;
	}

	bool ConvInvocationMatches(unsigned core,const DataflowConvInvocation& value) const
	{
		return core<DataflowCoreCount && written[core]
			&& memcmp(&invocation[core],&value,sizeof(value))==0;
	}

	bool PublishCommand(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount || !written[core]) return false;
		snapshot[core]={sequence,0,sequence,DataflowWorkerComplete,0,2};
		return true;
	}

	bool WaitForCompletion(unsigned core,unsigned sequence,
		DataflowWorkerSnapshot& result)
	{
		return core<DataflowCoreCount
			&& snapshot[core].commandSequence==sequence
			&& snapshot[core].completedSequence==sequence
			&& (result=snapshot[core],true);
	}

	bool AuditConv(const DataflowConvProgram& program,unsigned sequence,
		const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount]) const
	{
		if (!authority.PendingConvMatches(program)) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowConvInvocation expected{};
			if (receipts[core].commandSequence!=sequence
				|| receipts[core].state!=DataflowWorkerComplete
				|| !written[core]
				|| !DataflowConvProgramBuilder::BindCore(program,core,expected)
				|| memcmp(&expected,&invocation[core],sizeof(expected))!=0) return false;
		}
		return true;
	}

	bool Acknowledge(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount
			|| snapshot[core].completedSequence!=sequence
			|| snapshot[core].state!=DataflowWorkerComplete) return false;
		snapshot[core].acknowledgedSequence=sequence;
		snapshot[core].state=DataflowWorkerReady;
		snapshot[core].phase=0;
		return true;
	}

	bool CompleteConv(const DataflowConvProgram& program,unsigned sequence)
		{ return sequence && authority.CompleteConv(program); }
};

static void RunAllCores(DataflowConvProgram& program,ConvMockIO& io)
{
	assert(program.Valid());
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowConvInvocation invocation{};
		assert(DataflowConvProgramBuilder::BindCore(program,core,invocation));
		assert(invocation.Valid());
		float qkv[16]{},weights[64]{},history[48]{},next[48]{},output[16]{};
		assert(PioneerQwen35DataflowConvWorker(invocation,qkv,weights,history,
			next,output,io));
	}
}

static float ReferenceSilu(float value)
{
	return value/(1.0f+std::exp(-value));
}

int main()
{
	DeviceLayout manifest{};
	manifest.magic=LayoutMagic;
	manifest.version=LayoutVersion;
	manifest.context=8;
	manifest.activation[0]=MakeRows(Ffn,sizeof(float),16,0x08000000);
	manifest.activation[1]=MakeRows(Ffn,sizeof(float),16,0x09000000);
	TensorSpace& convWeight=manifest.layers[0].weights[ConvWeights];
	convWeight.type=0;
	convWeight.inputs=4;
	convWeight.place=MakeRows(ConvChannels,4*sizeof(float),1,0x10000000);
	TensorSpace& qkvWeight=manifest.layers[0].weights[RecurrentQkv];
	qkvWeight.type=13;
	qkvWeight.inputs=Hidden;
	qkvWeight.place=MakeRows(ConvChannels,
		PioneerQwen35Projection::TileBytes(qkvWeight.type)*(Hidden/256)/16,
		16,0x11000000);
	manifest.layers[0].conv[0]=MakeRows(ConvChannels,3*sizeof(float),1,
		0x20000000);
	manifest.layers[0].conv[1]=MakeRows(ConvChannels,3*sizeof(float),1,
		0x30000000);
	DataflowGraphWorkspace workspace{};
	workspace.space[DataflowGraphResidualWorkspaceIndex]=
		MakeRows(Hidden,sizeof(float),16,0x40000000);
	workspace.space[DataflowGraphUpWorkspaceIndex]=
		MakeRows(Ffn,sizeof(float),16,0x50000000);
	workspace.space[DataflowGraphRecurrentQkvWorkspaceIndex]=
		MakeRows(ConvChannels,sizeof(float),16,0x60000000);
	workspace.space[DataflowGraphRecurrentGateWorkspaceIndex]=
		MakeRows(DeltaInner,sizeof(float),16,0x70000000);
	workspace.space[DataflowGraphRecurrentAlphaWorkspaceIndex]=
		MakeRows(DeltaHeads,sizeof(float),16,0x80000000);
	workspace.space[DataflowGraphRecurrentBetaWorkspaceIndex]=
		MakeRows(DeltaHeads,sizeof(float),16,0x81000000);
	assert(workspace.Valid(manifest));
	DataflowProjectionAuthority authority;
	assert(authority.BindValidatedModel(manifest,workspace,TestRoutes));
	assert(authority.MarkActivationInitialized(0));
	DataflowProjectionProgram qkvProjection{};
	assert(authority.BuildProjection(0,RecurrentQkv,0,
		DataflowGraphRecurrentQkvOutputSlot,qkvProjection));
	assert(authority.Begin(qkvProjection));
	assert(authority.Complete(qkvProjection));
	assert(authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex)
		&& authority.GraphWorkspaceLayer(DataflowGraphRecurrentQkvWorkspaceIndex)==0);

	DataflowConvProgram position0{};
	assert(DataflowConvProgramBuilder::Build(manifest,workspace,0,0,
		TestRoutes,position0));
	assert(position0.historyRead.parts[0].address
		==manifest.layers[0].conv[0].parts[0].address);
	assert(position0.historyWrite.parts[0].address
		==manifest.layers[0].conv[1].parts[0].address);
	DataflowConvProgram authorizedPosition0{};
	assert(authority.BuildConv(0,0,authorizedPosition0));
	assert(authority.BeginConv(authorizedPosition0));
	assert(!authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex)
		&& !authority.BuildConv(0,0,position0));
	assert(authority.CompleteConv(authorizedPosition0));
	assert(authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex));
	DataflowProjectionOwner owner;
	ConvOwnerMockIO ownerIO(authority);
	assert(owner.Initialize(ownerIO)==DataflowOwnerComplete);
	assert(owner.RunConv(position0,ownerIO)==DataflowOwnerComplete);
	assert(owner.NextSequence()==1
		&& authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex));
	unsigned covered=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowConvInvocation invocation{};
		assert(DataflowConvProgramBuilder::BindCore(position0,core,invocation));
		assert(invocation.groupCount==(core<32?5U:4U));
		covered+=invocation.groupCount;
	}
	assert(covered==DataflowConvGroups);

	ConvMockIO io{};
	for (unsigned row=0;row<ConvChannels;row++) {
		io.qkv[row]=float(int(row%17)-8)*0.125f;
		for (unsigned tap=0;tap<4;tap++)
			io.weights[row*4+tap]=float(int((row+tap*3)%11)-5)*0.0625f;
	}
	float expectedQkv[ConvChannels];
	memcpy(expectedQkv,io.qkv,sizeof(expectedQkv));
	float expectedHistory[ConvChannels*3]{};
	for (unsigned row=0;row<ConvChannels;row++) {
		const float current=expectedQkv[row];
		const float sum=current*io.weights[row*4+3];
		const float activated=ReferenceSilu(sum);
		expectedHistory[row*3+2]=current;
		expectedQkv[row]=activated;
	}
	RunAllCores(position0,io);
	for (unsigned group=0;group<DataflowConvGroups;group++) {
		assert(io.qkvReads[group]==1 && io.weightReads[group]==1
			&& io.historyReads[group]==0 && io.qkvWrites[group]==1
			&& io.historyWrites[group]==1);
	}
	for (unsigned row=0;row<ConvChannels;row++) {
		assert(std::fabs(io.qkv[row]-expectedQkv[row])<2.0e-6f);
		for (unsigned tap=0;tap<3;tap++)
			assert(std::fabs(io.history[1][row*3+tap]
				-expectedHistory[row*3+tap])<1.0e-7f);
	}

	DataflowConvProgram position1{};
	assert(DataflowConvProgramBuilder::Build(manifest,workspace,0,1,
		TestRoutes,position1));
	assert(position1.historyRead.parts[0].address
		==manifest.layers[0].conv[1].parts[0].address);
	assert(position1.historyWrite.parts[0].address
		==manifest.layers[0].conv[0].parts[0].address);
	DataflowConvProgram authorizedPosition1{};
	assert(authority.BuildConv(0,1,authorizedPosition1));
	assert(authority.BeginConv(authorizedPosition1));
	assert(authority.CompleteConv(authorizedPosition1));
	assert(!authority.BuildConv(3,0,authorizedPosition1)
		&& !authority.BuildConv(0,manifest.context,authorizedPosition1));
	for (unsigned row=0;row<ConvChannels;row++) {
		io.qkv[row]=float(int(row%13)-6)*0.1f;
		expectedQkv[row]=io.qkv[row];
	}
	for (unsigned group=0;group<DataflowConvGroups;group++) {
		io.qkvReads[group]=io.weightReads[group]=io.historyReads[group]=0;
		io.qkvWrites[group]=io.historyWrites[group]=0;
	}
	for (unsigned row=0;row<ConvChannels;row++) {
		const float current=expectedQkv[row];
		const float* old=io.history[1]+row*3;
		float sum=current*io.weights[row*4+3];
		sum=((old[0]*io.weights[row*4]+old[1]*io.weights[row*4+1])
			+old[2]*io.weights[row*4+2])+sum;
		expectedHistory[row*3]=old[1];
		expectedHistory[row*3+1]=old[2];
		expectedHistory[row*3+2]=current;
		expectedQkv[row]=ReferenceSilu(sum);
	}
	RunAllCores(position1,io);
	for (unsigned group=0;group<DataflowConvGroups;group++) {
		assert(io.qkvReads[group]==1 && io.weightReads[group]==1
			&& io.historyReads[group]==1 && io.qkvWrites[group]==1
			&& io.historyWrites[group]==1);
	}
	for (unsigned row=0;row<ConvChannels;row++) {
		assert(std::fabs(io.qkv[row]-expectedQkv[row])<2.0e-6f);
		for (unsigned tap=0;tap<3;tap++)
			assert(std::fabs(io.history[0][row*3+tap]
				-expectedHistory[row*3+tap])<1.0e-7f);
	}

	DataflowConvProgram invalid=position1;
	invalid.layer=3;
	assert(!invalid.Valid());
	DataflowConvInvocation bad{};
	assert(DataflowConvProgramBuilder::BindCore(position1,0,bad));
	bad.firstGroup++;
	assert(!bad.Valid());
	DataflowConvProgram abort{};
	assert(authority.BuildConv(0,2,abort) && authority.BeginConv(abort));
	assert(authority.AbortPendingProjection());
	assert(!authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex));
	return 0;
}
