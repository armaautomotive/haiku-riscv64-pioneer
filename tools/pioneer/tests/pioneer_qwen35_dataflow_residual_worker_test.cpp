/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

#include "../PioneerQwen35DataflowBlackholeResidualWorker.h"

using namespace PioneerQwen35;

static const unsigned TestRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{886,1078,1270,885,1077,1269,1461}
};

static void Rows(RowSpace& space,unsigned rows,uint32_t base)
{
	space={};
	space.rows=rows;
	space.rowBytes=sizeof(float);
	space.unit=16;
	const unsigned groups=rows/16;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=groups/Banks+unsigned(bank<groups%Banks);
		if (!count) continue;
		space.parts[space.count++]={bank,base+bank*0x01000000,first*16,count*16};
		first+=count;
	}
}

struct PanelIO {
	const DataflowResidualInvocation* program=nullptr;
	std::vector<float>* activation=nullptr;
	std::vector<float>* residual=nullptr;
	std::vector<unsigned>* writes=nullptr;

	bool Bind(const DataflowResidualInvocation& invocation)
	{
		if (!invocation.Valid()) return false;
		program=&invocation;
		return true;
	}

	bool ReadActivationPanel(unsigned row,float* output) const
	{
		if (!program || !activation || !output || row+16>Hidden) return false;
		for (unsigned lane=0;lane<16;lane++) output[lane]=(*activation)[row+lane];
		return true;
	}

	bool ReadResidualPanel(unsigned row,float* output) const
	{
		if (!program || !residual || !output || row+16>Hidden) return false;
		for (unsigned lane=0;lane<16;lane++) output[lane]=(*residual)[row+lane];
		return true;
	}

	bool WritePanel(const RowSpace& space,const DataflowOutputSegment& output,
		std::vector<float>* values) const
	{
		if (!program || !values || output.firstRow+16>Hidden) return false;
		bool assigned=false;
		for (unsigned i=0;i<program->segmentCount;i++) {
			const auto& expected=program->segment[i];
			if (expected.firstRow!=output.firstRow) continue;
			if (expected.bank!=output.bank || expected.address!=output.address
				|| expected.rows!=output.rows || expected.bytes!=output.bytes)
				return false;
			assigned=true;
			break;
		}
		Range range{};
		if (!assigned || !space.Resolve(output.firstRow,16,0,output.bytes,range)
			|| range.bank!=output.bank || range.address!=output.address
			|| (*writes)[output.firstRow/16]++) return false;
		for (unsigned lane=0;lane<16;lane++)
			(*values)[output.firstRow+lane]=outputPage[lane];
		return true;
	}

	bool WriteResidualPanel(const DataflowOutputSegment& output,
		const float* outputPage) const
	{
		return program && program->mode==DataflowResidualSave && outputPage
			&& WritePanel(program->residual,output,
				const_cast<std::vector<float>*>(residual));
	}

	bool WriteActivationPanel(const DataflowOutputSegment& output,
		const float* outputPage) const
	{
		return program && program->mode==DataflowResidualAdd && outputPage
			&& WritePanel(program->activation,output,activation);
	}

	float* outputPage=nullptr;
};

static void RunAllCores(const DataflowResidualProgram& program,
	std::vector<float>& activation,std::vector<float>& residual,
	std::vector<unsigned>& writes)
{
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowResidualInvocation invocation{};
		assert(DataflowResidualProgramBuilder::BindCore(program,core,invocation));
		float activationPage[16]{},residualPage[16]{},outputPage[16]{};
		PanelIO io{&invocation,&activation,&residual,&writes,outputPage};
		assert(PioneerQwen35DataflowResidualWorker(invocation,activationPage,
			residualPage,outputPage,io));
	}
}

int main()
{
	DeviceLayout manifest{};
	manifest.magic=LayoutMagic;
	manifest.version=LayoutVersion;
	manifest.context=128;
	Rows(manifest.activation[0],Ffn,0x04000000);
	Rows(manifest.activation[1],Ffn,0x06000000);
	DataflowGraphWorkspace workspace{};
	Rows(workspace.space[DataflowGraphResidualWorkspaceIndex],Hidden,0x20000000);
	Rows(workspace.space[DataflowGraphUpWorkspaceIndex],Ffn,0x30000000);
	Rows(workspace.space[DataflowGraphRecurrentQkvWorkspaceIndex],ConvChannels,
		0x40000000);
	Rows(workspace.space[DataflowGraphRecurrentGateWorkspaceIndex],DeltaInner,
		0x50000000);
	Rows(workspace.space[DataflowGraphRecurrentAlphaWorkspaceIndex],DeltaHeads,
		0x60000000);
	Rows(workspace.space[DataflowGraphRecurrentBetaWorkspaceIndex],DeltaHeads,
		0x70000000);
	assert(workspace.Valid(manifest));

	std::vector<float> activation(Ffn),residual(Hidden),saved(Hidden),delta(Hidden);
	std::vector<unsigned> writes(DataflowResidualGroups,0);
	for (unsigned row=0;row<Hidden;row++) {
		activation[row]=float(int(row%701)-350)*0.03125f;
		saved[row]=activation[row];
		delta[row]=float(int(row%101)-50)*0.0625f;
	}
	DataflowResidualProgram program{};
	assert(DataflowResidualProgramBuilder::Build(manifest,workspace,
		DataflowResidualSave,0,TestRoutes,program));
	RunAllCores(program,activation,residual,writes);
	for (unsigned group=0;group<DataflowResidualGroups;group++) assert(writes[group]==1);
	for (unsigned row=0;row<Hidden;row++) assert(residual[row]==saved[row]);

	for (unsigned row=0;row<Hidden;row++) activation[row]=delta[row];
	for (unsigned& count:writes) count=0;
	assert(DataflowResidualProgramBuilder::Build(manifest,workspace,
		DataflowResidualAdd,0,TestRoutes,program));
	RunAllCores(program,activation,residual,writes);
	for (unsigned group=0;group<DataflowResidualGroups;group++) assert(writes[group]==1);
	for (unsigned row=0;row<Hidden;row++)
		assert(std::fabs(activation[row]-(saved[row]+delta[row]))<=1.0e-7f);
	return 0;
}
