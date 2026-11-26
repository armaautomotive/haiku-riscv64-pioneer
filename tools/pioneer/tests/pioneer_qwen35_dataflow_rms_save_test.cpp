/* SPDX-License-Identifier: MIT */
// Verify the actual fused worker against the existing hidden RMS operation.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY 1
#define main q35_original_rms_worker_test_main
#include "pioneer_qwen35_dataflow_rms_worker_test.cpp"
#undef main
#include "../PioneerQwen35DataflowRmsSave.h"
#include "../PioneerQwen35DataflowDeviceStream.h"

struct MockFusedIO : MockRmsIO {
    float* saved;
    unsigned reads=0,writes=0;
    bool failSave=false;
    bool ReadInputPanel(unsigned row,float* out) {
        ++reads;return MockRmsIO::ReadInputPanel(row,out);
    }
    bool WriteResidualPanel(const DataflowOutputSegment& segment,const float* values) {
        if (failSave || !values || segment.firstRow>=Hidden || segment.rows!=16 || segment.bytes!=64) return false;
        ++writes;std::memcpy(saved+segment.firstRow,values,64);return true;
    }
};

int main() {
    (void)&q35_original_rms_worker_test_main;
    const DeviceLayout model=MakeLayout();
    DataflowRmsProgram rms{};
    assert(DataflowRmsProgramBuilder::Build(model,0,AttentionNorm,0,1,TestBankRoutes,rms));
    DataflowResidualProgram residual{};
    residual.mode=DataflowResidualSave;residual.activationSlot=0;
    residual.activation=model.activation[0];Rows(residual.residual,Hidden,BankBase+0x40000000);
    for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
        residual.bankRoute[noc][bank]=TestBankRoutes[noc][bank];
    assert(residual.Valid());
    float input[Hidden],weight[Hidden],expected[Hidden],actual[Hidden]{},saved[Hidden]{};
    for (unsigned i=0;i<Hidden;i++) {input[i]=float(int(i%67)-33)/96.0f;weight[i]=0.75f+float(i%23)/128.0f;}
    assert(Rms<Hidden>(input,weight,expected));
    DataflowRmsScaleRecord scale{};
    unsigned reads=0,writes=0;
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowRmsSaveInvocation invocation{};
        assert(DataflowRmsSaveBind(rms,residual,core,invocation));
        MockFusedIO io{{invocation.rms,input,weight,actual,scale,core!=0},saved};
        float cache[Hidden],inputPage[16],weightPage[16],outputPage[16];
        assert(PioneerQwen35DataflowRmsSaveWorker(invocation,1,cache,inputPage,weightPage,outputPage,io));
        if (!core) scale=io.record;
        reads+=io.reads;writes+=io.writes;
    }
    assert(reads==DataflowRmsGroups && writes==DataflowResidualGroups);
    assert(std::memcmp(expected,actual,sizeof(expected))==0 && std::memcmp(input,saved,sizeof(saved))==0);
    DataflowRmsSaveInvocation valid{};assert(DataflowRmsSaveBind(rms,residual,0,valid));
    DataflowStreamRecord record{};
    assert(DataflowStreamEncode(valid,99,1,record)
        && DataflowStreamRecordStructureValid(record,99,1,0)
        && record.header[5]==DataflowRmsSaveMagic && record.header[1]==6);
    auto damaged=record;damaged.payload[3]^=1;
    assert(!DataflowStreamRecordStructureValid(damaged,99,1,0));
    auto bad=valid;bad.residual.core=1;assert(!bad.Valid());
    bad=valid;bad.residual.bankRoute[1][0]^=1;assert(!bad.Valid());
    bad=valid;bad.residual.residual=valid.rms.output;assert(!bad.Valid());
    bad=valid;bad.residual.mode=DataflowResidualAdd;assert(!bad.Valid());
    bad=valid;bad.rms.layer=Layers;bad.rms.role=FinalNorm;assert(!bad.Valid());
    auto overlapping=residual;
    Rows(overlapping.residual,Hidden,BankBase+0x30000000);
    assert(overlapping.Valid());
    DataflowRmsSaveInvocation refused{};
    assert(!DataflowRmsSaveBind(rms,overlapping,0,refused) && !refused.magic);
    MockFusedIO io{{valid.rms,input,weight,actual},saved};io.failSave=true;
    float cache[Hidden],inputPage[16],weightPage[16],outputPage[16];unsigned stage=0;
    assert(!PioneerQwen35DataflowRmsSaveWorker(valid,1,cache,inputPage,weightPage,outputPage,io,&stage));
    assert(stage==DataflowRmsWorkerInputRead && io.reads==1 && io.writes==0);
    MockFusedIO weightFault{{valid.rms,input,weight,actual},saved};
    weightFault.failStage=DataflowRmsWorkerWeightRead;
    assert(!PioneerQwen35DataflowRmsSaveWorker(valid,1,cache,inputPage,weightPage,outputPage,weightFault,&stage));
    assert(stage==DataflowRmsWorkerWeightRead && weightFault.reads==1 && weightFault.writes==1);
    std::puts("PASS fused residual save and RMS for 120 simulated workers; NO HARDWARE");
}
