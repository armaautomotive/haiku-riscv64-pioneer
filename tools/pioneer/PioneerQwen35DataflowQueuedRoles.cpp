/* SPDX-License-Identifier: MIT */
// Offline-only concrete role bodies for a future multi-RISC service. These
// functions do not install images, select reset state, or launch processors.
// An owner must still supply safe boot, dispatch, cache, stop and restoration.
#define PIONEER_QWEN35_QUEUED_COMPUTE 1
#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowTensixOps.h"
#include "PioneerQwen35DataflowRoleCoordinator.h"

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

// These once-only command adapters still need a bounded resident loop, safe
// processor startup/stop, and a physical owner before they can run on-card.
extern "C" unsigned qwen35_queued_reader_command(
    PioneerQwen35::DataflowRoleMailbox* box,unsigned sequence)
{
    if (!box) return PioneerQwen35::DataflowRoleCommandInvalid;
    struct Body { bool Run() { return qwen35_queued_reader_once(); } } body;
    return PioneerQwen35::DataflowRoleRunCommand(*box,PioneerQwen35::DataflowRoleReader,sequence,body);
}
extern "C" unsigned qwen35_queued_writer_command(
    PioneerQwen35::DataflowRoleMailbox* box,unsigned sequence)
{
    if (!box) return PioneerQwen35::DataflowRoleCommandInvalid;
    struct Body { bool Run() { return qwen35_queued_writer_once(); } } body;
    return PioneerQwen35::DataflowRoleRunCommand(*box,PioneerQwen35::DataflowRoleWriter,sequence,body);
}

// Future resident loops can call these bounded ticks. They do not provide
// boot, polling policy, parking, reset control or memory ownership.
extern "C" unsigned qwen35_queued_reader_tick(
    PioneerQwen35::DataflowRoleMailbox* box,PioneerQwen35::DataflowRoleTickState* state)
{
    if (!box || !state) return PioneerQwen35::DataflowRoleTickInvalid;
    struct Body { bool Run() { return qwen35_queued_reader_once(); } } body;
    return PioneerQwen35::DataflowRoleServiceTick(*box,PioneerQwen35::DataflowRoleReader,*state,body);
}
extern "C" unsigned qwen35_queued_writer_tick(
    PioneerQwen35::DataflowRoleMailbox* box,PioneerQwen35::DataflowRoleTickState* state)
{
    if (!box || !state) return PioneerQwen35::DataflowRoleTickInvalid;
    struct Body { bool Run() { return qwen35_queued_writer_once(); } } body;
    return PioneerQwen35::DataflowRoleServiceTick(*box,PioneerQwen35::DataflowRoleWriter,*state,body);
}

// Compile-only coordinator binding. There is no processor launch, mailbox
// initialization, graph integration, or physical queue visibility proof.
extern "C" unsigned qwen35_queued_compute_coordinate(
    PioneerQwen35::DataflowRoleMailbox* box,unsigned sequence,unsigned polls)
{
    using namespace PioneerQwen35;
    if (!box) return DataflowRoleCoordinatorInvalid;
    DataflowDeviceContext context;
    if (!DataflowBindLocalContext(context,DataflowFailureQueue))
        return DataflowRoleCoordinatorInvalid;
    struct Ops {
        DataflowDeviceContext& context;
        DataflowBlackholeTensixOps compute;
        bool RunCompute() {
            return PioneerQwen35DataflowCompute(*context.program,context.queues[0],
                context.queues[1],context.queues[2],compute);
        }
        void Pause() { asm volatile("" ::: "memory"); }
        bool AuditAndResetCleanQueues() {
            return DataflowRoleCleanQueuesForReuse(context.queues[0].control);
        }
    } ops{context,{}};
    return DataflowRoleCoordinate(*box,sequence,polls,ops);
}
