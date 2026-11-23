/* SPDX-License-Identifier: MIT */
// Offline-only shared-activation role adapters. They are not linked into the
// resident service and provide no boot, processor launch, or ownership.
#ifdef PIONEER_QWEN35_QUEUED_COMPUTE
#error "shared activation compute must validate pointers into the immutable cache"
#endif
#define PIONEER_QWEN35_SHARED_ROLE_NOC_SAFETY 1
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
        context.queues[1],context.activationCache,reader,
        reinterpret_cast<DataflowActivationReuse*>(static_cast<uintptr_t>(DataflowActivationReuseAddress)));
}
extern "C" bool qwen35_shared_writer_once()
{
    DataflowDeviceContext context;
    if (!DataflowBindLocalContext(context,DataflowFailureWriter)) return false;
    DataflowBlackholeWriterOps writer;
    return PioneerQwen35SharedActivationWriter(*context.program,context.queues[2],writer);
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
    // Match Tenstorrent's Blackhole set_l1_data_cache<false>() policy before
    // this TRISC reads any shared role state. Preserve every other cfg0 bit.
    // This witness does not yet own the old TRISC0 state or its restoration.
    uint32_t cfg0;
    asm volatile("fence iorw,iorw\n\tcsrsi 0x7c0,8\n\tcsrr %0,0x7c0"
        : "=r"(cfg0) :: "memory");
    if (!(cfg0&8U) || !box) return DataflowRoleCoordinatorInvalid;
    DataflowDeviceContext context;
    if (!DataflowBindLocalContext(context,DataflowFailureQueue))
        return DataflowRoleCoordinatorInvalid;
    struct Ops {
        DataflowDeviceContext& context;
        DataflowBlackholeTensixOps compute;
        bool RunCompute() {
            compute.UseSharedActivationSums();
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
