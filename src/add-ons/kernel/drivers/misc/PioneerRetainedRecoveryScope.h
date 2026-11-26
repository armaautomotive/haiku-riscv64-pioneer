/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RETAINED_RECOVERY_SCOPE_H
#define PIONEER_RETAINED_RECOVERY_SCOPE_H
#if !defined(PIONEER_QWEN35_RETAINED_SCOPE_WITNESS) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) \
    || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Retained recovery scope is an isolated non-loadable witness
#endif
#include <stdint.h>

// Both pins and the exact controller must already be bound to the same live
// device by the caller, and their owner must outlive this object. A driver
// lock must serialize every call. This does not establish device identity,
// hardware lifetime, DMA/MMIO/firmware exclusion, or a release proof.
// Every entered operation parks the controller lease before either pin's
// postcheck runs. No method releases the lease or the pins. A caller may
// mark a B_DEV_NOT_READY result pending only after validating its retained
// recovery ticket and retry semantics; that assertion is not proof here.
template<class Pin,class Controller> class PioneerRetainedRecoveryScope {
public:
    enum Result { Rejected=0, Pending=1, AwaitingProof=2, Retained=3 };
    using Operation=status_t (*)(void*,bool*);
private:
    enum Phase { Fresh,Running,Parked,Finished,Faulted } fPhase=Fresh;
    Pin& fCard;Pin& fRoot;Controller& fController;
    const void* fOwner;uint64_t fSession,fCardGeneration,fRootGeneration,fTicket=0;
    struct Call {
        PioneerRetainedRecoveryScope& self;
        Operation operation;void* argument;
        status_t operationStatus=B_ERROR;bool pending=false,entered=false,parked=false;
        static status_t Root(void* opaque) {
            auto& c=*static_cast<Call*>(opaque);auto& s=c.self;
            status_t status=s.fPhase==Fresh
                ?s.fController.BeginPioneerRetainedRecovery(s.fOwner,s.fSession,s.fTicket)
                :s.fController.ResumePioneerRetainedRecovery(s.fOwner,s.fSession,s.fTicket);
            if (status!=B_OK) return status;
            c.entered=true;s.fPhase=Running;
            c.operationStatus=c.operation(c.argument,&c.pending);
            status=s.fController.ParkPioneerRetainedRecovery(s.fOwner,s.fSession,s.fTicket);
            c.parked=status==B_OK;
            return status;
        }
        static status_t Card(void* opaque) {
            auto& c=*static_cast<Call*>(opaque);auto& s=c.self;status_t result=B_ERROR;
            const status_t status=s.fRoot.Run(s.fOwner,s.fSession,s.fRootGeneration,Root,&c,&result);
            return status==B_OK?result:status;
        }
    };
public:
    PioneerRetainedRecoveryScope(Pin& card,Pin& root,Controller& controller,
        const void* owner,uint64_t session,uint64_t cardGeneration,uint64_t rootGeneration)
        :fCard(card),fRoot(root),fController(controller),fOwner(owner),fSession(session),
         fCardGeneration(cardGeneration),fRootGeneration(rootGeneration) {}
    PioneerRetainedRecoveryScope(const PioneerRetainedRecoveryScope&)=delete;
    PioneerRetainedRecoveryScope& operator=(const PioneerRetainedRecoveryScope&)=delete;
    Result Step(Operation operation,void* argument,status_t& operationStatus) {
        operationStatus=B_ERROR;
        if (!operation || !fOwner || !fSession || !fCardGeneration || !fRootGeneration
            || &fCard==&fRoot
            || (fPhase!=Fresh && fPhase!=Parked)) return Rejected;
        Call call{*this,operation,argument};status_t inner=B_ERROR;
        const status_t outer=fCard.Run(fOwner,fSession,fCardGeneration,Call::Card,&call,&inner);
        operationStatus=call.operationStatus;
        if (outer!=B_OK || inner!=B_OK || !call.entered || !call.parked) {
            fPhase=Faulted;return Retained;
        }
        if (call.pending && call.operationStatus==B_DEV_NOT_READY) { fPhase=Parked;return Pending; }
        if (!call.pending && call.operationStatus==B_OK) { fPhase=Finished;return AwaitingProof; }
        fPhase=Faulted;return Retained;
    }
    uint64_t Ticket() const { return fTicket; }
};
#endif
