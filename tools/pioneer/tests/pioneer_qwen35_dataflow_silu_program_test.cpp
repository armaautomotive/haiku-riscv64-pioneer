/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../PioneerQwen35DataflowSiluProgram.h"

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
	assert(workspace.Valid(manifest));

	DataflowSiluProgram program{};
	assert(DataflowSiluProgramBuilder::Build(manifest,workspace,0,
		TestRoutes,program));
	assert(program.Valid() && program.gateSlot==0);
	unsigned covered=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowSiluInvocation invocation{};
		assert(DataflowSiluProgramBuilder::BindCore(program,core,invocation));
		assert(invocation.Valid());
		assert(invocation.segmentCount==DataflowSiluGroupsForCore(core));
		for (unsigned i=0;i<invocation.segmentCount;i++) {
			const auto& part=invocation.segment[i];
			assert(part.firstRow==covered && part.rows==16
				&& part.bytes==16*sizeof(float));
			Range expected{};
			assert(program.gate.Resolve(part.firstRow,16,0,part.bytes,expected));
			assert(part.bank==expected.bank && part.address==expected.address);
			covered+=part.rows;
		}
	}
	assert(covered==Ffn);

	DataflowSiluInvocation invocation{};
	assert(DataflowSiluProgramBuilder::BindCore(program,0,invocation));
	DataflowSiluInvocation forged=invocation;
	forged.segment[0].address+=4096;
	assert(!forged.Valid());
	forged=invocation;
	forged.gateSlot=ActivationBuffers;
	assert(!forged.Valid());
	forged=invocation;
	forged.bankRoute[0][0]=forged.bankRoute[0][1];
	assert(!forged.Valid());
	DataflowSiluProgram invalidProgram=program;
	invalidProgram.gate=workspace.space[DataflowGraphUpWorkspaceIndex];
	assert(!invalidProgram.Valid());
	return 0;
}
