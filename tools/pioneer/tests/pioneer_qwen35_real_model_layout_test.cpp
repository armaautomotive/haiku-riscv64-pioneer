/* SPDX-License-Identifier: MIT */
// Offline qualification of the resident rewrite's real-model layout,
// embedding input and projection unpack. Never a Work inference fallback.
#include "../PioneerModelWeightCache.h"
#include "../PioneerQwen35PackedPlan.h"
#include "../PioneerQwen35DataflowEmbedding.h"
#include "../TenstorrentGgufQuant.h"
#define PIONEER_Q38_VECTOR_LAYOUT_MACHINE_ONLY
#include "pioneer_q38_vector_layout_test.cpp"
#include "../PioneerQwen35DataflowVectorUnpack.h"
#include "../PioneerQwen35DataflowAffine.h"
#include "../PioneerQwen35Q5Panel.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

using namespace PioneerQwen35;

static bool ReadTiled(PioneerQwen35Gguf& source,const PioneerQwen35PackedPlan& plan,
    unsigned part,uint64_t offset,uint8_t* output,unsigned bytes)
{
    uint8_t page[4096];
    for (unsigned copied=0;copied<bytes;) {
        const uint64_t at=offset+copied;
        if (!plan.ReadWeightPage(source,part,unsigned(at/4096),page,sizeof(page))) return false;
        const unsigned within=unsigned(at%4096);
        const unsigned count=std::min(bytes-copied,unsigned(sizeof(page)-within));
        std::memcpy(output+copied,page+within,count);copied+=count;
    }
    return true;
}

// Compare actual GGUF blocks and the resident SFPU unpack transform. The
// scalar panel code and simulated SFPU are references only; actual matrix
// execution still requires a hardware projection test.
static void CheckProjectionSamples(PioneerQwen35Gguf& source,const PioneerQwen35PackedPlan& plan)
{
    (void)Read;(void)Word;
    unsigned sampled[15]{},panels[15]{},lastTensor[15];
    std::fill(std::begin(lastTensor),std::end(lastTensor),UINT32_MAX);
    for (unsigned partIndex=0;partIndex<plan.Parts().size();partIndex++) {
        const auto& part=plan.Parts()[partIndex];
        if (part.kind!=PIONEER_Q35_WEIGHT || part.tensor>=source.Tensors().size()) continue;
        const auto& tensor=source.Tensors()[part.tensor];
        const unsigned type=tensor.type;
        if ((type!=8 && type!=12 && type!=13 && type!=14) || sampled[type]>=4
            || part.tensor==lastTensor[type]) continue;
        const unsigned blockBytes=PioneerQ35BlockBytes(type),blocks=unsigned(tensor.inputs/PioneerQ35Block(type));
        const unsigned group=(part.rows/16-1)*unsigned(sampled[type]&1);
        const unsigned block=(blocks-1)*unsigned((sampled[type]>>1)&1);
        const unsigned tileBytes=PioneerQ35TileBytes(type);
        std::vector<uint8_t> tile(tileBytes),raw(blockBytes);
        assert(ReadTiled(source,plan,partIndex,uint64_t(group*blocks+block)*tileBytes,tile.data(),tileBytes));
        for (unsigned row=0;row<16;row++) {
            const uint64_t relative=uint64_t(part.firstRow+group*16+row)*blocks*blockBytes
                +uint64_t(block)*blockBytes;
            assert(source.ReadPacked(part.tensor,relative,blockBytes,raw.data(),blockBytes));
            for (unsigned byte=0;byte<blockBytes;byte++) assert(tile[byte*16+row]==raw[byte]);
        }
        for (unsigned panel=0;panel<(type==8?2U:16U);panel++) {
            PioneerQ38Panel expected{},affine{};PioneerQ38VectorPanelSpan span{};
            assert(type==13?PioneerQwen35Q5Panel(tile.data(),tileBytes,panel,&expected)
                :PioneerQ38UnpackPanel(type,tile.data(),tileBytes,panel,&expected));
            assert(PioneerQwen35DataflowAffineOnly(type,tile.data(),tileBytes,panel,&affine));
            assert(type==13?PioneerQwen35DataflowQ5PanelSpan(panel,span)
                :PioneerQ38VectorPanel(type,panel,span));
            auto machine=std::make_unique<VectorLayout>();
            for (unsigned k=0;k<16;k++) for (unsigned row=0;row<16;row++) {
                machine->Raw(k,row,tile[span.low+k*16+row]);
                if (span.second) machine->Raw(16+k,row,tile[span.high+k*16+row]);
            }
            assert(type==13?PioneerQwen35DataflowQ5Transform(*machine,panel)
                :PioneerQ38VectorTransform(*machine,type,panel));
            for (unsigned i=0;i<256;i++) assert(machine->moves[i]==1
                && machine->moved[i]==uint16_t(expected.operand[i/2]>>((i%2)*16)));
            for (unsigned row=0;row<16;row++) assert(affine.coefficient[row]==expected.coefficient[row]
                && affine.minimum[row]==expected.minimum[row]);
            panels[type]++;
        }
        sampled[type]++;lastTensor[type]=part.tensor;
    }
    for (unsigned type:{8U,12U,13U,14U}) assert(sampled[type]==4);
    std::printf("PASS authenticated 4B projection tiles: Q8=%u Q4_K=%u Q5_K=%u Q6_K=%u SFPU panels; SIMULATED ONLY\n",
        panels[8],panels[12],panels[13],panels[14]);
}

