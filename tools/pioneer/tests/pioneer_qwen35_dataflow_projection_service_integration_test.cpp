/* SPDX-License-Identifier: MIT */
// Connect the production service owner and projection scheduler to the real
// reader/compute/writer queue functions over a simulated device. The matrix
// model and fake service I/O are test-only; no hardware or Work fallback.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define main qwen35_projection_fixture_test_main
#include "pioneer_qwen35_dataflow_projection_qualification_test.cpp"
#undef main
#define main qwen35_service_owner_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main

struct ProjectionServiceIO : MockDataflowIO {
    SimMemory memory;
    unsigned projected=0;

    bool WaitForCompletion(unsigned core,unsigned sequence,DataflowWorkerSnapshot& result) {
        // Production must publish every shard before waiting for any receipt.
        for (unsigned i=0;i<DataflowCoreCount;i++)
            if (mailbox[i].commandSequence!=sequence) return false;
        if (core!=projected || !invocation[core].Valid()
            || invocation[core].assignment.weight[0].firstGroup!=core) return false;
        DataflowQueueControl controls[DataflowCircularBufferCount]{};
        DataflowQueueView queues[DataflowCircularBufferCount]{};
        std::vector<uint32_t> local((DataflowLocalBufferEnd-DataflowWeightBufferAddress)/4);
        std::vector<uint32_t> cache(DataflowActivationCacheBytes/4);
        if (!DataflowBindQueues(invocation[core],controls,
            reinterpret_cast<uint8_t*>(local.data()),queues)) return false;
        Reader reader{memory};MatrixSimulation compute{memory};Writer writer{memory};
        bool read=false,calculate=false,write=false;
        std::thread a([&] { read=PioneerQwen35SharedActivationReader(invocation[core],queues[0],queues[1],
            reinterpret_cast<uint8_t*>(cache.data()),reader); });
        std::thread b([&] { calculate=PioneerQwen35SharedActivationCompute(invocation[core],queues[0],queues[1],
            queues[2],reinterpret_cast<uint8_t*>(cache.data()),compute); });
        std::thread c([&] { write=PioneerQwen35SharedActivationWriter(invocation[core],queues[2],writer); });
        a.join();b.join();c.join();
        if (!read || !calculate || !write) return false;
        for (auto& control:controls)
            if (DataflowQueueFailure(control)
                || DataflowLoadAcquire(control.produced)!=DataflowLoadAcquire(control.consumed)) return false;
        projected++;
        return MockDataflowIO::WaitForCompletion(core,sequence,result);
    }
};

int main()
{
    auto io=std::make_unique<ProjectionServiceIO>();
    auto service=std::make_unique<DataflowServiceOwner>();
    auto program=std::make_unique<DataflowProjectionProgram>();
    auto schedule=std::make_unique<PioneerQ35ProjectionProgram>();
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    assert(Fixture::Build(io->memory.bank,io->memory.base,Fixture::ScratchBytes,TestRoutes,*program,*schedule));
    assert(Fixture::FillWeightBytes(0,io->memory.weights.data(),Fixture::WeightBytes));
    assert(Fixture::FillInputBytes(0,io->memory.input.data(),Fixture::InputBytes));
    assert(Fixture::FillOutputBytes(0,io->memory.output.data(),Fixture::OutputBytes));
    assert(service->Install(image,*io)==DataflowServiceComplete && service->Ready());
    assert(service->RunProjection(*program,*io)==DataflowServiceComplete);
    assert(service->NextSequence()==1 && io->projected==DataflowCoreCount);
    assert(io->memory.reads==DataflowCoreCount*Fixture::Blocks
        && io->memory.matmuls==DataflowCoreCount*Fixture::Blocks
        && io->memory.writes==DataflowCoreCount);
    for (unsigned row=0;row<Fixture::Rows;row++) {
        uint32_t bits=0;std::memcpy(&bits,io->memory.output.data()+row*4,4);
        assert(Fixture::OutputMatches(row,bits));
    }
    // A failed receipt must not commit a result. Exercise the resident
    // owner's existing recovery path and a later successful sequence.
    io->projected=0;io->failWorkerCore=37;
    assert(Fixture::FillOutputBytes(0,io->memory.output.data(),Fixture::OutputBytes));
    assert(service->RunProjection(*program,*io)==DataflowServiceProjectionFailed
        && service->Faulted() && io->completeCount==1);
    assert(service->RecoverProjection(*io)==DataflowServiceRecovered && service->Ready()
        && service->NextSequence()==2);
    io->projected=0;io->failWorkerCore=UINT32_MAX;
    assert(Fixture::FillOutputBytes(0,io->memory.output.data(),Fixture::OutputBytes));
    assert(service->RunProjection(*program,*io)==DataflowServiceComplete
        && service->NextSequence()==3 && io->completeCount==2);
    for (unsigned row=0;row<Fixture::Rows;row++) {
        uint32_t bits=0;std::memcpy(&bits,io->memory.output.data()+row*4,4);
        assert(Fixture::OutputMatches(row,bits));
    }
    assert(service->Close(*io)==DataflowServiceClosed && service->Closed());
    assert(io->roleStops==1 && io->roleRestores==1 && io->roleVerifies==1
        && io->restoreCount==DataflowCoreCount);
    std::puts("PASS resident service dispatch, simulated Q4_K results and fault recovery, then clean close; NO HARDWARE");
    return 0;
}
