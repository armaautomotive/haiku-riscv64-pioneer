/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#include "../PioneerQwen35DataflowFfnSequence.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
struct Authority {
    int fail=-1;unsigned calls=0;
    bool BuildResidual(unsigned mode,unsigned,DataflowResidualProgram& p) {p.mode=mode;return true;}
    bool BuildRms(unsigned,unsigned,unsigned,unsigned,DataflowRmsProgram&) {return true;}
    bool BuildProjection(unsigned,unsigned role,unsigned,unsigned,DataflowProjectionProgram& p,unsigned limit=DataflowCoreCount) {
        ++calls;p.type=role;
        if (role==Gate || role==Up) assert(limit==60);else assert(role==Down && limit==120);
        return int(role)!=fail;
    }
    bool BuildSilu(unsigned,DataflowSiluProgram&) {return true;}
};
struct Service {
    bool pairFail=false;unsigned pairCalls=0,singleCalls=0,siluCalls=0;
    int RunResidual(const DataflowResidualProgram&,int&) {return 0;}
    int RunRms(const DataflowRmsProgram&,int&) {return 0;}
    int RunProjection(const DataflowProjectionProgram& p,int&) {assert(p.type==Down);++singleCalls;return 0;}
    int RunIndependentProjectionPair(const DataflowProjectionProgram& gate,const DataflowProjectionProgram& up,int&) {
        assert(gate.type==Gate && up.type==Up);++pairCalls;return pairFail?1:0;
    }
    int RunSilu(const DataflowSiluProgram&,int&) {++siluCalls;return 0;}
};
int main() {
    auto* scratch=new DataflowProjectionProgram;int io=0;unsigned output=0;
    for (int mode=0;mode<4;mode++) {
        Authority authority;Service service;
        if (mode==1) authority.fail=Gate;
        if (mode==2) authority.fail=Up;
        if (mode==3) service.pairFail=true;
        output=0;
        const auto result=DataflowFfnSequence::Run(7,1,authority,service,io,output,*scratch);
        if (mode==0) assert(result==DataflowFfnSequenceComplete && output==0 && service.pairCalls==1
            && service.singleCalls==1 && service.siluCalls==1);
        else {
            assert(output==ActivationBuffers && !service.singleCalls && !service.siluCalls);
            if (mode==1) assert(result==DataflowFfnGateBuildFailed && !service.pairCalls);
            if (mode==2) assert(result==DataflowFfnUpBuildFailed && !service.pairCalls);
            if (mode==3) assert(result==DataflowFfnGateRunFailed && service.pairCalls==1);
        }
    }
    delete scratch;std::puts("PASS paired FFN sequence and fail-closed projection errors; SIMULATED ONLY");
}