int main(int argc,char** argv)
{
    if (argc!=2) {
        std::fprintf(stderr,"usage: %s Qwen3.5-4B-Q4_K_M.gguf\n",argv[0]);return 2;
    }
    std::unique_ptr<FILE,decltype(&std::fclose)> file(std::fopen(argv[1],"rb"),&std::fclose);
    if (!file) { std::perror("open GGUF");return 1; }
    PioneerQwen35Gguf source;
    PioneerModelWeightCache::Digest digest{};std::string error;
    if (!source.Open(file.get())
        || !PioneerModelWeightCache::HashFile(fileno(file.get()),source.FullBytes,digest,error)
        || PioneerModelWeightCache::Hex(digest)!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
        || !source.Stable()) {
        std::fprintf(stderr,"Pinned model authentication failed: %s\n",error.c_str());return 1;
    }
    PioneerQwen35PackedPlan plan;
    assert(plan.Build(source,128,true));
    const auto* embedding=source.TiedHead();
    assert(embedding && embedding->type==14 && embedding->inputs==2560);
    const unsigned tensor=unsigned(embedding-source.Tensors().data());
    std::vector<unsigned> tokens={0,1,15,16,17,255,unsigned(embedding->outputs-1)};
    for (const auto& part:plan.Parts()) {
        if (part.kind!=PIONEER_Q35_WEIGHT || part.tensor!=tensor) continue;
        tokens.push_back(part.firstRow);
        tokens.push_back(part.firstRow+part.rows-1);
        if (part.firstRow) tokens.push_back(part.firstRow-1);
    }
    std::sort(tokens.begin(),tokens.end());tokens.erase(std::unique(tokens.begin(),tokens.end()),tokens.end());
    uint8_t tile[DataflowQ6KEmbeddingTileBytes],raw[210];
    unsigned panels=0;
    for (unsigned token:tokens) {
        PioneerQ35EmbeddingProposal location{};assert(plan.ResolveEmbedding(token,location));
        const auto& part=plan.Parts().at(location.part);
        assert(part.tensor==tensor && location.lane==(token-part.firstRow)%16);
        for (unsigned block=0;block<embedding->inputs/256;block++) {
            const uint64_t offset=uint64_t(location.address-part.proposedAddress)
                +uint64_t(block)*DataflowQ6KEmbeddingTileBytes;
            assert(ReadTiled(source,plan,location.part,offset,tile,sizeof(tile)));
            const uint64_t relative=uint64_t(token)*(embedding->inputs/256)*sizeof(raw)
                +uint64_t(block)*sizeof(raw);
            assert(source.ReadPacked(tensor,relative,sizeof(raw),raw,sizeof(raw)));
            float expected[256];assert(PioneerDecodeQ6K(raw,expected));
            for (unsigned panel=0;panel<DataflowQ6KEmbeddingPanels;panel++) {
                float actual[16];
                assert(DataflowDecodeQ6KEmbeddingPanel(tile,sizeof(tile),location.lane,panel,actual,16));
                for (unsigned i=0;i<16;i++) {
                    if (PioneerAttentionBits(actual[i])!=PioneerAttentionBits(expected[panel*16+i])) {
                        std::fprintf(stderr,"embedding mismatch token=%u block=%u panel=%u element=%u\n",
                            token,block,panel,i);return 1;
                    }
                }
                ++panels;
            }
        }
    }
    assert(source.Stable());
    std::printf("PASS authenticated 4B GGUF embedding: %zu boundary tokens, %u panels, exact scalar reference; OFF-CARD ONLY\n",
        tokens.size(),panels);
    CheckProjectionSamples(source,plan);
    assert(source.Stable());
    return 0;
}
