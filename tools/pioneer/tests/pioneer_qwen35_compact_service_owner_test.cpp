/* SPDX-License-Identifier: MIT */
// Exercise the real service owner with a compact-sized dispatch image and fake IO.
#define main q35_existing_service_owner_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#include <vector>

#ifndef PIONEER_QWEN35_DEVICE_DISPATCH
#error Compact simulator ownership test requires the actual dispatch path
#endif

struct CompactIO : MockDataflowIO {
    static constexpr unsigned Words=37608/sizeof(uint32_t);
    std::vector<uint32_t> dispatch=std::vector<uint32_t>(DataflowCoreCount*Words);
    unsigned failCore=UINT32_MAX,corruptCore=UINT32_MAX;
    bool WriteCoreMemory(unsigned core,unsigned address,const void* input,unsigned bytes) {
        if (address>=DataflowServiceDispatchAddress && address<DataflowServiceDispatchLimit) {
            if (core>=DataflowCoreCount || !held[core] || !input || !bytes || (address&3) || (bytes&3))
                return false;
            const unsigned offset=(address-DataflowServiceDispatchAddress)/4;
            if (offset>Words || bytes/4>Words-offset || (core==failCore && offset>=Words-64)) return false;
            memcpy(dispatch.data()+core*Words+offset,input,bytes);return true;
        }
        return MockDataflowIO::WriteCoreMemory(core,address,input,bytes);
    }
    bool ReadCoreMemory(unsigned core,unsigned address,void* output,unsigned bytes) {
        if (address>=DataflowServiceDispatchAddress && address<DataflowServiceDispatchLimit) {
            if (core>=DataflowCoreCount || !held[core] || !output || !bytes || (address&3) || (bytes&3))
                return false;
            const unsigned offset=(address-DataflowServiceDispatchAddress)/4;
            if (offset>Words || bytes/4>Words-offset) return false;
            memcpy(output,dispatch.data()+core*Words+offset,bytes);
            if (core==corruptCore && offset>=Words-64) static_cast<uint32_t*>(output)[0]^=1;
            return true;
        }
        return MockDataflowIO::ReadCoreMemory(core,address,output,bytes);
    }
};

int main() {
    static_assert(CompactIO::Words*4>0x7000 && CompactIO::Words*4<=0x9c00,
        "test crosses the old limit without exceeding the declared dispatch range");
    uint32_t text[96]{};
    std::vector<uint32_t> dispatch(CompactIO::Words);
    for (unsigned i=0;i<dispatch.size();i++) dispatch[i]=0x9a000000U+i;
    DataflowServiceImageView image={DataflowServiceBootWord,text,96,dispatch.data(),unsigned(dispatch.size())};
    assert(image.Valid());
    assert(DataflowServicePcInImage(DataflowServiceDispatchAddress+(CompactIO::Words-1)*4,96,CompactIO::Words));
    assert(!DataflowServicePcInImage(DataflowServiceDispatchAddress+CompactIO::Words*4,96,CompactIO::Words));
    {
        CompactIO io;DataflowServiceOwner owner;
        assert(owner.Install(image,io)==DataflowServiceComplete && owner.Ready());
        for (unsigned core=0;core<DataflowCoreCount;core++)
            assert(!memcmp(io.dispatch.data()+core*CompactIO::Words,dispatch.data(),dispatch.size()*4));
        assert(owner.Close(io)==DataflowServiceClosed && owner.Closed());
        assert(io.restoreCount==DataflowCoreCount);
    }
    for (unsigned fault=0;fault<2;fault++) {
        CompactIO io;DataflowServiceOwner owner;
        if (fault==0) io.failCore=3;else io.corruptCore=3;
        assert(owner.Install(image,io)==DataflowServiceImageWriteFailed);
        assert(owner.Faulted() && io.startCount==0 && io.restoreCount==0);
    }
    std::puts("PASS compact-size dispatch staging, readback, fault retention and 120-worker close; FAKE IO ONLY");
}
