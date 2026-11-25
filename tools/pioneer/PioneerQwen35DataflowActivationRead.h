/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ACTIVATION_READ_H
#define PIONEER_QWEN35_DATAFLOW_ACTIVATION_READ_H
#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {
enum : unsigned { DataflowActivationReadMaxBytes=16384 };

// Walk only the caller's contiguous row slice, splitting at both bank and
// NoC transfer boundaries. The first walk validates the whole plan before
// any request can be issued; the second submits the same immutable plan.
template<class Emit>
bool DataflowActivationReadChunks(const RowSpace& source,unsigned base,unsigned rows,
    uint32_t destination,const unsigned (&routes)[Banks],Emit emit)
{
    if (!source.Valid() || source.unit!=16 || source.rowBytes!=sizeof(float)
        || !rows || ((base|rows)&15) || base>source.rows || rows>source.rows-base
        || destination!=DataflowActivationCacheAddress
        || uint64_t(rows)*sizeof(float)>DataflowActivationCacheBytes) return false;
    const unsigned end=base+rows;unsigned covered=0;
    for (unsigned i=0;i<source.count && covered<rows;i++) {
        const auto& part=source.parts[i];const unsigned partEnd=part.firstRow+part.rows;
        if (partEnd<=base || part.firstRow>=end) continue;
        const unsigned first=part.firstRow>base?part.firstRow:base;
        const unsigned last=partEnd<end?partEnd:end;
        if (first!=base+covered || last<=first || !routes[part.bank] || routes[part.bank]>0xfff)
            return false;
        const unsigned partRows=last-first,localStart=first-part.firstRow;
        for (unsigned offset=0;offset<partRows;) {
            unsigned chunkRows=DataflowActivationReadMaxBytes/sizeof(float);
            if (chunkRows>partRows-offset) chunkRows=partRows-offset;
            const unsigned bytes=chunkRows*sizeof(float);
            const uint64_t remote=uint64_t(part.address)+uint64_t(localStart+offset)*sizeof(float);
            const uint64_t local=uint64_t(destination)+uint64_t(covered+offset)*sizeof(float);
            if (!chunkRows || (chunkRows&15) || remote>=BankLimit || bytes>BankLimit-remote
                || local>UINT32_MAX || local+bytes>uint64_t(DataflowActivationCacheAddress)+DataflowActivationCacheBytes
                || !emit(part.bank,uint32_t(remote),uint32_t(local),bytes)) return false;
            offset+=chunkRows;
        }
        covered+=partRows;
    }
    return covered==rows;
}

// The pipeline owns both request IDs for the entire read. On any failed or
// uncertain operation, the caller faults the role and retains its buffers.
template<class Pipeline>
bool DataflowActivationReadPipelined(const RowSpace& source,unsigned base,unsigned rows,
    uint32_t destination,const unsigned (&routes)[Banks],Pipeline& pipeline)
{
    auto validate=[](unsigned,uint32_t,uint32_t,unsigned) { return true; };
    if (!DataflowActivationReadChunks(source,base,rows,destination,routes,validate)
        || !pipeline.Start()) return false;
    uint32_t issued=0,retired=0;
    auto issue=[&](unsigned bank,uint32_t remote,uint32_t local,unsigned bytes) {
        if (issued-retired==2) {
            if (!pipeline.Complete(retired)) return false;
            ++retired;
        }
        if (!pipeline.Issue(issued,remote,routes[bank],local,bytes)) return false;
        ++issued;return true;
    };
    if (!DataflowActivationReadChunks(source,base,rows,destination,routes,issue)) return false;
    while (retired<issued) {
        if (!pipeline.Complete(retired)) return false;
        ++retired;
    }
    return pipeline.Finish();
}
}
#endif
