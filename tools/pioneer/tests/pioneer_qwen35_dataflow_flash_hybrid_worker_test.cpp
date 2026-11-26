/* SPDX-License-Identifier: MIT */
// Exercise the guarded hybrid producers and reducers with in-memory transport.
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowDenseAttentionHybrid.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace PioneerQwen35;
static void Flat(RowSpace& r,unsigned rows,unsigned bytes,unsigned bank,unsigned address,unsigned unit) {
    r={};r.rows=rows;r.rowBytes=bytes;r.unit=unit;r.count=1;r.parts[0]={bank,address,0,rows};
}
static DataflowDenseAttentionProgram Program(unsigned pos) {
    DataflowDenseAttentionProgram p{};p.layer=3;p.position=pos;p.context=128;p.mode=DataflowDenseAttentionRun;
    const unsigned routes[Banks]={786,978,1170,785,977,1169,1361};
    for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) p.bankRoute[n][b]=routes[b];
    Flat(p.qkv,ConvChannels,sizeof(float),0,0x10000000,16);
    Flat(p.gate,DeltaInner,sizeof(float),0,0x10010000,16);
    Flat(p.contextOutput,Ffn,sizeof(float),0,0x10020000,16);
    Flat(p.kv,2*DenseKeyHeads,p.context*DenseHeadWidth*sizeof(uint16_t),1,0x20000000,1);
    p.queryNorm.type=0;p.queryNorm.inputs=DenseHeadWidth;
    Flat(p.queryNorm.place,1,DenseHeadWidth*sizeof(float),2,0x30000000,1);
    p.keyNorm.type=0;p.keyNorm.inputs=DenseHeadWidth;
    Flat(p.keyNorm.place,1,DenseHeadWidth*sizeof(float),3,0x40000000,1);
    p.rope={0,0x10030000,unsigned(p.context*Rotary*sizeof(float))};
    return p;
}
struct Partial {
    std::array<float,DenseHeadWidth> values{};
    float maximum=0,denominator=0;unsigned first=0,count=0;bool present=false;
};
struct Mock {
    const DataflowDenseAttentionProgram* program=nullptr;
    const std::vector<float>& qg;const std::vector<uint32_t>& kv;
    const std::array<float,DenseHeadWidth>& norm;const std::array<float,Rotary>& rope;
    std::vector<Partial>& partials;std::vector<float>& output;std::vector<unsigned>& coverage;
    unsigned reads=0,published=0;int failRead=-1,failPublish=-1,failPartial=-1;
    bool BindShardProgram(const DataflowDenseAttentionProgram& p) {program=&p;return p.Valid();}
    bool ReadNorm(bool query,float* out) const {
        if (!query || !out) return false;std::memcpy(out,norm.data(),sizeof(norm));return true;
    }
    bool ReadRope(float* out) const {if (!out) return false;std::memcpy(out,rope.data(),sizeof(rope));return true;}
    bool ReadProjectedRows(unsigned first,float* out) const {
        if (!out || first>qg.size() || DenseHeadWidth>qg.size()-first) return false;
        std::memcpy(out,qg.data()+first,DenseHeadWidth*sizeof(float));return true;
    }
    bool ReadKvTile(unsigned head,bool value,unsigned first,unsigned count,float* out) {
        if (!program || !out || head>=DenseKeyHeads || !count || count>DataflowFlashHybridTilePositions
            || first>program->position || count>program->position+1-first || int(reads++)==failRead) return false;
        const size_t index=((head+(value?DenseKeyHeads:0))*program->context+first)*(DenseHeadWidth/2);
        std::memcpy(out,kv.data()+index,count*DataflowDenseAttentionHeadBytes);return true;
    }
    bool PublishPartial(unsigned core,unsigned q,unsigned first,unsigned count,const DataflowDenseAttentionRow& row) {
        if (core>=DataflowCoreCount || q>=DataflowFlashQueriesPerKv || !row.sums || !count
            || int(published++)==failPublish) return false;
        auto& p=partials[core*DataflowFlashQueriesPerKv+q];if (p.present) return false;
        std::memcpy(p.values.data(),row.sums,DenseHeadWidth*sizeof(float));
        p.maximum=row.maximum;p.denominator=row.denominator;p.first=first;p.count=count;p.present=true;return true;
    }
    bool ReadPartial(unsigned core,unsigned q,unsigned first,unsigned count,DataflowDenseAttentionRow& row) const {
        if (core>=DataflowCoreCount || q>=DataflowFlashQueriesPerKv || int(core)==failPartial || !row.sums) return false;
        const auto& p=partials[core*DataflowFlashQueriesPerKv+q];
        if (!p.present || p.first!=first || p.count!=count) return false;
        std::memcpy(row.sums,p.values.data(),DenseHeadWidth*sizeof(float));
        row.maximum=p.maximum;row.denominator=p.denominator;return true;
    }
    bool WriteContextSlice(unsigned first,unsigned count,const float* values) {
        if (!values || first>output.size() || count>output.size()-first) return false;
        for (unsigned i=0;i<count;i++) {if (coverage[first+i]++) return false;output[first+i]=values[i];}
        return true;
    }
};
struct Scratch {
    std::array<float,DataflowFlashHybridTilePositions*DataflowDenseAttentionHeadBytes/sizeof(float)> keys{},values{};
    std::array<float,DataflowFlashQueriesPerKv*DenseHeadWidth> queries{},sums{};
    std::array<float,DenseHeadWidth> norm{},gate{},incoming{};std::array<float,Rotary> rope{};
    DataflowFlashHybridScratch Get() {return {keys.data(),values.data(),queries.data(),sums.data(),
        norm.data(),rope.data(),gate.data(),incoming.data()};}
};
static float Sample(unsigned i,unsigned salt) {return float(int((i*17+salt*23)%193)-96)*0.003f;}
int main() {
    std::vector<float> qg(ConvChannels);for (unsigned i=0;i<qg.size();i++) qg[i]=Sample(i,1);
    std::array<float,DenseHeadWidth> norm{};for (unsigned i=0;i<norm.size();i++) norm[i]=0.8f+float(i%17)*0.002f;
    std::array<float,Rotary> rope{};for (unsigned i=0;i<rope.size();i++) rope[i]=(i&1)?0.0f:1.0f;
    std::vector<uint32_t> kv(2*DenseKeyHeads*128*(DenseHeadWidth/2));
    for (unsigned head=0;head<2*DenseKeyHeads;head++) for (unsigned pos=0;pos<128;pos++)
        for (unsigned pair=0;pair<DenseHeadWidth/2;pair++) {
            const size_t index=(head*128+pos)*(DenseHeadWidth/2)+pair;
            kv[index]=PioneerAttentionToHalf(Sample(pair*2,head+pos+3))
                | (unsigned(PioneerAttentionToHalf(Sample(pair*2+1,head+pos+3)))<<16);
        }
    float maximum=0;
    for (unsigned position:{0U,1U,2U,4U,7U,14U,29U,33U,63U,127U}) {
        auto program=Program(position);assert(program.Valid());
        std::vector<Partial> partials(DataflowCoreCount*DataflowFlashQueriesPerKv);
        std::vector<float> output(DenseHeads*DenseHeadWidth);std::vector<unsigned> coverage(output.size());
        Mock io{nullptr,qg,kv,norm,rope,partials,output,coverage};Scratch scratch{};
        for (unsigned core=0;core<DataflowCoreCount;core++)
            assert(DataflowFlashHybridProduce(program,core,scratch.Get(),io));
        for (unsigned core=0;core<DataflowCoreCount;core++)
            assert(DataflowFlashHybridReduce(program,core,scratch.Get(),io));
        for (unsigned head=0;head<DenseHeads;head++) {
            std::array<float,DenseHeadWidth> query{},expected{};
            std::memcpy(query.data(),qg.data()+2*head*DenseHeadWidth,sizeof(query));
            assert(DataflowDenseNormalize(query.data(),norm.data(),query.data()) && DataflowDenseRotate(query.data(),rope.data()));
            DataflowDenseAttentionRow row{expected.data()};const unsigned kh=head/DataflowFlashQueriesPerKv;
            for (unsigned pos=0;pos<=position;pos++) {
                const size_t key=(kh*128+pos)*(DenseHeadWidth/2),val=((kh+DenseKeyHeads)*128+pos)*(DenseHeadWidth/2);
                assert(row.Add(query.data(),kv.data()+key,kv.data()+val));
            }
            assert(row.Finish(qg.data()+(2*head+1)*DenseHeadWidth));
            for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
                const unsigned index=head*DenseHeadWidth+lane;assert(coverage[index]==1);
                const float error=std::fabs(output[index]-expected[lane]);if (error>maximum) maximum=error;
                assert(std::isfinite(output[index]) && error<2e-5f);
            }
        }
        Mock fail{nullptr,qg,kv,norm,rope,partials,output,coverage};fail.failRead=0;
        assert(!DataflowFlashHybridProduce(program,0,scratch.Get(),fail));
        fail.failRead=-1;fail.failPublish=0;
        assert(!DataflowFlashHybridProduce(program,0,scratch.Get(),fail));
        fail.failRead=-1;fail.failPartial=0;
        assert(!DataflowFlashHybridReduce(program,0,scratch.Get(),fail));
        assert(!DataflowFlashHybridProduce(program,DataflowCoreCount,scratch.Get(),fail));
        assert(!DataflowFlashHybridReduce(program,DataflowCoreCount,scratch.Get(),fail));
        if (position==1) {
            Mock corrupted{nullptr,qg,kv,norm,rope,partials,output,coverage};
            const float denominator=partials[0].denominator;partials[0].denominator=0;
            assert(!DataflowFlashHybridReduce(program,0,scratch.Get(),corrupted));
            partials[0].denominator=denominator;
            const unsigned outside=partials[0].first+partials[0].count;assert(outside<DenseHeadWidth);
            partials[0].values[outside]=1;
            assert(!DataflowFlashHybridReduce(program,0,scratch.Get(),corrupted));
            partials[0].values[outside]=0;
            std::vector<Partial> incomplete(DataflowCoreCount*DataflowFlashQueriesPerKv);
            Mock uncertain{nullptr,qg,kv,norm,rope,incomplete,output,coverage};uncertain.failPublish=1;
            // Core 3's output interval crosses a head boundary. The first
            // publish can succeed before the second fails; report failure,
            // leave its effect intact, and require the caller to retain it.
            assert(!DataflowFlashHybridProduce(program,3,scratch.Get(),uncertain));
            assert(uncertain.published==2 && incomplete[3*4].present && !incomplete[3*4+1].present);
        }
    }
    std::printf("PASS hybrid producer/reducer on 120 mocked workers; max error %.9g; NO HARDWARE\n",maximum);
}
