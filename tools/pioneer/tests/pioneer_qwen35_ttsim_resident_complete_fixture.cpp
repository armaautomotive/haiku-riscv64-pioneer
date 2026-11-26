/* SPDX-License-Identifier: MIT */
// Turn a verified, 21-step layer fixture into a 521-step prompt witness.
// Steps 22..521 are diagnostic copies, NOT model layers or inference output.
// This builds synthetic simulator inputs only and grants no hardware authority.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowResidentDispatch.h"
#include "../PioneerQwen35DataflowResidualProgram.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace PioneerQwen35;

static bool ReadExact(const std::string& path,void* data,size_t bytes) {
    FILE* f=std::fopen(path.c_str(),"rb");
    if (!f) return false;
    const bool okay=std::fread(data,1,bytes,f)==bytes && std::fgetc(f)==EOF && !std::ferror(f);
    return std::fclose(f)==0 && okay;
}
static bool WriteNew(const std::string& path,const void* data,size_t bytes) {
    FILE* f=std::fopen(path.c_str(),"wbx");
    if (!f) return false;
    const bool written=std::fwrite(data,1,bytes,f)==bytes;
    const bool closed=std::fclose(f)==0;
    if (!written || !closed) { std::remove(path.c_str());return false; }
    return true;
}
int main(int argc,char** argv) {
    if (argc!=3) { std::fprintf(stderr,"usage: %s source-fixture empty-output-directory\n",argv[0]);return 2; }
    const std::string input=argv[1],output=argv[2];
    constexpr unsigned originalCount=22,layerSteps=21,completeCount=DataflowGraphPromptSteps;
    // The final FFN tensor ends beyond 0x33000000. Keep the diagnostic
    // scratch outside all fixture weights and the disjoint stream leases.
    constexpr unsigned streamStart=0x40000000,streamStride=0x200000,scratchAddress=0x36000000;
    static_assert(completeCount==521 && completeCount*DataflowStreamRecordBytes<streamStride,
        "each synthetic worker stream must fit in its own aligned lease");
    static_assert(scratchAddress>=0x32000000U+19537920U
        && scratchAddress+Hidden*sizeof(float)<streamStart,
        "diagnostic scratch must not overwrite the FFN tensor or the streams");
    std::vector<DataflowGraphRunDescriptor> oldTemplates(DataflowCoreCount),newTemplates(DataflowCoreCount);
    std::vector<DataflowStreamRecord> oldRecords(DataflowCoreCount*originalCount);
    if (!ReadExact(input+"/templates.bin",oldTemplates.data(),oldTemplates.size()*sizeof(oldTemplates[0]))
        || !ReadExact(input+"/records.bin",oldRecords.data(),oldRecords.size()*sizeof(oldRecords[0]))) return 1;
    std::array<DataflowBarrierPacket,2> commands[2]{};
    std::vector<DataflowStreamRecord> records(DataflowCoreCount*completeCount);
    for (unsigned core=0;core<DataflowCoreCount;++core) {
        const auto& old=oldTemplates[core];
        if (!DataflowGraphRunValid(old,core) || old.token.position!=0 || old.ticket.count!=completeCount
            || old.binding.epoch!=99 || old.binding.core!=core || old.binding.bank!=0) return 1;
        for (unsigned i=0;i<layerSteps;++i) {
            const auto& source=oldRecords[core*originalCount+i];
            if (!DataflowStreamRecordStructureValid(source,99,i+1,core)) return 1;
            records[core*completeCount+i]=source;
        }
        DataflowResidualInvocation original{};
        DataflowDynamicRead(oldRecords[core*originalCount+1],original);
        if (!original.Valid() || original.magic!=DataflowResidualMagic || original.mode!=DataflowResidualSave) return 1;
        DataflowResidualProgram filler{};
        filler.mode=DataflowResidualSave;filler.activationSlot=original.activationSlot;
        std::memcpy(filler.bankRoute,original.bankRoute,sizeof(filler.bankRoute));
        filler.activation=original.activation;
        filler.residual={Hidden,4,1,16,{{0,scratchAddress,0,Hidden}}};
        DataflowResidualInvocation fillerCore{};
        if (!filler.Valid() || !DataflowResidualProgramBuilder::BindCore(filler,core,fillerCore)) return 1;
        for (unsigned i=layerSteps;i<completeCount;++i)
            if (!DataflowStreamEncode(fillerCore,99,i+1,records[core*completeCount+i])) return 1;
        auto binding=old.binding;binding.base=streamStart+core*streamStride;
        const Range lease={0,binding.base,DataflowGraphDecodeSteps*DataflowStreamRecordBytes};
        if (!DataflowGraphRunBuild(binding,lease,old.routes,old.token,old.ticket,
                old.triggerSequence,old.pollLimit,old.inputSlot,newTemplates[core])) return 1;
        if (!core) {
            DataflowResidentCommand one{};
            if (!DataflowResidentCommandBuild(newTemplates[core],one)) return 1;
            auto two=one;two.trigger=one.trigger+1;two.epoch=one.epoch+1;
            two.firstSequence=one.firstSequence+one.count;two.position=1;
            two.checksum=DataflowResidentCommandChecksum(two);
            if (!DataflowResidentCommandValid(two)) return 1;
            commands[0]={DataflowResidentCommandPayload(one),DataflowResidentCommandCommit(one)};
            commands[1]={DataflowResidentCommandPayload(two),DataflowResidentCommandCommit(two)};
        }
    }
    const std::array<std::string,5> names={output+"/templates.bin",output+"/records.bin",output+"/command.bin",
        output+"/command-next.bin",output+"/full-prompt.marker"};
    const std::array<const void*,5> data={newTemplates.data(),records.data(),commands[0].data(),commands[1].data(),""};
    const std::array<size_t,5> sizes={newTemplates.size()*sizeof(newTemplates[0]),records.size()*sizeof(records[0]),
        sizeof(commands[0]),sizeof(commands[1]),0};
    unsigned written=0;
    for (;written<names.size();++written) if (!WriteNew(names[written],data[written],sizes[written])) break;
    if (written!=names.size()) { for (unsigned i=0;i<written;++i) std::remove(names[i].c_str());return 1; }
    std::puts("prepared two simulator prompt commands: 21 model-layer steps plus 500 diagnostic copies; NO HARDWARE");
}
