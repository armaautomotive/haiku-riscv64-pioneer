/* SPDX-License-Identifier: MIT */
// Build dependent resident stream records using pinned 4B weights.
// Define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY to append sharded recurrence.
// This synthetic layout is for simulator testing only and grants no authority.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#include "../PioneerQwen35DataflowResidentDispatch.h"
#include "../PioneerQwen35DataflowResidualProgram.h"
#include "../PioneerQwen35DataflowRmsProgram.h"
#include "../PioneerQwen35DataflowSiluProgram.h"
#include "../PioneerQwen35DataflowProjectionPair.h"
#include "../PioneerQwen35DataflowActivationSharePlan.h"
#include "../PioneerQwen35TiledReader.h"
#include "../PioneerModelWeightCache.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace PioneerQwen35;
static bool SameFile(const struct stat& a,const struct stat& b)
{
    if (a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size
        || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime) return false;
#if defined(__APPLE__)
    return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec
        && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
    return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
static bool WriteNew(const std::string& path,const void* data,size_t size)
{
    FILE* out=std::fopen(path.c_str(),"wbx");
    if (!out) return false;
    const bool wrote=std::fwrite(data,1,size,out)==size;
    const bool closed=std::fclose(out)==0;
    if (!wrote || !closed) { std::remove(path.c_str());return false; }
    return true;
}
int main(int argc,char** argv)
{
    if (argc<3 || argc>5) { std::fprintf(stderr,"usage: %s pinned-model.gguf empty-directory [0|1 [42|261]]\n",argv[0]);return 2; }
    if (argc>=4 && std::strcmp(argv[3],"0") && std::strcmp(argv[3],"1")) return 2;
    if (argc==5 && std::strcmp(argv[4],"42") && std::strcmp(argv[4],"261")) return 2;
    const unsigned position=argc>=4 && !std::strcmp(argv[3],"1") ? 1 : 0;
    const unsigned tokenId=argc==5 && !std::strcmp(argv[4],"261") ? 261 : 42;
    const int fd=open(argv[1],O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
    if (fd<0) return 1;
    FILE* source=fdopen(fd,"rb");
    if (!source) { close(fd);return 1; }
    struct stat before{},after{};
    PioneerModelWeightCache::Digest digest{};std::string error;
    if (fstat(fd,&before) || !PioneerModelWeightCache::HashFile(fd,PioneerQwen35Gguf::FullBytes,digest,error)
        || PioneerModelWeightCache::Hex(digest)!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
        || fstat(fd,&after) || !SameFile(before,after)) { fclose(source);return 1; }
    PioneerQwen35Gguf sourceModel;PioneerQwen35TiledReader reader;
    if (!sourceModel.Open(source) || !reader.Bind(sourceModel)) { fclose(source);return 1; }
    const auto* tensor=sourceModel.Find("token_embd.weight");
    if (!tensor || tensor->type!=14 || tensor->inputs!=Hidden || tensor->outputs!=Vocabulary) {
        fclose(source);return 1;
    }
    const unsigned index=unsigned(tensor-sourceModel.Tensors().data());
    std::vector<uint8_t> weights(DataflowEmbeddingTileCount*DataflowQ6KEmbeddingTileBytes);
    std::array<uint8_t,4096> page{};
    for (unsigned p=0;p*page.size()<weights.size();p++) {
        if (!reader.ReadPage(sourceModel,index,tokenId&~15U,16,p,page.data(),page.size())) { fclose(source);return 1; }
        const size_t count=std::min(page.size(),weights.size()-p*page.size());
        std::memcpy(weights.data()+p*page.size(),page.data(),count);
    }
    const auto* normTensor=sourceModel.Find("blk.0.attn_norm.weight");
    std::vector<uint8_t> norm(Hidden*sizeof(float));
    if (!normTensor || normTensor->type!=0 || normTensor->inputs!=Hidden || normTensor->outputs!=1
        || !sourceModel.ReadPacked(unsigned(normTensor-sourceModel.Tensors().data()),0,
            unsigned(norm.size()),norm.data(),unsigned(norm.size()))) { fclose(source);return 1; }
    const char* names[]={"blk.0.attn_qkv.weight","blk.0.ssm_alpha.weight","blk.0.ssm_beta.weight"};
    const unsigned types[]={13,8,8},rows[]={8192,32,32},tiles[]={2816,576,576};
    const unsigned offsets[]={0,14417920,14512128},lengths[]={14417920,92160,92160};
    std::vector<uint8_t> projection(14604288);
    for (unsigned n=0;n<3;n++) {
        const auto* t=sourceModel.Find(names[n]);
        if (!t || t->type!=types[n] || t->inputs!=Hidden || t->outputs!=rows[n]) { fclose(source);return 1; }
        const unsigned tensorIndex=unsigned(t-sourceModel.Tensors().data());
        for (unsigned p=0;p*page.size()<lengths[n];p++) {
            if (!reader.ReadPage(sourceModel,tensorIndex,0,rows[n],p,page.data(),page.size())) {
                fclose(source);return 1;
            }
            const size_t count=std::min(page.size(),size_t(lengths[n])-p*page.size());
            std::memcpy(projection.data()+offsets[n]+p*page.size(),page.data(),count);
        }
    }
    const auto* gateTensor=sourceModel.Find("blk.0.attn_gate.weight");
    std::vector<uint8_t> gate(5898240);
    if (!gateTensor || gateTensor->type!=12 || gateTensor->inputs!=Hidden || gateTensor->outputs!=4096) {
        fclose(source);return 1;
    }
    const unsigned gateIndex=unsigned(gateTensor-sourceModel.Tensors().data());
    for (unsigned p=0;p*page.size()<gate.size();p++) {
        if (!reader.ReadPage(sourceModel,gateIndex,0,4096,p,page.data(),page.size())) {
            fclose(source);return 1;
        }
        const size_t count=std::min(page.size(),gate.size()-p*page.size());
        std::memcpy(gate.data()+p*page.size(),page.data(),count);
    }
    const auto* convTensor=sourceModel.Find("blk.0.ssm_conv1d.weight");
    std::vector<uint8_t> convWeights(ConvChannels*4*sizeof(float));
    if (!convTensor || convTensor->type!=0 || convTensor->inputs!=4 || convTensor->outputs!=ConvChannels
        || !sourceModel.ReadPacked(unsigned(convTensor-sourceModel.Tensors().data()),0,
            unsigned(convWeights.size()),convWeights.data(),unsigned(convWeights.size()))) { fclose(source);return 1; }
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
    const auto* outputTensor=sourceModel.Find("blk.0.ssm_out.weight");
    std::vector<uint8_t> outputWeights(7208960);
    if (!outputTensor || outputTensor->type!=13 || outputTensor->inputs!=DeltaInner
        || outputTensor->outputs!=Hidden) { fclose(source);return 1; }
    const unsigned outputIndex=unsigned(outputTensor-sourceModel.Tensors().data());
    for (unsigned p=0;p*page.size()<outputWeights.size();p++) {
        if (!reader.ReadPage(sourceModel,outputIndex,0,Hidden,p,page.data(),page.size())) {
            fclose(source);return 1;
        }
        const size_t count=std::min(page.size(),outputWeights.size()-p*page.size());
        std::memcpy(outputWeights.data()+p*page.size(),page.data(),count);
    }
    const auto* ffnNormTensor=sourceModel.Find("blk.0.post_attention_norm.weight");
    std::vector<uint8_t> ffnNorm(Hidden*sizeof(float));
    if (!ffnNormTensor || ffnNormTensor->type!=0 || ffnNormTensor->inputs!=Hidden || ffnNormTensor->outputs!=1
        || !sourceModel.ReadPacked(unsigned(ffnNormTensor-sourceModel.Tensors().data()),0,
            unsigned(ffnNorm.size()),ffnNorm.data(),unsigned(ffnNorm.size()))) { fclose(source);return 1; }
    const char* ffnNames[]={"blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"};
    const unsigned ffnInputs[]={Hidden,Hidden,Ffn},ffnRows[]={Ffn,Ffn,Hidden};
    const unsigned ffnTypes[]={12,12,14},ffnSizes[]={13271040,13271040,19537920};
    const unsigned ffnOffsets[]={0,13271040,26542080};
    std::vector<uint8_t> ffnWeights(46080000);
    for (unsigned n=0;n<3;n++) {
        const auto* t=sourceModel.Find(ffnNames[n]);
        if (!t || t->type!=ffnTypes[n] || t->inputs!=ffnInputs[n] || t->outputs!=ffnRows[n]) { fclose(source);return 1; }
        const unsigned tensorIndex=unsigned(t-sourceModel.Tensors().data());
        for (unsigned p=0;p*page.size()<ffnSizes[n];p++) {
            if (!reader.ReadPage(sourceModel,tensorIndex,0,ffnRows[n],p,page.data(),page.size())) {
                fclose(source);return 1;
            }
            const size_t count=std::min(page.size(),size_t(ffnSizes[n])-p*page.size());
            std::memcpy(ffnWeights.data()+ffnOffsets[n]+p*page.size(),page.data(),count);
        }
    }
    std::vector<uint8_t> deltaParams(192*sizeof(float));
    const char* deltaNames[]={"blk.0.ssm_a","blk.0.ssm_dt.bias","blk.0.ssm_norm.weight"};
    const unsigned deltaSizes[]={32,32,128};
    unsigned deltaOffset=0;
    for (unsigned i=0;i<3;i++) {
        const auto* t=sourceModel.Find(deltaNames[i]);
        if (!t || t->type!=0 || t->inputs!=deltaSizes[i] || t->outputs!=1
            || !sourceModel.ReadPacked(unsigned(t-sourceModel.Tensors().data()),0,deltaSizes[i]*sizeof(float),
                deltaParams.data()+deltaOffset,deltaSizes[i]*sizeof(float))) { fclose(source);return 1; }
        deltaOffset+=deltaSizes[i]*sizeof(float);
    }
#endif
    if (!sourceModel.Stable() || fstat(fd,&after) || !SameFile(before,after)) { fclose(source);return 1; }
    fclose(source);

    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    DeviceLayout model{};model.magic=LayoutMagic;model.version=LayoutVersion;model.context=32;
    model.tied.type=14;model.tied.inputs=Hidden;
    model.tied.place={Vocabulary,2120,1,16,{{0,0x10000000,0,Vocabulary}}};
    model.activation[0]={Ffn,4,1,16,{{0,0x20000000,0,Ffn}}};
    model.activation[1]={Ffn,4,1,16,{{0,0x20020000,0,Ffn}}};
    model.layers[0].weights[AttentionNorm].type=0;
    model.layers[0].weights[AttentionNorm].inputs=Hidden;
    model.layers[0].weights[AttentionNorm].place={1,Hidden*4,1,1,{{0,0x20030000,0,1}}};
    DataflowDynamicTokenContext token{};DataflowEmbeddingProgram embedding{};
    if (!DataflowDynamicTokenContext::Build(model,tokenId,position,token)
        || !DataflowEmbeddingProgramBuilder::Build(model,tokenId,0,routes,embedding)
        || embedding.weightBank!=0 || embedding.weightBase!=0x10000000+(tokenId&~15U)*2120
        || embedding.lane!=(tokenId&15U)) return 1;
    DataflowResidualProgram residual{};residual.mode=DataflowResidualSave;residual.activationSlot=0;
    std::memcpy(residual.bankRoute,routes,sizeof(routes));residual.activation=model.activation[0];
    residual.residual={Hidden,4,1,16,{{0,0x20010000,0,Hidden}}};
    if (!residual.Valid()) return 1;
    DataflowRmsProgram rms{};
    if (!DataflowRmsProgramBuilder::Build(model,0,AttentionNorm,0,1,routes,rms)) return 1;
    auto programs=std::make_unique<DataflowProjectionProgram[]>(3);
    const unsigned outputBase[]={0x21000000,0x21010000,0x21020000};
    for (unsigned n=0;n<3;n++) {
        const unsigned blocks=Hidden/(types[n]==8?32:256);
        TensorSpace weight{};weight.type=types[n];weight.inputs=Hidden;
        weight.place={rows[n],blocks*(tiles[n]/16),1,16,{{0,0x28000000+offsets[n],0,rows[n]}}};
        RowSpace output{rows[n],4,1,16,{{0,outputBase[n],0,rows[n]}}};
        PioneerQ35ProjectionProgram schedule{};
        if (!DataflowProgramBuilder::Build(weight,model.activation[1],Hidden,output,routes,
                programs[n],schedule,0,n==0?116:120)) return 1;
    }
    if (programs[0].activeCores!=116 || programs[1].activeCores!=2 || programs[2].activeCores!=2
        || !DataflowProjectionTriple::Compatible(programs[0],programs[1],programs[2])) return 1;
    auto projections=std::make_unique<DataflowCoreInvocation[]>(DataflowCoreCount);
    auto sharing=std::make_unique<DataflowActivationSharePlan>();
    for (unsigned core=0;core<DataflowCoreCount;core++)
        if (!DataflowProjectionTriple::BindCore(programs[0],programs[1],programs[2],core,projections[core])
            || !projections[core].assignment.active || !sharing->Add(projections[core])) return 1;
    if (!sharing->Seal() || sharing->Groups()!=2) return 1;
    for (unsigned core=0;core<DataflowCoreCount;core++)
        if (!sharing->Get(core,projections[core].sharing) || !projections[core].Valid()) return 1;
    TensorSpace gateWeight{};gateWeight.type=12;gateWeight.inputs=Hidden;
    gateWeight.place={4096,1440,1,16,{{0,0x29000000,0,4096}}};
    RowSpace gateOutput{4096,4,1,16,{{0,0x21030000,0,4096}}};
    DataflowProjectionProgram gateProgram{};PioneerQ35ProjectionProgram gateSchedule{};
    if (!DataflowProgramBuilder::Build(gateWeight,model.activation[1],Hidden,gateOutput,routes,
            gateProgram,gateSchedule) || gateProgram.activeCores!=DataflowCoreCount) return 1;
    auto gateInvocations=std::make_unique<DataflowCoreInvocation[]>(DataflowCoreCount);
    auto gateSharing=std::make_unique<DataflowActivationSharePlan>();
    for (unsigned core=0;core<DataflowCoreCount;core++)
        if (!DataflowProgramBuilder::BindCore(gateProgram,core,gateInvocations[core])
            || !gateInvocations[core].assignment.active || !gateSharing->Add(gateInvocations[core])) return 1;
    if (!gateSharing->Seal() || gateSharing->Groups()!=1) return 1;
    for (unsigned core=0;core<DataflowCoreCount;core++)
        if (!gateSharing->Get(core,gateInvocations[core].sharing) || !gateInvocations[core].Valid()) return 1;
    DataflowConvProgram conv{};conv.layer=0;conv.position=position;
    std::memcpy(conv.bankRoute,routes,sizeof(routes));
    conv.qkv={ConvChannels,4,1,16,{{0,0x21000000,0,ConvChannels}}};
    conv.weights.type=0;conv.weights.inputs=4;
    conv.weights.place={ConvChannels,16,1,1,{{0,0x22000000,0,ConvChannels}}};
    conv.historyRead={ConvChannels,12,1,1,{{0,0x22040000,0,ConvChannels}}};
    conv.historyWrite={ConvChannels,12,1,1,{{0,0x22060000,0,ConvChannels}}};
    if (!conv.Valid()) return 1;
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
    DataflowDeltaProgram delta{};delta.layer=0;delta.position=position;
    std::memcpy(delta.bankRoute,routes,sizeof(routes));
    delta.qkv=conv.qkv;delta.gate=gateOutput;
    delta.alpha={DeltaHeads,4,1,16,{{0,0x21010000,0,DeltaHeads}}};
    delta.beta={DeltaHeads,4,1,16,{{0,0x21020000,0,DeltaHeads}}};
    delta.negativeA.inputs=DeltaHeads;delta.negativeA.place={1,DeltaHeads*4,1,1,{{0,0x23000000,0,1}}};
    delta.deltaBias.inputs=DeltaHeads;delta.deltaBias.place={1,DeltaHeads*4,1,1,{{0,0x23001000,0,1}}};
    delta.deltaNorm.inputs=DeltaWidth;delta.deltaNorm.place={1,DeltaWidth*4,1,1,{{0,0x23002000,0,1}}};
    delta.stateRead={DeltaHeads,DeltaStateWords*4,1,1,{{0,0x24000000,0,DeltaHeads}}};
    delta.stateWrite={DeltaHeads,DeltaStateWords*4,1,1,{{0,0x25000000,0,DeltaHeads}}};
    if (!delta.Valid()) return 1;
    // Preserve the RMS output for independent checking before the actual
    // recurrent output projection overwrites activation slot 1.
    DataflowResidualProgram rmsCheckpoint{};
    rmsCheckpoint.mode=DataflowResidualSave;rmsCheckpoint.activationSlot=1;
    std::memcpy(rmsCheckpoint.bankRoute,routes,sizeof(routes));
    rmsCheckpoint.activation=model.activation[1];
    rmsCheckpoint.residual={Hidden,4,1,16,{{0,0x21040000,0,Hidden}}};
    if (!rmsCheckpoint.Valid()) return 1;
    TensorSpace recurrentWeight{};recurrentWeight.type=13;recurrentWeight.inputs=DeltaInner;
    recurrentWeight.place={Hidden,2816,1,16,{{0,0x2a000000,0,Hidden}}};
    DataflowProjectionProgram recurrent{};PioneerQ35ProjectionProgram recurrentSchedule{};
    if (!DataflowProgramBuilder::Build(recurrentWeight,conv.qkv,DeltaInner,model.activation[1],routes,
            recurrent,recurrentSchedule,DataflowProjectionRecurrentVBase)
        || recurrent.activeCores!=DataflowCoreCount) return 1;
    auto recurrentInvocations=std::make_unique<DataflowCoreInvocation[]>(DataflowCoreCount);
    auto recurrentSharing=std::make_unique<DataflowActivationSharePlan>();
    for (unsigned core=0;core<DataflowCoreCount;core++)
        if (!DataflowProgramBuilder::BindCore(recurrent,core,recurrentInvocations[core])
            || !recurrentInvocations[core].assignment.active || !recurrentSharing->Add(recurrentInvocations[core])) return 1;
    if (!recurrentSharing->Seal() || recurrentSharing->Groups()!=1) return 1;
    for (unsigned core=0;core<DataflowCoreCount;core++)
        if (!recurrentSharing->Get(core,recurrentInvocations[core].sharing) || !recurrentInvocations[core].Valid()) return 1;
    auto makeResidual=[&](unsigned mode,unsigned slot,unsigned destination) {
        DataflowResidualProgram p{};p.mode=mode;p.activationSlot=slot;
        std::memcpy(p.bankRoute,routes,sizeof(routes));p.activation=model.activation[slot];
        p.residual={Hidden,4,1,16,{{0,destination,0,Hidden}}};return p;
    };
    auto recurrentCheckpoint=makeResidual(DataflowResidualSave,1,0x21060000);
    auto embeddingCheckpoint=makeResidual(DataflowResidualSave,0,0x21080000);
    auto attentionAdd=makeResidual(DataflowResidualAdd,1,0x20010000);
    auto ffnSave=makeResidual(DataflowResidualSave,1,0x20010000);
    auto ffnNormCheckpoint=makeResidual(DataflowResidualSave,0,0x21070000);
    auto finalAdd=makeResidual(DataflowResidualAdd,0,0x20010000);
    if (!recurrentCheckpoint.Valid() || !embeddingCheckpoint.Valid() || !attentionAdd.Valid()
        || !ffnSave.Valid() || !ffnNormCheckpoint.Valid() || !finalAdd.Valid()) return 1;
    model.layers[0].weights[PostAttentionNorm].type=0;
    model.layers[0].weights[PostAttentionNorm].inputs=Hidden;
    model.layers[0].weights[PostAttentionNorm].place={1,Hidden*4,1,1,{{0,0x20050000,0,1}}};
    DataflowRmsProgram postRms{};
    if (!DataflowRmsProgramBuilder::Build(model,0,PostAttentionNorm,1,0,routes,postRms)) return 1;
    const RowSpace upOutput{Ffn,4,1,16,{{0,0x21050000,0,Ffn}}};
    auto ffnPrograms=std::make_unique<DataflowProjectionProgram[]>(3);
    auto ffnInvocations=std::make_unique<DataflowCoreInvocation[]>(3*DataflowCoreCount);
    const unsigned weightBases[]={0x30000000,0x31000000,0x32000000};
    for (unsigned n=0;n<3;n++) {
        TensorSpace weight{};weight.type=ffnTypes[n];weight.inputs=ffnInputs[n];
        weight.place={ffnRows[n],(ffnInputs[n]/256)*(n==2?212:144),1,16,{{0,weightBases[n],0,ffnRows[n]}}};
        const RowSpace& input=n==2?model.activation[1]:model.activation[0];
        const RowSpace& output=n==0?model.activation[1]:n==1?upOutput:model.activation[0];
        PioneerQ35ProjectionProgram schedule{};
        if (!DataflowProgramBuilder::Build(weight,input,ffnInputs[n],output,routes,ffnPrograms[n],schedule)
            || ffnPrograms[n].activeCores!=DataflowCoreCount) return 1;
        auto plan=std::make_unique<DataflowActivationSharePlan>();
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            auto& invocation=ffnInvocations[n*DataflowCoreCount+core];
            if (!DataflowProgramBuilder::BindCore(ffnPrograms[n],core,invocation)
                || !invocation.assignment.active || !plan->Add(invocation)) return 1;
        }
        if (!plan->Seal() || plan->Groups()!=1) return 1;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            auto& invocation=ffnInvocations[n*DataflowCoreCount+core];
            if (!plan->Get(core,invocation.sharing) || !invocation.Valid()) return 1;
        }
    }
    DataflowSiluProgram silu{};silu.gateSlot=1;silu.gate=model.activation[1];silu.up=upOutput;
    std::memcpy(silu.bankRoute,routes,sizeof(routes));
    if (!silu.Valid()) return 1;
    constexpr unsigned recordCount=22;
    constexpr unsigned postRmsStep=5;
#else
    constexpr unsigned recordCount=7;
    constexpr unsigned postRmsStep=4;
#endif
    std::vector<DataflowGraphRunDescriptor> descriptors(DataflowCoreCount);
    std::vector<DataflowStreamRecord> records(DataflowCoreCount*recordCount);
    std::array<DataflowBarrierPacket,2> command{};
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        const unsigned base=0x04000000+core*0x100000;
        const DataflowStreamBinding binding={DataflowStreamBindingMagic,DataflowStreamVersion,
            99,core,0,routes[0][0],base,DataflowGraphDecodeSteps};
        const Range lease={0,base,DataflowGraphDecodeSteps*DataflowStreamRecordBytes};
        auto& descriptor=descriptors[core];
        if (!DataflowGraphRunBuild(binding,lease,routes,token,{recordCount,1,DataflowGraphPromptSteps},
                1,1000000,0,descriptor)) return 1;
        if (!core) {
            DataflowResidentCommand dynamic{};
            if (!DataflowResidentCommandBuild(descriptor,dynamic)) return 1;
            command={DataflowResidentCommandPayload(dynamic),DataflowResidentCommandCommit(dynamic)};
        }
        DataflowEmbeddingInvocation e{};DataflowResidualInvocation r{};DataflowRmsInvocation n{};
        DataflowConvInvocation c{};
        if (!DataflowEmbeddingProgramBuilder::BindCore(embedding,core,e)
            || !DataflowStreamEncode(e,99,1,records[core*recordCount])
            || !DataflowResidualProgramBuilder::BindCore(residual,core,r)
            || !DataflowStreamEncode(r,99,2,records[core*recordCount+1])
            || !DataflowRmsProgramBuilder::BindCore(rms,core,n)
            || !DataflowStreamEncode(n,99,3,records[core*recordCount+2])
            || !DataflowStreamEncode(projections[core],99,postRmsStep,records[core*recordCount+postRmsStep-1])
            || !DataflowStreamEncode(gateInvocations[core],99,postRmsStep+1,records[core*recordCount+postRmsStep])
            || !DataflowConvProgramBuilder::BindCore(conv,core,c)
            || !DataflowStreamEncode(c,99,postRmsStep+2,records[core*recordCount+postRmsStep+1])) return 1;
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
        DataflowResidualInvocation checkpoint{};
        DataflowDeltaShardInvocation update{},normalize{};
        if (!DataflowResidualProgramBuilder::BindCore(rmsCheckpoint,core,checkpoint)
            || !DataflowStreamEncode(checkpoint,99,4,records[core*recordCount+3])
            || !DataflowDeltaShardBind(delta,core,DataflowDeltaShardUpdate,update)
            || !DataflowDeltaShardBind(delta,core,DataflowDeltaShardNormalize,normalize)
            || !DataflowStreamEncode(update,99,8,records[core*recordCount+7])
            || !DataflowStreamEncode(normalize,99,9,records[core*recordCount+8])
            || !DataflowStreamEncode(recurrentInvocations[core],99,10,records[core*recordCount+9])) return 1;
        auto emitResidual=[&](const DataflowResidualProgram& program,unsigned step) {
            DataflowResidualInvocation invocation{};
            return DataflowResidualProgramBuilder::BindCore(program,core,invocation)
                && DataflowStreamEncode(invocation,99,step,records[core*recordCount+step-1]);
        };
        DataflowRmsInvocation postNorm{};DataflowSiluInvocation activation{};
        if (!emitResidual(recurrentCheckpoint,11) || !emitResidual(embeddingCheckpoint,12)
            || !emitResidual(attentionAdd,13) || !emitResidual(ffnSave,14)
            || !DataflowRmsProgramBuilder::BindCore(postRms,core,postNorm)
            || !DataflowStreamEncode(postNorm,99,15,records[core*recordCount+14])
            || !emitResidual(ffnNormCheckpoint,16)
            || !DataflowStreamEncode(ffnInvocations[core],99,17,records[core*recordCount+16])
            || !DataflowStreamEncode(ffnInvocations[DataflowCoreCount+core],99,18,records[core*recordCount+17])
            || !DataflowSiluProgramBuilder::BindCore(silu,core,activation)
            || !DataflowStreamEncode(activation,99,19,records[core*recordCount+18])
            || !DataflowStreamEncode(ffnInvocations[2*DataflowCoreCount+core],99,20,records[core*recordCount+19])
            || !emitResidual(finalAdd,21)) return 1;
#endif
        // The final zero record deliberately terminates after the last barrier.
    }
    std::vector<float> history(ConvChannels*3);
    for (size_t i=0;i<history.size();i++) history[i]=float(int((i*7)%29)-14)*0.0625f;
    const std::array<uint8_t,4> positionBytes={uint8_t(position),0,0,0};
    const std::string directory=argv[2];
    const std::array<std::string,10> paths={directory+"/templates.bin",directory+"/command.bin",
        directory+"/records.bin",directory+"/weights.bin",directory+"/norm.bin",directory+"/projection.bin",directory+"/gate.bin",directory+"/conv.bin",
        directory+"/history.bin",directory+"/position.bin"};
    const std::array<const void*,10> data={descriptors.data(),command.data(),records.data(),weights.data(),norm.data(),projection.data(),gate.data(),convWeights.data(),history.data(),positionBytes.data()};
    const std::array<size_t,10> size={descriptors.size()*sizeof(descriptors[0]),sizeof(command),
        records.size()*sizeof(records[0]),weights.size(),norm.size(),projection.size(),gate.size(),convWeights.size(),
        history.size()*sizeof(history[0]),positionBytes.size()};
    unsigned written=0;
    for (;written<paths.size();written++) if (!WriteNew(paths[written],data[written],size[written])) break;
    if (written!=paths.size()) {
        for (unsigned i=0;i<written;i++) std::remove(paths[i].c_str());
        return 1;
    }
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
    std::vector<float> oldState(DeltaHeads*DeltaStateWords);
    for (size_t i=0;i<oldState.size();i++) oldState[i]=float(int((i*11)%97)-48)/1024.0f;
    const std::string paramsPath=directory+"/delta-params.bin",statePath=directory+"/delta-state.bin";
    const std::string outputPath=directory+"/recurrent-output-weight.bin";
    const bool paramsWritten=WriteNew(paramsPath,deltaParams.data(),deltaParams.size());
    const bool stateWritten=paramsWritten && WriteNew(statePath,oldState.data(),oldState.size()*sizeof(float));
    const bool outputWritten=stateWritten && WriteNew(outputPath,outputWeights.data(),outputWeights.size());
    const std::string ffnNormPath=directory+"/ffn-norm.bin",ffnWeightsPath=directory+"/ffn-weights.bin";
    const bool ffnNormWritten=outputWritten && WriteNew(ffnNormPath,ffnNorm.data(),ffnNorm.size());
    const bool ffnWritten=ffnNormWritten && WriteNew(ffnWeightsPath,ffnWeights.data(),ffnWeights.size());
    if (!ffnWritten) {
        if (ffnNormWritten) std::remove(ffnNormPath.c_str());
        if (outputWritten) std::remove(outputPath.c_str());
        if (stateWritten) std::remove(statePath.c_str());
        if (paramsWritten) std::remove(paramsPath.c_str());
        for (const auto& path:paths) std::remove(path.c_str());
        return 1;
    }
    std::puts("prepared authenticated 4B recurrent layer and four diagnostic copies; NO HARDWARE");
#else
    std::puts("prepared authenticated 4B weights and six synthetic resident graph steps; NO HARDWARE");
#endif
    return 0;
}
