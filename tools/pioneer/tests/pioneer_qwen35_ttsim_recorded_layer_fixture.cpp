/* SPDX-License-Identifier: MIT */
// Authenticated 4B first layer through the actual full-graph recorder.
// The next record is deliberately zeroed. Offline simulator fixture only.
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
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
using namespace PioneerQwen35;
static constexpr unsigned kExecuted=17,kRecords=kExecuted+1;
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
    std::vector<DataflowStreamRecord> records=std::vector<DataflowStreamRecord>(DataflowCoreCount*kRecords);
    RowSpace final{};unsigned count=0;bool sealed=false;
    bool Begin(const DataflowStreamPlan&) {return true;}
    bool StageRecord(const DataflowStreamBinding& binding,const Range&,unsigned step,const DataflowStreamRecord& record) {
        if (binding.core!=count%DataflowCoreCount || step!=count/DataflowCoreCount
            || !DataflowStreamRecordStructureValid(record,binding.epoch,step+1,binding.core)) return false;
        ++count;
        if (step<kExecuted) records[binding.core*kRecords+step]=record;
        if (step==kExecuted-1 && binding.core==0) {
            if (record.header[5]!=DataflowResidualMagic) return false;
            DataflowResidualInvocation invocation{};std::memcpy(&invocation,record.payload,sizeof(invocation));
            if (!invocation.Valid() || invocation.mode!=DataflowResidualAdd) return false;
            final=invocation.activation;
        }
        return true;
    }
    bool Seal(const DataflowStreamPlan& plan) {sealed=count==plan.count*DataflowCoreCount;return sealed;}
    void Abort() {sealed=false;}
};
int main(int argc,char** argv) {
    if (argc!=3) {std::fprintf(stderr,"usage: %s pinned.gguf empty-output-directory\n",argv[0]);return 2;}
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
    auto plan=std::make_unique<DataflowStreamPlan>();auto sink=std::make_unique<Sink>();
    auto scratch=std::make_unique<DataflowStreamRecord>();auto projection=std::make_unique<DataflowProjectionProgram>();unsigned stage=0;
    if (!live->BindValidatedModel(*model,*workspace,routes,rope)
        || !DataflowStreamPlanBuilder::Build(packed,*model,*workspace,rope,routes,99,DataflowGraphPromptSteps,*plan)
        || !DataflowStreamRecording::Build(packed,*live,*candidate,*plan,routes,0,42,0,false,*sink,*scratch,*projection,stage)
        || !sink->sealed || stage!=DataflowDecoderSequenceComplete || !sink->final.Valid()) {std::fclose(file);return 1;}
    DataflowDynamicTokenContext token{};if (!DataflowDynamicTokenContext::Build(*model,42,0,token)) return 1;
    std::vector<DataflowGraphRunDescriptor> descriptors(DataflowCoreCount);
    std::array<DataflowBarrierPacket,2> command{};
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        if (!DataflowGraphRunBuild(plan->binding[core],plan->lease[core],routes,token,
                {5,1,DataflowGraphPromptSteps},1,1000000,0,descriptors[core])) return 1;
        if (!core) {DataflowResidentCommand c{};if (!DataflowResidentCommandBuild(descriptors[core],c)) return 1;
            command={DataflowResidentCommandPayload(c),DataflowResidentCommandCommit(c)};}
    }
    const std::string dir=argv[2];
    if (!WriteNew(dir+"/templates.bin",descriptors.data(),descriptors.size()*sizeof(descriptors[0]))
        || !WriteNew(dir+"/records.bin",sink->records.data(),sink->records.size()*sizeof(sink->records[0]))
        || !WriteNew(dir+"/command.bin",command.data(),sizeof(command))) return 1;
    FILE* data=std::fopen((dir+"/weights.bin").c_str(),"wbx");
    FILE* index=std::fopen((dir+"/layout.txt").c_str(),"wx");
    if (!data || !index) return 1;
    PioneerQ35EmbeddingProposal embedding{};if (!packed.ResolveEmbedding(42,embedding)) return 1;
    std::array<uint8_t,4096> page{};uint64_t offset=0;
    for (unsigned n=0;n<packed.Parts().size();n++) {
        const auto& part=packed.Parts()[n];
        if (part.kind!=PIONEER_Q35_WEIGHT || (part.layer!=0 && n!=embedding.part)) continue;
        unsigned first=0,last=part.allocated/4096;
        if (n==embedding.part) {
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
    std::printf("prepared authenticated 4B full recorded graph with %u first-layer steps and %llu staged weight bytes; NO HARDWARE\n",
        kExecuted,static_cast<unsigned long long>(offset));
}
