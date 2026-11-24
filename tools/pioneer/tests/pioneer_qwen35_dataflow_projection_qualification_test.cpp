/* SPDX-License-Identifier: MIT */
// Offline fixture and concurrent role simulation. This is not Work-mode
// inference, hardware execution, or a substitute for Tensix qualification.
#include "../PioneerQwen35DataflowProjectionQualification.h"
#include "../PioneerQwen35DataflowSharedActivationRoles.h"
#include "../PioneerQ38ProjectionInput.h"
#include "../PioneerQwen38Panel.h"
#include "../TenstorrentQ4K.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

using namespace PioneerQwen35;
using Fixture=DataflowProjectionQualification;

struct SimMemory {
    unsigned bank=0,base=BankBase;
    std::vector<uint8_t> weights=std::vector<uint8_t>(Fixture::WeightBytes);
    std::vector<uint8_t> input=std::vector<uint8_t>(Fixture::InputBytes);
    std::vector<uint8_t> output=std::vector<uint8_t>(Fixture::OutputBytes);
    unsigned reads=0,matmuls=0,writes=0,peak=0;
    uint32_t InputAddress() const { return base+Fixture::WeightPages*Fixture::PageBytes; }
    uint32_t OutputAddress() const { return base+(Fixture::WeightPages+Fixture::InputPages)*Fixture::PageBytes; }
};

struct Reader {
    SimMemory& memory;
    bool Bind(const DataflowCoreInvocation& p) { return p.Valid() && p.type==12; }
    bool ReadActivation(const RowSpace& source,unsigned rows,uint8_t* out,unsigned bytes) {
        if (!out || rows!=Fixture::Inputs || bytes!=Fixture::InputBytes || source.count!=1
            || source.parts[0].bank!=memory.bank || source.parts[0].address!=memory.InputAddress()) return false;
        std::memcpy(out,memory.input.data(),bytes);return true;
    }
    bool PackActivation(unsigned type,unsigned inputs,unsigned chunk,const float* source,uint8_t* output,unsigned bytes) {
        return type==12 && inputs==Fixture::Inputs && chunk<Fixture::Blocks
            && bytes==sizeof(PioneerQ38ProjectionInput)
            && PioneerQ38ProjectionQuantize(type,source+chunk*256,256,
                reinterpret_cast<PioneerQ38ProjectionInput*>(output));
    }
    struct WeightPipeline {
        struct Request { uint32_t address=0;uint8_t* output=nullptr;unsigned bytes=0;bool active=false; };
        Reader& reader;Request requests[2]{};uint32_t issued=0,retired=0;
        explicit WeightPipeline(Reader& r):reader(r) {}
        bool Start() { return true; }
        bool Issue(uint32_t sequence,unsigned bank,uint32_t address,uint8_t* out,unsigned bytes) {
            if (bank!=reader.memory.bank || sequence!=issued || issued-retired>=2 || !out
                || requests[sequence%2].active || address<reader.memory.base
                || bytes!=Fixture::TileBytes || address-reader.memory.base>Fixture::WeightBytes-bytes) return false;
            requests[sequence%2]={address,out,bytes,true};issued++;
            const unsigned pending=issued-retired;
            if (pending>reader.memory.peak) reader.memory.peak=pending;
            return true;
        }
        bool Complete(uint32_t sequence) {
            if (sequence!=retired || !requests[sequence%2].active) return false;
            auto& r=requests[sequence%2];
            std::memcpy(r.output,reader.memory.weights.data()+r.address-reader.memory.base,r.bytes);
            r.active=false;retired++;reader.memory.reads++;return true;
        }
        bool Finish() { return issued==retired; }
        uint32_t Issued() const { return issued; }
        uint32_t Retired() const { return retired; }
    };
};

