/* SPDX-License-Identifier: MIT */
// Authenticated 4B layer prefix through the actual full-graph recorder.
// For a partial prefix, the next record is deliberately zeroed. Simulator only.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowResidentDispatch.h"
#include "../PioneerQwen35DataflowStreamRecorder.h"
#include "../PioneerQwen35Preload.h"
#include "../PioneerModelWeightCache.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
using namespace PioneerQwen35;
static bool SameFile(const struct stat& a,const struct stat& b) {
    if (a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size
        || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime) return false;
#if defined(__APPLE__)
    return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
    return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
static bool WriteNew(const std::string& path,const void* data,size_t bytes) {
    FILE* file=std::fopen(path.c_str(),"wbx");if (!file) return false;
    bool okay=std::fwrite(data,1,bytes,file)==bytes;
    if (std::fclose(file)) okay=false;
    if (!okay) std::remove(path.c_str());return okay;
}
struct Sink {
    Sink(unsigned layers,bool decode):targetLayers(layers),planSteps(decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps),
        selectToken(decode),records(DataflowCoreCount*planSteps) {}
    unsigned targetLayers,planSteps;bool selectToken;unsigned adds=0,executed=0;
    std::vector<DataflowStreamRecord> records;
    RowSpace final{};unsigned count=0;bool sealed=false;
    bool Begin(const DataflowStreamPlan&) {return true;}
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned step,const DataflowStreamRecord& record) {
        if (binding.core!=count%DataflowCoreCount || step!=count/DataflowCoreCount
            || !DataflowStreamRecordStructureValid(record,binding.epoch,step+1,binding.core)) return false;
        ++count;
        if (binding.core==0 && !executed && record.header[5]==DataflowResidualMagic) {
            DataflowResidualInvocation invocation{};std::memcpy(&invocation,record.payload,sizeof(invocation));
            if (!invocation.Valid()) return false;
            if (invocation.mode==DataflowResidualAdd && ++adds==targetLayers*2) {
                executed=step+1;final=invocation.activation;
            }
        }
        if (!executed || step<executed || selectToken) records[binding.core*planSteps+step]=record;
        return true;
    }
    bool Seal(const DataflowStreamPlan& plan) {sealed=count==plan.count*DataflowCoreCount && executed;return sealed;}
    void Abort() {sealed=false;}
};
int main(int argc,char** argv) {
    if (argc<3 || argc>5) {std::fprintf(stderr,"usage: %s pinned.gguf empty-output-directory [layers [decode]]\n",argv[0]);return 2;}
    char* end=nullptr;const unsigned long parsed=argc>=4?std::strtoul(argv[3],&end,10):1;
    if ((argc>=4 && (!argv[3][0] || *end)) || !parsed || parsed>Layers) return 2;
    const unsigned layers=unsigned(parsed);
    const bool decode=argc==5 && !std::strcmp(argv[4],"decode");
    if ((argc==5 && !decode) || (decode && layers!=Layers)) return 2;
    const int fd=open(argv[1],O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if (fd<0) return 1;
    FILE* file=fdopen(fd,"rb");if (!file) {close(fd);return 1;}
    PioneerModelWeightCache::Digest digest{};std::string error;
    struct stat before{},after{};
    if (fstat(fd,&before) || !PioneerModelWeightCache::HashFile(fd,PioneerQwen35Gguf::FullBytes,digest,error)
        || PioneerModelWeightCache::Hex(digest)!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
        || fstat(fd,&after) || !SameFile(before,after)) {std::fclose(file);return 1;}
    PioneerQwen35Gguf source;PioneerQwen35PackedPlan packed;
    if (!source.Open(file) || !packed.Build(source,32,true)) {std::fclose(file);return 1;}
    auto model=std::make_unique<DeviceLayout>();auto workspace=std::make_unique<DataflowGraphWorkspace>();Range rope{};
    if (!BuildDeviceLayout(source,packed,32,*model) || !ResolveRope(*model,packed,rope)
        || !DataflowGraphWorkspaceBuilder::Build(packed,*model,*workspace)) {std::fclose(file);return 1;}
    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    auto live=std::make_unique<DataflowProjectionAuthority>();auto candidate=std::make_unique<DataflowProjectionAuthority>();
    auto plan=std::make_unique<DataflowStreamPlan>();auto sink=std::make_unique<Sink>(layers,decode);
    auto scratch=std::make_unique<DataflowStreamRecord>();auto projection=std::make_unique<DataflowProjectionProgram>();unsigned stage=0;
    if (!live->BindValidatedModel(*model,*workspace,routes,rope)
        || !DataflowStreamPlanBuilder::Build(packed,*model,*workspace,rope,routes,99,sink->planSteps,*plan)
        || !DataflowStreamRecording::Build(packed,*live,*candidate,*plan,routes,0,42,0,decode,*sink,*scratch,*projection,stage)
        || !sink->sealed || stage!=DataflowDecoderSequenceComplete || !sink->final.Valid()) {std::fclose(file);return 1;}
    DataflowDynamicTokenContext token{};if (!DataflowDynamicTokenContext::Build(*model,42,0,token)) return 1;
    std::vector<DataflowGraphRunDescriptor> descriptors(DataflowCoreCount);
    std::array<DataflowBarrierPacket,2> command{};
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        if (!DataflowGraphRunBuild(plan->binding[core],plan->lease[core],routes,token,
                {5,1,sink->planSteps},1,1000000,0,descriptors[core])) return 1;
        if (!core) {DataflowResidentCommand c{};if (!DataflowResidentCommandBuild(descriptors[core],c)) return 1;
            command={DataflowResidentCommandPayload(c),DataflowResidentCommandCommit(c)};}
    }
    const std::string dir=argv[2];
    const unsigned recordCount=decode?sink->planSteps:sink->executed+unsigned(layers<Layers);
    std::vector<DataflowStreamRecord> compact(DataflowCoreCount*recordCount);
    for (unsigned core=0;core<DataflowCoreCount;core++)
        std::memcpy(compact.data()+core*recordCount,
            sink->records.data()+core*sink->planSteps,(decode?sink->planSteps:sink->executed)*sizeof(compact[0]));
    if (!WriteNew(dir+"/templates.bin",descriptors.data(),descriptors.size()*sizeof(descriptors[0]))
        || !WriteNew(dir+"/records.bin",compact.data(),compact.size()*sizeof(compact[0]))
        || !WriteNew(dir+"/command.bin",command.data(),sizeof(command))) return 1;
#ifdef PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY
    if (!WriteNew(dir+"/flash-hybrid.marker","",0)) return 1;
#endif
#ifdef PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY
    if (!WriteNew(dir+"/ffn-pair.marker","",0)) return 1;
#endif
#ifdef PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY
    if (!WriteNew(dir+"/fused-rms-save.marker","",0)) return 1;
#endif
    FILE* data=std::fopen((dir+"/weights.bin").c_str(),"wbx");
    FILE* index=std::fopen((dir+"/layout.txt").c_str(),"wx");
    if (!data || !index) return 1;
    if (layers!=1 && std::fprintf(index,"C %u %u %u %u\n",layers,sink->executed,recordCount,unsigned(decode))<0) return 1;
    PioneerQ35EmbeddingProposal embedding{};if (!packed.ResolveEmbedding(42,embedding)) return 1;
    std::array<uint8_t,4096> page{};uint64_t offset=0;
    for (unsigned n=0;n<packed.Parts().size();n++) {
        const auto& part=packed.Parts()[n];
        if (part.kind!=PIONEER_Q35_WEIGHT
            || (part.layer>=layers && !(decode && part.layer==Layers) && n!=embedding.part)) continue;
        unsigned first=0,last=part.allocated/4096;
        if (!decode && n==embedding.part) {
            const unsigned start=embedding.address-part.proposedAddress;
            first=start/4096;last=(start+part.rowBytes*16+4095)/4096;
        }
        const unsigned bytes=(last-first)*4096;
        if (std::fprintf(index,"W %u %u %u %llu\n",part.bank,part.proposedAddress+first*4096,bytes,
                static_cast<unsigned long long>(offset))<0) return 1;
        for (unsigned p=first;p<last;p++) {
            if (!packed.ReadWeightPage(source,n,p,page.data(),page.size())
                || std::fwrite(page.data(),1,page.size(),data)!=page.size()) return 1;
            offset+=page.size();
        }
    }
    if (layers>=4) {
        const unsigned allocated=(rope.bytes+4095)&~4095U;
        if (std::fprintf(index,"W %u %u %u %llu\n",rope.bank,rope.address,allocated,
                static_cast<unsigned long long>(offset))<0) return 1;
        for (unsigned at=0;at<allocated;at+=4096) {
            if (!FillRope(rope,at,4096,page.data()) || std::fwrite(page.data(),1,page.size(),data)!=page.size()) return 1;
            offset+=page.size();
        }
    }
    for (unsigned core=0;core<DataflowCoreCount;core++)
        if (std::fprintf(index,"S %u %u %u\n",core,plan->binding[core].bank,plan->binding[core].base)<0) return 1;
    for (unsigned part=0;part<sink->final.count;part++) {
        const auto& f=sink->final.parts[part];
        if (std::fprintf(index,"O %u %u %u %u\n",f.bank,f.address,f.firstRow,f.rows)<0) return 1;
    }
    bool okay=source.Stable() && !fstat(fd,&after) && SameFile(before,after);
    if (std::fclose(data)) okay=false;
    if (std::fclose(index)) okay=false;
    if (std::fclose(file)) okay=false;
    if (!okay) return 1;
    std::printf("prepared authenticated 4B recorded graph with %u layers, %u records%s and %llu staged bytes; NO HARDWARE\n",
        layers,recordCount,decode?" including decode":"",static_cast<unsigned long long>(offset));
}
