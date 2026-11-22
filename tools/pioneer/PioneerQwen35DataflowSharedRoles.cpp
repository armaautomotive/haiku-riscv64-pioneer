/* SPDX-License-Identifier: MIT */
// Offline-only shared-activation role adapters. They are not linked into the
// resident service and provide no boot, processor launch, or ownership.
#ifdef PIONEER_QWEN35_QUEUED_COMPUTE
#error "shared activation compute must validate pointers into the immutable cache"
#endif
#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowTensixOps.h"
#include "PioneerQwen35DataflowRoleCoordinator.h"
#include "PioneerQwen35DataflowSharedActivationRoles.h"
using namespace PioneerQwen35;
extern "C" bool qwen35_shared_reader_once()
{
    DataflowDeviceContext context;
    if (!DataflowBindLocalContext(context,DataflowFailureReader)) return false;
    DataflowBlackholeReaderOps reader;
    return PioneerQwen35SharedActivationReader(*context.program,context.queues[0],
        context.queues[1],context.activationCache,reader);
}
extern "C" bool qwen35_shared_writer_once()
{
    DataflowBlackholeWriterOps writer;
    return PioneerQwen35DataflowWriterEntry(writer);
}
extern "C" unsigned qwen35_shared_reader_tick(DataflowRoleMailbox* box,DataflowRoleTickState* state)
{
    if (!box || !state) return DataflowRoleTickInvalid;
    struct Body { bool Run() { return qwen35_shared_reader_once(); } } body;
    return DataflowRoleServiceTick(*box,DataflowRoleReader,*state,body);
}
extern "C" unsigned qwen35_shared_writer_tick(DataflowRoleMailbox* box,DataflowRoleTickState* state)
{
    if (!box || !state) return DataflowRoleTickInvalid;
    struct Body { bool Run() { return qwen35_shared_writer_once(); } } body;
    return DataflowRoleServiceTick(*box,DataflowRoleWriter,*state,body);
}
extern "C" unsigned qwen35_shared_compute_coordinate(
    DataflowRoleMailbox* box,unsigned sequence,unsigned polls)
{
    if (!box) return DataflowRoleCoordinatorInvalid;
    DataflowDeviceContext context;
    if (!DataflowBindLocalContext(context,DataflowFailureQueue))
        return DataflowRoleCoordinatorInvalid;
    struct Ops {
        DataflowDeviceContext& context;
        DataflowBlackholeTensixOps compute;
        bool RunCompute() {
            return PioneerQwen35SharedActivationCompute(*context.program,
                context.queues[0],context.queues[1],context.queues[2],
                context.activationCache,compute);
        }
        void Pause() { asm volatile("" ::: "memory"); }
        bool AuditAndResetCleanQueues() {
            return DataflowRoleCleanQueuesForReuse(context.queues[0].control);
        }
    } ops{context,{}};
    return DataflowRoleCoordinate(*box,sequence,polls,ops);
}