// Independent host test model for the Tensix operation. This exists only in
// this test binary; the driver compute path has no CPU fallback.
struct MatrixSimulation {
    enum { TensixMatrixEngine=1 };
    SimMemory& memory;float sum[16]{};bool omitMinimum=false;
    bool Bind(const DataflowCoreInvocation& p) { return p.Valid() && p.type==12; }
    bool TensixBeginOutput(unsigned type) {
        if (type!=12) return false;
        for (float& x:sum) x=0;return true;
    }
    static int Operand(uint16_t bits) {
        assert(PioneerInt8OperandValid(bits));
        return (bits&0x8000)?-int(bits&1023):int(bits&1023);
    }
    bool TensixMatmul(unsigned type,unsigned,const uint8_t* weights,const uint8_t* input) {
        if (type!=12 || !weights || !input) return false;
        const auto& packed=*reinterpret_cast<const PioneerQ38ProjectionInput*>(input);
        const float inputScale=PioneerAttentionFloat(packed.scale[0]);
        for (unsigned panel=0;panel<16;panel++) {
            PioneerQ38Panel decoded{};
            if (!PioneerQ38UnpackPanel(type,weights,Fixture::TileBytes,panel,&decoded)) return false;
            for (unsigned row=0;row<16;row++) {
                int dot=0,activationSum=0;
                for (unsigned lane=0;lane<16;lane++) {
                    const unsigned at=lane*16+row;
                    const uint16_t weight=uint16_t(decoded.operand[at/2]>>((at%2)*16));
                    const unsigned ai=panel*16+lane;
                    const uint16_t activation=uint16_t(packed.operand[ai/2]>>((ai%2)*16));
                    const int activationValue=Operand(activation);
                    dot+=Operand(weight)*activationValue;activationSum+=activationValue;
                }
                const float coefficient=PioneerAttentionFloat(decoded.coefficient[row]);
                const float minimum=PioneerAttentionFloat(decoded.minimum[row]);
                if (minimum!=-float(Fixture::Minimum(panel/2))) return false;
                sum[row]+=float(dot)*coefficient*inputScale
                    +(omitMinimum?0.0f:float(activationSum)*minimum*inputScale);
            }
        }
        memory.matmuls++;return true;
    }
    bool TensixFinishOutput(uint8_t* out,unsigned bytes) {
        if (!out || bytes!=sizeof(sum)) return false;
        std::memcpy(out,sum,bytes);return true;
    }
};

struct Writer {
    SimMemory& memory;
    bool Bind(const DataflowCoreInvocation& p) { return p.Valid() && p.type==12; }
    bool WriteOutput(unsigned bank,uint32_t address,const uint8_t* in,unsigned bytes) {
        if (bank!=memory.bank || !in || address<memory.OutputAddress() || bytes!=DataflowOutputPageBytes
            || address-memory.OutputAddress()>Fixture::OutputBytes-bytes) return false;
        std::memcpy(memory.output.data()+address-memory.OutputAddress(),in,bytes);memory.writes++;return true;
    }
    bool WriteOutputPair(unsigned,uint32_t,const uint8_t*) { return false; } // exactly one group per worker
};

