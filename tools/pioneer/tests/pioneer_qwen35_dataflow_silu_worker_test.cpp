/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

#include "../PioneerQwen35DataflowBlackholeSiluWorker.h"

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
		space.parts[bank]={bank,base+bank*0x01000000,first*16,count*16};
		space.count++;
		first+=count;
	}
}

struct PanelIO {
	const DataflowSiluInvocation* program=nullptr;
	std::vector<float>* gate=nullptr;
	const std::vector<float>* up=nullptr;
	std::vector<unsigned>* writes=nullptr;

	bool Bind(const DataflowSiluInvocation& invocation)
	{
		if (!invocation.Valid()) return false;
		program=&invocation;
		return true;
	}

	bool ReadGatePanel(unsigned row,float* output) const
	{
		if (!program || !gate || !output || row+16>gate->size()) return false;
		for (unsigned lane=0;lane<16;lane++) output[lane]=(*gate)[row+lane];
		return true;
	}

	bool ReadUpPanel(unsigned row,float* output) const
	{
		if (!program || !up || !output || row+16>up->size()) return false;
		for (unsigned lane=0;lane<16;lane++) output[lane]=(*up)[row+lane];
		return true;
	}

	bool WriteGatePanel(const DataflowOutputSegment& output,const float* values) const
	{
		if (!program || !gate || !writes || !values || output.firstRow+16>gate->size())
			return false;
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
		if (!assigned || (*writes)[output.firstRow/16]++) return false;
		for (unsigned lane=0;lane<16;lane++) (*gate)[output.firstRow+lane]=values[lane];
		return true;
	}
};

static float Reference(float gate,float up)
{
	const double value=double(gate);
	const double sigmoid=value>=0
		?1.0/(1.0+std::exp(-value))
		:std::exp(value)/(1.0+std::exp(value));
	return float((value*sigmoid)*double(up));
}

int main()
{
	DeviceLayout manifest{};
	manifest.magic=LayoutMagic;
	manifest.version=LayoutVersion;
	manifest.context=128;
	DataflowGraphWorkspace workspace{};
	Rows(manifest.activation[0],Ffn,0x04000000);
	Rows(manifest.activation[1],Ffn,0x06000000);
	Rows(workspace.space[DataflowGraphResidualWorkspaceIndex],Hidden,0x20000000);
	Rows(workspace.space[DataflowGraphUpWorkspaceIndex],Ffn,0x30000000);
	assert(workspace.Valid(manifest));
	DataflowSiluProgram program{};
	assert(DataflowSiluProgramBuilder::Build(manifest,workspace,0,
		TestRoutes,program));

	std::vector<float> gate(Ffn),up(Ffn),expected(Ffn);
	std::vector<unsigned> writes(Ffn/16,0);
	for (unsigned row=0;row<Ffn;row++) {
		gate[row]=float(int(row%2401)-1200)*0.01f;
		up[row]=float(int(row%257)-128)*0.125f;
		expected[row]=Reference(gate[row],up[row]);
	}
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowSiluInvocation invocation{};
		assert(DataflowSiluProgramBuilder::BindCore(program,core,invocation));
		float gatePage[16]{},upPage[16]{},outputPage[16]{};
		PanelIO io{&invocation,&gate,&up,&writes};
		assert(PioneerQwen35DataflowSiluWorker(invocation,gatePage,upPage,
			outputPage,io));
	}
	for (unsigned group=0;group<Ffn/16;group++) assert(writes[group]==1);
	for (unsigned row=0;row<Ffn;row++) {
		const float tolerance=2.5e-6f*(1.0f+std::fabs(expected[row]));
		assert(std::fabs(gate[row]-expected[row])<=tolerance);
	}
	return 0;
}
