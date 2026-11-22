/* SPDX-License-Identifier: MIT */
// Offline-only concrete role bodies for a future multi-RISC service. These
// functions do not install images, select reset state, or launch processors.
// An owner must still supply safe boot, dispatch, cache, stop and restoration.
#define PIONEER_QWEN35_QUEUED_COMPUTE 1
#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowTensixOps.h"

extern "C" bool qwen35_queued_reader_once()
{
    PioneerQwen35::DataflowBlackholeReaderOps reader;
    return PioneerQwen35::PioneerQwen35DataflowReaderEntry(reader);
}

extern "C" bool qwen35_queued_compute_once()
{
    PioneerQwen35::DataflowBlackholeTensixOps compute;
    return PioneerQwen35::PioneerQwen35DataflowComputeEntry(compute);
}

extern "C" bool qwen35_queued_writer_once()
{
    PioneerQwen35::DataflowBlackholeWriterOps writer;
    return PioneerQwen35::PioneerQwen35DataflowWriterEntry(writer);
}