int main()
{
    unsigned routes[2][Banks]{};
    for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++) routes[noc][bank]=bank+1;
    auto program=std::make_unique<DataflowProjectionProgram>();
    auto schedule=std::make_unique<PioneerQ35ProjectionProgram>();
    assert(Fixture::Build(0,BankBase,Fixture::ScratchBytes,routes,*program,*schedule));
    assert(program->activeCores==120 && program->groups==120 && program->blocks==10);
    auto invalid=std::make_unique<DataflowProjectionProgram>();
    auto rejected=std::make_unique<PioneerQ35ProjectionProgram>();
    assert(!Fixture::Build(0,BankBase+1,Fixture::ScratchBytes,routes,*invalid,*rejected));
    assert(!Fixture::Build(0,BankBase,Fixture::ScratchBytes-1,routes,*invalid,*rejected));
    assert(!Fixture::Build(Banks,BankBase,Fixture::ScratchBytes,routes,*invalid,*rejected));
    assert(!Fixture::Build(0,BankLimit-4096,Fixture::ScratchBytes,routes,*invalid,*rejected));
    routes[1][1]=routes[1][0];
    assert(!Fixture::Build(0,BankBase,Fixture::ScratchBytes,routes,*invalid,*rejected));
    routes[1][1]=2;
    SimMemory memory;
    // Exercise unaligned chunk boundaries and reject out-of-range requests.
    for (unsigned at=0;at<Fixture::WeightBytes;) {
        const unsigned bytes=std::min(4093U,Fixture::WeightBytes-at);
        assert(Fixture::FillWeightBytes(at,memory.weights.data()+at,bytes));at+=bytes;
    }
    assert(Fixture::FillInputBytes(0,memory.input.data(),Fixture::InputBytes));
    assert(Fixture::FillOutputBytes(0,memory.output.data(),Fixture::OutputBytes));
    assert(!Fixture::FillWeightBytes(Fixture::WeightBytes,memory.weights.data(),1)
        && !Fixture::FillInputBytes(Fixture::InputBytes,memory.input.data(),1)
        && !Fixture::FillOutputBytes(Fixture::OutputBytes,memory.output.data(),1));

    // Independently decode the canonical GGUF block bytes and verify the
    // fixture's integer oracle for every worker, row and block.
    for (unsigned group=0;group<120;group++) for (unsigned lane=0;lane<16;lane++) {
        const unsigned row=group*16+lane;unsigned sum=0;
        for (unsigned block=0;block<Fixture::Blocks;block++) {
            uint8_t raw[144]{};float decoded[256]{};
            const unsigned tile=(group*Fixture::Blocks+block)*Fixture::TileBytes;
            for (unsigned byte=0;byte<144;byte++) raw[byte]=memory.weights[tile+byte*16+lane];
            assert(PioneerDecodeQ4K(raw,decoded));
            for (float value:decoded) assert(value==float(Fixture::Quant(row,block)));
            sum+=Fixture::Quant(row,block)*(block<6 ? (1U<<(block*2)) : 1U);
        }
        assert(Fixture::ExpectedInteger(row)==sum*32U*127U);
        if (row) assert(Fixture::ExpectedInteger(row)>Fixture::ExpectedInteger(row-1));
        const float expected=float(Fixture::ExpectedInteger(row));uint32_t bits=0;
        std::memcpy(&bits,&expected,sizeof(bits));
        assert(Fixture::OutputMatches(row,bits) && !Fixture::OutputMatches(row,bits^1));
    }
    assert(!Fixture::OutputMatches(Fixture::Rows,0));

    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowCoreInvocation invocation{};
        assert(DataflowProgramBuilder::BindCore(*program,core,invocation)
            && invocation.assignment.active && invocation.assignment.groups==1
            && invocation.assignment.weight[0].firstGroup==core
            && invocation.assignment.output[0].firstRow==core*16);
        DataflowQueueControl controls[DataflowCircularBufferCount]{};
        std::vector<uint32_t> local((DataflowLocalBufferEnd-DataflowWeightBufferAddress)/4);
        std::vector<uint32_t> cache(DataflowActivationCacheBytes/4);
        DataflowQueueView queues[DataflowCircularBufferCount]{};
        assert(DataflowBindQueues(invocation,controls,reinterpret_cast<uint8_t*>(local.data()),queues));
        Reader reader{memory};MatrixSimulation compute{memory};Writer writer{memory};
        bool readOkay=false,computeOkay=false,writeOkay=false;
        std::thread a([&] { readOkay=PioneerQwen35SharedActivationReader(invocation,queues[0],queues[1],
            reinterpret_cast<uint8_t*>(cache.data()),reader); });
        std::thread b([&] { computeOkay=PioneerQwen35SharedActivationCompute(invocation,queues[0],queues[1],
            queues[2],reinterpret_cast<uint8_t*>(cache.data()),compute); });
        std::thread c([&] { writeOkay=PioneerQwen35SharedActivationWriter(invocation,queues[2],writer); });
        a.join();b.join();c.join();
        assert(readOkay && computeOkay && writeOkay);
        for (auto& control:controls) assert(!DataflowQueueFailure(control)
            && DataflowLoadAcquire(control.produced)==DataflowLoadAcquire(control.consumed));
    }
    assert(memory.reads==120*Fixture::Blocks && memory.matmuls==120*Fixture::Blocks
        && memory.writes==120 && memory.peak==2);
    for (unsigned row=0;row<Fixture::Rows;row++) {
        uint32_t bits=0;std::memcpy(&bits,memory.output.data()+row*4,4);
        assert(Fixture::OutputMatches(row,bits));
    }
    // Dropping the Q4_K minimum correction must be caught by the same
    // fixed fixture, even though every queue and worker otherwise succeeds.
    {
        DataflowCoreInvocation invocation{};assert(DataflowProgramBuilder::BindCore(*program,0,invocation));
        DataflowQueueControl controls[DataflowCircularBufferCount]{};
        std::vector<uint32_t> local((DataflowLocalBufferEnd-DataflowWeightBufferAddress)/4);
        std::vector<uint32_t> cache(DataflowActivationCacheBytes/4);
        DataflowQueueView queues[DataflowCircularBufferCount]{};
        assert(DataflowBindQueues(invocation,controls,reinterpret_cast<uint8_t*>(local.data()),queues));
        Reader reader{memory};MatrixSimulation compute{memory};compute.omitMinimum=true;Writer writer{memory};
        bool readOkay=false,computeOkay=false,writeOkay=false;
        std::thread a([&] { readOkay=PioneerQwen35SharedActivationReader(invocation,queues[0],queues[1],
            reinterpret_cast<uint8_t*>(cache.data()),reader); });
        std::thread b([&] { computeOkay=PioneerQwen35SharedActivationCompute(invocation,queues[0],queues[1],
            queues[2],reinterpret_cast<uint8_t*>(cache.data()),compute); });
        std::thread c([&] { writeOkay=PioneerQwen35SharedActivationWriter(invocation,queues[2],writer); });
        a.join();b.join();c.join();assert(readOkay && computeOkay && writeOkay);
        for (unsigned row=0;row<16;row++) {
            uint32_t bits=0;std::memcpy(&bits,memory.output.data()+row*4,4);
            assert(!Fixture::OutputMatches(row,bits));
        }
    }
    std::puts("PASS fixed Q4_K projection partitions and exact results for 120 simulated workers; NO HARDWARE");
    return 0;
}
