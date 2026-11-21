/* SPDX-License-Identifier: MIT */
#define PIONEER_Q35_OPERATOR_TEST 1
#include "../PioneerQwen35DataflowBlackholeArgmaxWorker.h"

#include <assert.h>
#include <cmath>
#include <cstdint>
#include <limits>

using namespace PioneerQwen35;

static const unsigned TestBankRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

struct MockLogitsIO {
	unsigned calls=0;
	bool injectNonFinite=false;

	bool ReadLogits(const Range& range,unsigned route,float* local,unsigned bytes)
	{
		if (range.bank!=0 || route!=TestBankRoutes[0][0] || !local
			|| range.address<BankBase+0x10000000 || (bytes&63)) return false;
		const unsigned first=(range.address-(BankBase+0x10000000))/sizeof(float);
		const unsigned rows=bytes/sizeof(float);
		if (first+rows>Vocabulary || rows>DataflowNocMaxTransfer/sizeof(float))
			return false;
		for (unsigned i=0;i<rows;i++) {
			const unsigned token=first+i;
			local[i]=token==12345 || token==12346?7.0f:-float(token%97)/100.0f;
			if (injectNonFinite && token==90000)
				local[i]=std::numeric_limits<float>::quiet_NaN();
		}
		calls++;
		return true;
	}
};

static DataflowArgmaxProgram MakeProgram()
{
	DataflowArgmaxProgram program{};
	program.magic=DataflowArgmaxMagic;
	program.version=DataflowArgmaxVersion;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		program.bankRoute[noc][bank]=TestBankRoutes[noc][bank];
	program.logits.rows=Vocabulary;
	program.logits.rowBytes=sizeof(float);
	program.logits.count=1;
	program.logits.unit=16;
	program.logits.parts[0]={0,BankBase+0x10000000,0,Vocabulary};
	assert(program.Valid());
	return program;
}

int main()
{
	const DataflowArgmaxProgram program=MakeProgram();
	DataflowArgmaxInvocation core0{},core1{};
	assert(DataflowArgmaxProgramBuilder::BindCore(program,0,core0));
	assert(DataflowArgmaxProgramBuilder::BindCore(program,1,core1));
	assert(core0.active && !core1.active);

	float scratch[DataflowNocMaxTransfer/sizeof(float)];
	DataflowWorkerMailbox mailbox{};
	MockLogitsIO io{};
	assert(PioneerQwen35DataflowArgmaxWorker(core0,7,scratch,mailbox,io));
	assert(io.calls>1);
	DataflowArgmaxResult result{};
	assert(DataflowArgmaxReadResult(mailbox,7,result));
	assert(result.token==12345 && result.sequence==7);

	MockLogitsIO inactiveIO{};
	assert(PioneerQwen35DataflowArgmaxWorker(core1,8,scratch,mailbox,inactiveIO));
	assert(inactiveIO.calls==0);

	MockLogitsIO badIO{};
	badIO.injectNonFinite=true;
	DataflowWorkerMailbox badMailbox{};
	assert(!PioneerQwen35DataflowArgmaxWorker(core0,9,scratch,badMailbox,badIO));
	assert(badIO.calls>0);
	return 0;
}
